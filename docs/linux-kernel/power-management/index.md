---
status: draft
reviewed: false
domain: linux-kernel
difficulty: advanced
last_reviewed: null
---

# Power Management

## What Problem Does This Solve?

Power management is the part of driver development where a driver stops treating
hardware as always-on. A working driver can still be a bad kernel citizen if it
leaves clocks enabled, keeps regulators on, blocks CPU idle states, loses device
state across suspend, or configures wakeup incorrectly.

For driver developers, power management answers practical questions:

- When may this device be turned off while Linux is still running?
- What must be enabled before register access is legal?
- What device state is lost when power, clocks, resets, or a domain are removed?
- Which interrupt or signal is allowed to wake the system?
- What ordering constraints exist between this device, its bus, its parent
  device, and shared providers?
- How do we debug a suspend hang, immediate wakeup, or broken resume?

The goal is not only lower power. Correct power management also makes probe,
remove, error recovery, suspend, resume, and repeated runtime transitions more
predictable.

## Prerequisites

This chapter assumes you are comfortable with:

- platform and bus driver structure
- probe and remove lifetimes
- devm-managed resources
- clocks, regulators, resets, pinctrl, and interrupts
- kernel locking and sleepable versus atomic context
- basic Device Tree consumer properties
- reading `dmesg`, sysfs, debugfs, and trace output

If those topics still feel new, keep the examples here close to the specific
driver you are studying. Power management becomes much easier when each
transition is written as a concrete hardware sequence instead of a set of vague
callbacks.

## The Main Forms Of Kernel Power Management

Power management appears in several layers. They are related, but they solve
different problems.

| Area | Main Question | Common Driver Impact |
| --- | --- | --- |
| Runtime PM | Can this device idle while the system remains awake? | Gate clocks, disable regulators, stop DMA, save small state |
| System suspend/resume | Can the whole machine enter a sleep state and recover? | Quiesce I/O, configure wake, save/restore volatile state |
| Wakeup handling | Which device may bring the system out of sleep? | Enable wake IRQs, expose policy, avoid false wakeups |
| CPU idle/frequency | Can CPUs enter deeper idle states or scale frequency? | Avoid needless polling, respect latency, avoid excess wakeups |
| Power domains | Is this device inside a shared power island? | Let genpd/runtime PM coordinate domain on/off |
| Regulator/clock/reset/pin dependencies | What must be sequenced before hardware access? | Build reliable power-up and power-down paths |
| Debugging | Where did the transition fail? | Trace phases, inspect wake sources, check resource state |

## Mental Model

Think of a device as sitting inside a dependency graph:

```text
userspace request or kernel client
  -> subsystem operation
     -> driver runtime PM get
        -> parent/bus runtime PM
           -> power domain
              -> regulators
              -> clocks
              -> resets
              -> pin states
              -> IRQ/wake configuration
                 -> register access / DMA / transfers
```

The driver owns the parts that are specific to the device. The platform owns the
board wiring and many shared providers. The PM core, bus, and generic PM domain
code coordinate ordering when drivers use the common APIs correctly.

A driver should therefore avoid private shortcuts such as:

- touching SoC power registers directly from a leaf driver
- assuming firmware left clocks or regulators on
- accessing registers before runtime PM has resumed the device
- enabling wakeup unconditionally regardless of user policy
- using delays to hide missing sequencing
- depending on bootloader state for suspend/resume behavior

## Runtime PM Versus System Sleep

Runtime PM and system sleep often use similar hardware sequences, but they are
not the same event.

Runtime PM happens while Linux is running:

```text
camera sensor unused for 2 seconds
  -> runtime suspend
     -> stop streaming
     -> disable clock
     -> maybe disable regulator
```

System sleep happens when the machine enters a suspend state:

```text
echo mem > /sys/power/state
  -> freeze userspace
  -> suspend devices in dependency order
  -> enter platform sleep state
  -> wake
  -> resume devices
  -> thaw userspace
```

Many drivers reuse runtime suspend/resume code for system sleep with helpers
such as `pm_runtime_force_suspend()` and `pm_runtime_force_resume()`. That is
often the cleanest design when runtime suspend already represents the device's
low-power state. It is not correct when system sleep needs different behavior,
such as arming a wake IRQ, preserving firmware context, or leaving a bus segment
powered.

## Device Power Is A State Machine

A driver usually has several practical states:

```text
not probed
  -> resources acquired
  -> hardware powered
  -> hardware initialized
  -> idle but runtime active
  -> runtime suspended
  -> system suspended
  -> resumed
  -> removed
```

Each transition should have an explicit owner and a matching reverse operation.

Example transition table:

| Transition | Typical Actions | Reverse |
| --- | --- | --- |
| Probe power-up | enable regulators, enable clocks, deassert reset, read ID | remove or probe failure cleanup |
| Runtime suspend | stop I/O, save registers, disable IRQ path if needed, gate clock | runtime resume |
| Runtime resume | enable clock, restore state, re-enable I/O path | runtime suspend |
| System suspend | block new I/O, flush work, arm wakeup, enter sleep state | system resume |
| Remove | stop users, disable runtime PM, return hardware to safe state | none |

This is one reason power management bugs are often lifecycle bugs. The broken
part is not a single API call; it is an incomplete transition.

## What A Driver Must Know About Its Hardware

Before writing PM callbacks, answer these questions from the datasheet, board
schematic, binding, and subsystem examples:

- Which power rails are required, and which are optional?
- Are any rails shared with other devices?
- Which clocks are required before register access?
- Does reset need to be asserted before power changes?
- Are registers lost when clocks stop, regulators turn off, or the domain powers
  down?
- Does the device need a startup delay after power, clock, or reset changes?
- Can the device wake the system, and through which interrupt or GPIO?
- Does the wake signal require a pin state, always-on domain, or always-on rail?
- Can DMA be active during suspend, or must all DMA be stopped first?
- Is there firmware ownership of power state?
- Does the bus or subsystem already implement part of the sequence?

Write the answers down in the driver or binding review notes. If you cannot
describe the sequence in prose, the code will usually be fragile.

## Chapter Structure

Read the pages in this order if you are building a driver from scratch.

1. [Runtime PM](runtime-pm.md) covers idle power management while the system is
   awake.
2. [Suspend And Resume](suspend-resume.md) explains system-wide sleep phases and
   driver callbacks.
3. [Wake Sources](wake-sources.md) covers wakeup-capable devices, wake IRQs, and
   policy ownership.
4. [cpuidle And cpufreq](cpuidle-cpufreq.md) explains how CPU power management
   interacts with driver behavior.
5. [Power Domains](power-domains.md) covers generic PM domains and shared power
   islands.
6. [Regulator And Clock Power Dependencies](regulator-clock-power-dependencies.md)
   ties regulators, clocks, resets, pinctrl, domains, and runtime PM into one
   sequencing model.
7. [Suspend And Resume Debugging](suspend-resume-debugging.md) gives a practical
   workflow for failures.

## A Minimal Driver PM Shape

A small platform driver often grows toward this shape:

```c
struct demo_priv {
    struct device *dev;
    void __iomem *base;
    struct clk *clk;
    struct regulator *vdd;
    struct reset_control *rst;
    int irq;
};
```

Probe acquires resources and leaves the device in a known state:

```c
static int demo_probe(struct platform_device *pdev)
{
    struct device *dev = &pdev->dev;
    struct demo_priv *priv;
    int ret;

    priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
    if (!priv)
        return -ENOMEM;

    priv->dev = dev;
    platform_set_drvdata(pdev, priv);

    priv->vdd = devm_regulator_get(dev, "vdd");
    if (IS_ERR(priv->vdd))
        return dev_err_probe(dev, PTR_ERR(priv->vdd),
                             "failed to get vdd\n");

    priv->clk = devm_clk_get(dev, NULL);
    if (IS_ERR(priv->clk))
        return dev_err_probe(dev, PTR_ERR(priv->clk),
                             "failed to get clock\n");

    priv->rst = devm_reset_control_get_optional_exclusive(dev, NULL);
    if (IS_ERR(priv->rst))
        return dev_err_probe(dev, PTR_ERR(priv->rst),
                             "failed to get reset\n");

    ret = demo_hw_power_on(priv);
    if (ret)
        return ret;

    ret = demo_hw_init(priv);
    if (ret)
        goto err_power_off;

    pm_runtime_set_active(dev);
    pm_runtime_enable(dev);
    pm_runtime_set_autosuspend_delay(dev, 1000);
    pm_runtime_use_autosuspend(dev);
    pm_runtime_mark_last_busy(dev);
    pm_runtime_put_autosuspend(dev);

    return 0;

err_power_off:
    demo_hw_power_off(priv);
    return ret;
}
```

The operation path brackets hardware access with runtime PM:

```c
static int demo_read_value(struct demo_priv *priv, u32 *value)
{
    int ret;

    ret = pm_runtime_resume_and_get(priv->dev);
    if (ret)
        return ret;

    *value = readl(priv->base + DEMO_VALUE);

    pm_runtime_mark_last_busy(priv->dev);
    pm_runtime_put_autosuspend(priv->dev);

    return 0;
}
```

Power callbacks contain the hardware-specific transition:

```c
static int demo_runtime_suspend(struct device *dev)
{
    struct demo_priv *priv = dev_get_drvdata(dev);

    demo_stop_io(priv);
    demo_hw_power_off(priv);

    return 0;
}

static int demo_runtime_resume(struct device *dev)
{
    struct demo_priv *priv = dev_get_drvdata(dev);
    int ret;

    ret = demo_hw_power_on(priv);
    if (ret)
        return ret;

    return demo_restore_registers(priv);
}
```

The actual driver may need a different order, but the design intent is stable:
acquire resources once, encode power transitions once, and have every hardware
access prove the device is active.

## Policy Versus Mechanism

Kernel drivers should implement mechanism. Policy normally belongs to userspace,
the subsystem, platform firmware, or board data.

Examples:

| Mechanism In Driver | Policy Elsewhere |
| --- | --- |
| expose runtime PM support | userspace may set `power/control` to `auto` or `on` |
| mark device wakeup capable | userspace/platform decides whether wakeup is enabled |
| request named supplies | Device Tree/ACPI describes actual PMIC rail |
| support autosuspend delay | subsystem or driver default chooses delay |
| support OPPs/frequency changes | cpufreq/devfreq governor chooses performance point |

Do not hard-code product policy in a generic driver unless the binding or
subsystem contract makes that policy part of the hardware description.

## Common Failure Patterns

Power management failures often repeat across subsystems:

| Symptom | Common Cause |
| --- | --- |
| Register read returns all ones or bus fault | power domain, clock, or reset not active |
| Driver works after bootloader but not from cold boot | missing regulator, reset, or startup delay |
| First transfer works, second transfer fails | runtime PM suspended the device between operations |
| System immediately wakes from suspend | wake IRQ left asserted, bad trigger type, unacked interrupt |
| Device missing after resume | volatile registers not restored |
| Suspend hangs at one device | active DMA/work/IRQ path not quiesced |
| High idle power | usage count leak, wakeup source active, polling, clock left prepared |
| `-EPROBE_DEFER` during probe | provider for clock/regulator/domain/reset not ready |

## Completion Criteria

You understand this chapter when you can:

- explain the difference between runtime PM and system sleep
- identify the dependencies that must be valid before register access
- write a runtime PM get/put pair around a hardware operation
- design suspend/resume callbacks that stop I/O and restore state
- configure wakeup as a policy-controlled capability
- inspect runtime PM state and wakeup statistics from sysfs/debugfs
- reason about a device inside a power domain
- debug whether a failure happened entering suspend, staying suspended, waking,
  or resuming

## Official References

- [Runtime PM](https://docs.kernel.org/power/runtime_pm.html)
- [Device Power Management Basics](https://docs.kernel.org/driver-api/pm/devices.html)
- [System Sleep States](https://docs.kernel.org/admin-guide/pm/sleep-states.html)
- [CPU Idle Time Management](https://docs.kernel.org/admin-guide/pm/cpuidle.html)
- [Regulator API](https://docs.kernel.org/driver-api/regulator.html)
- [Common Clock Framework](https://docs.kernel.org/driver-api/clk.html)

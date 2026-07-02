---
status: draft
reviewed: false
domain: linux-kernel
difficulty: advanced
last_reviewed: null
---

# Wake Sources

## What Problem Does This Solve?

Wake sources are devices or events that can bring a suspended system back to the
running state. A driver must distinguish between:

- hardware that is capable of wakeup
- policy that says wakeup is currently enabled
- the interrupt or signal used for wakeup
- the power, pin, and domain state required for that signal to work

Without that distinction, systems either fail to wake or wake immediately for
the wrong reason.

Example failures:

```text
button should wake system
  -> driver disabled GPIO bank clock
  -> button press is never seen

touchscreen should not wake system
  -> interrupt left as wake source
  -> suspend immediately exits

network device should wake on packet
  -> power domain is off
  -> wake packet cannot reach interrupt controller
```

## Core Concepts

### Wakeup-Capable Device

A wakeup-capable device has hardware wiring that can signal a wake event while
the system is asleep.

Examples:

- power button
- GPIO key
- keyboard
- touch controller interrupt
- RTC alarm
- network wake-on-LAN
- USB remote wakeup
- modem or Bluetooth host-wake GPIO

Capability is usually described by firmware or board data. In Device Tree, many
bindings use:

```dts
wakeup-source;
```

Driver code may mark capability:

```c
device_init_wakeup(dev, true);
```

Do this only when the hardware, binding, and platform wiring actually support
wake.

### Wakeup Policy

Capability does not mean wake is enabled. Policy decides whether the device may
wake the system now.

Sysfs exposes policy:

```sh
cat /sys/devices/.../power/wakeup
```

Typical values:

```text
enabled
disabled
```

Driver code checks policy:

```c
if (device_may_wakeup(dev))
    enable_irq_wake(priv->irq);
```

`device_may_wakeup(dev)` should be the normal gate in suspend callbacks. It
means the device is both wakeup-capable and currently enabled for wakeup.

### Wake IRQ

A wake IRQ is the interrupt line that can wake the system.

There are two common patterns.

Explicit suspend/resume handling:

```c
static int demo_suspend(struct device *dev)
{
    struct demo_priv *priv = dev_get_drvdata(dev);

    if (device_may_wakeup(dev))
        enable_irq_wake(priv->irq);

    return 0;
}

static int demo_resume(struct device *dev)
{
    struct demo_priv *priv = dev_get_drvdata(dev);

    if (device_may_wakeup(dev))
        disable_irq_wake(priv->irq);

    return 0;
}
```

Device PM wake IRQ helper:

```c
ret = dev_pm_set_wake_irq(dev, priv->irq);
if (ret)
    return ret;
```

Cleanup:

```c
dev_pm_clear_wake_irq(dev);
```

Use subsystem and driver-family conventions. Some devices use the same IRQ for
normal operation and wake; others have a separate wake GPIO or wake IRQ.

### Wakeup Source Object

The kernel also has wakeup source accounting. A wakeup source represents an
event that can keep the system awake long enough for the kernel or userspace to
handle it.

Drivers may use wakeup-source helpers when they need to report an event that
should prevent immediate re-suspend:

```c
pm_wakeup_event(dev, 5000);
```

This example tells the PM core that a wake event associated with `dev` should
keep the system awake for a bounded time.

This is different from enabling an IRQ as a hardware wake signal. A complete
wakeup-capable driver may need both:

```text
enable_irq_wake()
  -> hardware can wake the system

pm_wakeup_event()
  -> kernel accounts for the wake event after it happens
```

### Wake Reason

After a wakeup, you need to identify what caused it. Evidence may come from:

- `/sys/kernel/debug/wakeup_sources`
- interrupt counters in `/proc/interrupts`
- driver logs
- platform firmware wake reason registers
- RTC alarm state
- GPIO controller status
- PM trace events

Wake reason reporting is platform-dependent. Do not assume one interface exists
everywhere.

## Device Tree Example

GPIO key:

```dts
gpio-keys {
    compatible = "gpio-keys";

    power-button {
        label = "Power";
        linux,code = <KEY_POWER>;
        gpios = <&gpio1 4 GPIO_ACTIVE_LOW>;
        wakeup-source;
    };
};
```

I2C touch controller:

```dts
touch@38 {
    compatible = "example,touch";
    reg = <0x38>;
    interrupt-parent = <&gpio2>;
    interrupts = <7 IRQ_TYPE_LEVEL_LOW>;
    vdd-supply = <&vdd_3v3>;
    wakeup-source;
};
```

The binding must define whether `wakeup-source` is valid. Do not add it to a
node just because a device has an interrupt.

## Driver Capability Setup

Probe:

```c
static int demo_probe(struct i2c_client *client)
{
    struct device *dev = &client->dev;
    struct demo_priv *priv;
    int ret;

    priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
    if (!priv)
        return -ENOMEM;

    i2c_set_clientdata(client, priv);
    priv->irq = client->irq;

    ret = devm_request_threaded_irq(dev, priv->irq, NULL, demo_irq_thread,
                                    IRQF_ONESHOT, dev_name(dev), priv);
    if (ret)
        return ret;

    if (device_property_read_bool(dev, "wakeup-source"))
        device_init_wakeup(dev, true);

    return 0;
}
```

This pattern treats firmware data as the source of hardware capability and
leaves enable/disable policy to sysfs or platform policy.

If every instance of the hardware is always wake-capable by binding contract,
the driver may not need to check a property before calling
`device_init_wakeup()`. Follow the binding and subsystem precedent.

## Suspend And Resume With Wake

Wake-capable suspend sequence:

```text
stop normal I/O
clear stale interrupt status
configure device wake mode
enable IRQ wake if policy allows it
enter low-power state that preserves wake path
```

Example:

```c
static int demo_suspend(struct device *dev)
{
    struct demo_priv *priv = dev_get_drvdata(dev);
    int ret;

    ret = demo_stop_streaming(priv);
    if (ret)
        return ret;

    demo_clear_pending_irq(priv);

    if (device_may_wakeup(dev)) {
        ret = demo_enable_device_wake_mode(priv);
        if (ret)
            return ret;

        ret = enable_irq_wake(priv->irq);
        if (ret) {
            demo_disable_device_wake_mode(priv);
            return ret;
        }
    }

    return demo_enter_suspend_power_state(priv);
}
```

Resume:

```c
static int demo_resume(struct device *dev)
{
    struct demo_priv *priv = dev_get_drvdata(dev);
    int ret;

    ret = demo_exit_suspend_power_state(priv);
    if (ret)
        return ret;

    if (device_may_wakeup(dev)) {
        disable_irq_wake(priv->irq);
        demo_disable_device_wake_mode(priv);
    }

    demo_clear_pending_irq(priv);

    return demo_restart_streaming_if_needed(priv);
}
```

The exact ordering depends on hardware. Some devices require wake mode before
interrupt wake is enabled; some require the interrupt to remain masked as a
normal IRQ but armed as a wake signal; some use a dedicated always-on wake GPIO.

## Separate Normal IRQ And Wake IRQ

Some devices have one interrupt for normal data and a separate wake signal:

```dts
modem@0 {
    compatible = "example,modem";
    interrupts = <10 IRQ_TYPE_LEVEL_HIGH>,
                 <11 IRQ_TYPE_EDGE_RISING>;
    interrupt-names = "data", "wakeup";
    wakeup-source;
};
```

Driver:

```c
priv->data_irq = platform_get_irq_byname(pdev, "data");
if (priv->data_irq < 0)
    return priv->data_irq;

priv->wake_irq = platform_get_irq_byname(pdev, "wakeup");
if (priv->wake_irq < 0)
    return priv->wake_irq;

ret = dev_pm_set_wake_irq(dev, priv->wake_irq);
if (ret)
    return ret;
```

This is often cleaner than overloading a data interrupt when the hardware
provides a real wake line.

## Wakeup And Power Domains

A wake signal only works if the path from the device to the interrupt controller
remains powered and correctly configured.

Check:

```text
device wake logic powered?
GPIO bank powered?
interrupt controller wake-capable?
pinmux sleep state preserves wake function?
power domain allowed to turn off?
regulator needed for wake still enabled?
firmware configured for wake?
```

Example issue:

```text
touch controller interrupt is a GPIO
  -> touch controller vdd is kept on
  -> GPIO controller domain powers off
  -> wake signal cannot be latched
```

The fix may be in the GPIO controller, PM domain provider, pinctrl sleep state,
firmware, or board description, not in the touch driver alone.

## Wakeup And Runtime PM

Runtime PM and system wakeup are related but separate.

Runtime wake:

```text
device runtime suspended
interrupt occurs while system awake
driver resumes device and handles event
```

System wake:

```text
system suspended
hardware wake signal reaches wake controller
platform resumes
driver handles event
```

If a device may generate interrupts while runtime suspended, the driver must
keep enough wake or IRQ path enabled for runtime operation. That does not
automatically mean the device may wake the whole system from suspend.

## Wakeup Source Accounting

Inspect wakeup accounting:

```sh
sudo cat /sys/kernel/debug/wakeup_sources
```

Fields vary by kernel, but commonly include counts and timing for wakeup source
activity.

Useful questions:

- Which source is active before suspend?
- Which source has a new event count after immediate wake?
- Is a source staying active too long?
- Does the expected device show a wake event at all?

Wakeup source accounting is especially useful when the system resumes
immediately after entering suspend.

## Debugging Immediate Wakeups

Basic workflow:

1. Record baseline interrupt counts.
2. Clear or read device status registers where possible.
3. Attempt suspend.
4. Check wakeup source counters and interrupt deltas.
5. Inspect the driver's suspend path for stale pending status.

Commands:

```sh
cat /proc/interrupts > /tmp/irqs.before
sudo cat /sys/kernel/debug/wakeup_sources > /tmp/wakeup.before

sudo rtcwake -m mem -s 10

cat /proc/interrupts > /tmp/irqs.after
sudo cat /sys/kernel/debug/wakeup_sources > /tmp/wakeup.after
diff -u /tmp/wakeup.before /tmp/wakeup.after
```

Common causes:

| Cause | Evidence | Fix |
| --- | --- | --- |
| Level IRQ still asserted | immediate wake, IRQ count increases | clear device status before suspend |
| Wrong IRQ trigger type | repeated wake or missed wake | fix firmware interrupt flags |
| Wake enabled against policy | device wakes even when sysfs says disabled | gate with `device_may_wakeup()` |
| GPIO/pinctrl sleep state wrong | no wake or constant wake | fix `sleep` state |
| Power domain off | expected wake never happens | keep wake path powered or use wake-capable domain |
| Shared IRQ confusion | unrelated device appears to wake | inspect all users and status registers |

## Debugging Missed Wakeups

If the system never wakes when it should:

```sh
cat /sys/devices/.../power/wakeup
cat /proc/interrupts
sudo cat /sys/kernel/debug/wakeup_sources
```

Check:

- Is the device marked wakeup-capable?
- Is wakeup enabled by policy?
- Does suspend call `enable_irq_wake()` or configure a wake IRQ helper?
- Is the interrupt trigger type correct?
- Is the device's wake logic powered in suspend?
- Is the pinctrl sleep state preserving input and bias?
- Is the GPIO or interrupt controller itself wake-capable?
- Does firmware need platform-specific wake configuration?

Use a scope or logic analyzer if possible. Software can confirm that it asked
for wake; hardware evidence confirms whether the signal actually toggled.

## Common Wake Source Bugs

| Bug | Symptom | Fix |
| --- | --- | --- |
| Treating capability as policy | unwanted wakeups | check `device_may_wakeup()` in suspend |
| Missing `device_init_wakeup()` | no `/power/wakeup`, driver never arms wake | mark capability from firmware/binding |
| Stale level interrupt | immediate resume | clear status before enabling wake |
| Disabling wake path power | missed wake | keep needed rail/domain/GPIO bank alive |
| Wrong pinctrl sleep state | missed wake or high leakage | define correct sleep pins |
| Same IRQ used incorrectly | normal IRQ handler races with wake path | separate normal IRQ masking from wake enable |
| No wakeup accounting | hard to debug wake reason | call `pm_wakeup_event()` when appropriate |
| Wake mode not disabled on resume | repeated interrupts or odd runtime behavior | undo wake-specific device configuration |

## Practice Exercises

1. Find a Device Tree node with `wakeup-source`. Identify the interrupt or GPIO
   that can wake the system.
2. Check the corresponding sysfs `power/wakeup` file and toggle it between
   `enabled` and `disabled`.
3. Trace the driver's suspend callback and verify whether it uses
   `device_may_wakeup()`.
4. Trigger a controlled wake event and compare `/proc/interrupts` before and
   after suspend.
5. Inspect pinctrl sleep state and decide whether the wake pin remains an input
   with the right bias.

## Review Checklist

- Is wakeup capability described by firmware or binding?
- Does policy remain user/platform controllable?
- Does suspend use `device_may_wakeup()`?
- Is the correct IRQ or wake GPIO armed?
- Are pending level-triggered events cleared before suspend?
- Does the wake path remain powered and pinmuxed?
- Is wake-specific device configuration undone on resume?
- Is wakeup source accounting used for events that should keep the system awake?

## Related Topics

- [Suspend And Resume](suspend-resume.md)
- [Power Domains](power-domains.md)
- [IRQ Handling](../driver-interfaces/irq-handling.md)

## Official References

- [Device Power Management Basics](https://docs.kernel.org/driver-api/pm/devices.html)
- [System Sleep States](https://docs.kernel.org/admin-guide/pm/sleep-states.html)
- [Runtime PM](https://docs.kernel.org/power/runtime_pm.html)

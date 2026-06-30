---
status: draft
reviewed: false
domain: linux-kernel
difficulty: advanced
last_reviewed: null
---

# Runtime PM

## What Problem Does This Solve?

Runtime power management lets a device enter a low-power state while the rest of
the system continues running.

Without runtime PM, a driver often keeps hardware powered from probe until
remove:

```text
probe
  -> enable regulator
  -> enable clock
  -> initialize device
  -> leave everything on forever
```

With runtime PM, the device is powered only while it is needed:

```text
operation starts
  -> runtime resume
  -> access hardware
operation ends
  -> mark last busy
  -> autosuspend after delay
  -> runtime suspend
```

This matters for battery devices, thermals, fan noise, embedded boards, and
server idle power. It also matters for correctness: the same code that can
reliably power a device down and back up is often reused for system suspend,
resume, reset recovery, and probe failure paths.

## Core Concepts

### Runtime Active And Runtime Suspended

The PM core tracks whether a device is considered runtime active or runtime
suspended.

Runtime active means the device is usable by its driver:

```text
registers accessible
clocks valid
power rails valid
device state restored
I/O may be submitted
```

Runtime suspended means the device should not be touched by normal I/O paths:

```text
clocks may be gated
regulators may be disabled
power domain may be off
register contents may be lost
DMA engine must be idle
```

The exact hardware state is driver-specific. The PM core manages state and
ordering; the driver implements the hardware transition.

### Usage Count

Runtime PM uses a usage count to decide whether a device may suspend.

Conceptually:

```text
usage_count > 0  -> device is in use; keep active
usage_count == 0 -> device may idle or autosuspend
```

Driver operations increment the usage count before touching hardware and drop it
afterward:

```c
ret = pm_runtime_resume_and_get(dev);
if (ret)
    return ret;

/* hardware access is allowed here */

pm_runtime_mark_last_busy(dev);
pm_runtime_put_autosuspend(dev);
```

If a driver leaks a usage count, the device never suspends. If it drops the
usage count too early, the device may suspend while work is still using it.

### Runtime PM Callbacks

Drivers provide callbacks in `struct dev_pm_ops`:

```c
static int demo_runtime_suspend(struct device *dev)
{
    struct demo_priv *priv = dev_get_drvdata(dev);

    demo_stop_transfers(priv);
    demo_save_volatile_state(priv);
    clk_disable_unprepare(priv->clk);
    regulator_disable(priv->vdd);

    return 0;
}

static int demo_runtime_resume(struct device *dev)
{
    struct demo_priv *priv = dev_get_drvdata(dev);
    int ret;

    ret = regulator_enable(priv->vdd);
    if (ret)
        return ret;

    ret = clk_prepare_enable(priv->clk);
    if (ret) {
        regulator_disable(priv->vdd);
        return ret;
    }

    ret = demo_restore_volatile_state(priv);
    if (ret) {
        clk_disable_unprepare(priv->clk);
        regulator_disable(priv->vdd);
        return ret;
    }

    return 0;
}

static const struct dev_pm_ops demo_pm_ops = {
    .runtime_suspend = demo_runtime_suspend,
    .runtime_resume = demo_runtime_resume,
};
```

The callbacks must be sleepable because they commonly enable regulators, prepare
clocks, wait for hardware, and communicate with buses. Do not call runtime PM
get helpers from hard IRQ context unless you are using a non-sleeping variant
and you know the device is already active.

### Autosuspend

Autosuspend delays runtime suspend until the device has been idle for a period.
This avoids power-cycling hardware between closely spaced operations.

Example:

```c
pm_runtime_set_autosuspend_delay(dev, 1000);
pm_runtime_use_autosuspend(dev);
```

Operation path:

```c
ret = pm_runtime_resume_and_get(dev);
if (ret)
    return ret;

ret = demo_transfer(priv, msg);

pm_runtime_mark_last_busy(dev);
pm_runtime_put_autosuspend(dev);

return ret;
```

If the next operation arrives before the autosuspend delay expires, the device
stays active and avoids a full resume.

Choose the delay based on measured behavior:

| Device Pattern | Autosuspend Strategy |
| --- | --- |
| Human input device | short delay, wake-capable |
| Sensor sampled every second | delay slightly longer than sampling interval |
| Flash storage | subsystem policy; avoid churn during I/O bursts |
| Audio interface | coordinate with audio subsystem power state |
| Rarely used peripheral | short or no autosuspend delay |

### Parent And Bus Dependencies

Devices live below parents:

```text
I2C sensor
  -> I2C controller
     -> bus clock
        -> power domain
```

Runtime PM ordering must keep parents active while children are active. Bus
subsystems and the device core handle much of this when drivers use the common
model. A leaf driver should not manually power parent devices by poking their
registers.

Example consequence:

```text
sensor driver calls pm_runtime_resume_and_get(sensor)
  -> sensor runtime resume may require I2C transfers
  -> I2C controller and its parent resources must be available
```

If a runtime resume callback needs bus I/O, check the subsystem's PM rules. Some
buses resume the parent before the child callback; some drivers must avoid bus
transactions in certain PM phases.

### Runtime Idle

The optional `runtime_idle` callback is called when the PM core sees the device
become idle. Many simple drivers do not implement it and rely on autosuspend.

Example:

```c
static int demo_runtime_idle(struct device *dev)
{
    pm_runtime_mark_last_busy(dev);
    pm_runtime_autosuspend(dev);
    return 0;
}
```

Use this only when the driver or subsystem has a specific reason to control idle
handling. For many drivers, explicit `pm_runtime_put_autosuspend()` calls are
clearer.

## Probe Setup

Runtime PM setup order matters.

A common pattern when probe powers and initializes the device:

```c
static int demo_probe(struct platform_device *pdev)
{
    struct device *dev = &pdev->dev;
    struct demo_priv *priv;
    int ret;

    priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
    if (!priv)
        return -ENOMEM;

    platform_set_drvdata(pdev, priv);
    priv->dev = dev;

    ret = demo_get_resources(priv);
    if (ret)
        return ret;

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

Why this order?

- `demo_hw_power_on()` makes the hardware accessible for probe-time discovery.
- `demo_hw_init()` leaves the device in a known active state.
- `pm_runtime_set_active()` tells the PM core that the device is already active.
- `pm_runtime_enable()` allows runtime PM transitions.
- `pm_runtime_put_autosuspend()` drops the initial active reference and lets the
  device idle later.

Do not enable runtime PM before your driver data and resources are ready for a
callback. A runtime suspend or resume callback may run sooner than expected.

## Remove And Probe Failure

Remove must stop new users, synchronize outstanding work, and disable runtime PM
without racing active operations.

One common shape:

```c
static void demo_remove(struct platform_device *pdev)
{
    struct device *dev = &pdev->dev;
    struct demo_priv *priv = platform_get_drvdata(pdev);

    demo_unregister_users(priv);
    cancel_work_sync(&priv->work);

    pm_runtime_disable(dev);

    if (!pm_runtime_status_suspended(dev))
        demo_hw_power_off(priv);

    pm_runtime_set_suspended(dev);
}
```

This is not universal. Some drivers use devm cleanup actions, some leave final
power-down to runtime suspend, and some hardware must remain on for firmware or
shared-resource reasons. The important rule is that remove must not allow a
callback or worker to access hardware after the resources have been released.

For probe failure after runtime PM has been enabled, undo it explicitly:

```c
pm_runtime_disable(dev);
pm_runtime_dont_use_autosuspend(dev);
demo_hw_power_off(priv);
```

When possible, use `devm_add_action_or_reset()` for one-way enable steps so
failure paths stay readable.

## Bracketing Hardware Access

Every path that touches registers, DMA engines, FIFOs, or device state must
prove that the device is active.

Good:

```c
static int demo_set_mode(struct demo_priv *priv, u32 mode)
{
    int ret;

    ret = pm_runtime_resume_and_get(priv->dev);
    if (ret)
        return ret;

    writel(mode, priv->base + DEMO_MODE);

    pm_runtime_mark_last_busy(priv->dev);
    pm_runtime_put_autosuspend(priv->dev);

    return 0;
}
```

Bad:

```c
static int demo_set_mode(struct demo_priv *priv, u32 mode)
{
    writel(mode, priv->base + DEMO_MODE);
    return 0;
}
```

The bad version may work during probe testing because the device happens to be
active. It can fail later after autosuspend, after system resume, or on a board
where firmware starts with the device off.

## Error Handling Around Runtime PM Gets

Prefer:

```c
ret = pm_runtime_resume_and_get(dev);
if (ret)
    return ret;
```

This helper resumes the device and increments the usage count only on success.
Older code often uses `pm_runtime_get_sync()`, but that API can require careful
error-path usage-count handling.

If an operation fails after a successful get, still put the device:

```c
ret = pm_runtime_resume_and_get(dev);
if (ret)
    return ret;

ret = demo_start_transfer(priv);
if (ret)
    goto out_pm;

ret = demo_wait_done(priv);

out_pm:
pm_runtime_mark_last_busy(dev);
pm_runtime_put_autosuspend(dev);
return ret;
```

The put belongs to the successful get, not to the success of the hardware
operation.

## Locking And Runtime PM

Runtime PM callbacks and operation paths may share state. Protect that state
with normal kernel locking, but avoid deadlocks.

Common pattern:

```c
static int demo_configure(struct demo_priv *priv, u32 cfg)
{
    int ret;

    ret = pm_runtime_resume_and_get(priv->dev);
    if (ret)
        return ret;

    mutex_lock(&priv->lock);
    writel(cfg, priv->base + DEMO_CFG);
    priv->cached_cfg = cfg;
    mutex_unlock(&priv->lock);

    pm_runtime_mark_last_busy(priv->dev);
    pm_runtime_put_autosuspend(priv->dev);

    return 0;
}
```

Callback:

```c
static int demo_runtime_resume(struct device *dev)
{
    struct demo_priv *priv = dev_get_drvdata(dev);
    int ret;

    ret = demo_hw_power_on(priv);
    if (ret)
        return ret;

    mutex_lock(&priv->lock);
    writel(priv->cached_cfg, priv->base + DEMO_CFG);
    mutex_unlock(&priv->lock);

    return 0;
}
```

Avoid this pattern:

```c
mutex_lock(&priv->lock);
ret = pm_runtime_resume_and_get(dev);
```

If `runtime_resume` also needs `priv->lock`, the resume path can deadlock. In
general, resume the device before taking locks also used by PM callbacks, unless
the driver has a carefully documented lock order.

## Interrupts, Workqueues, And Timers

Runtime suspend must ensure asynchronous paths will not touch powered-off
hardware.

Before disabling clocks or power:

```text
stop new transfers
disable or mask hardware interrupts if required
stop DMA
flush or cancel work that may access registers
save volatile state
disable clock/regulator
```

Example:

```c
static int demo_runtime_suspend(struct device *dev)
{
    struct demo_priv *priv = dev_get_drvdata(dev);

    disable_irq(priv->irq);
    cancel_work_sync(&priv->work);

    demo_mask_device_irqs(priv);
    demo_stop_dma(priv);

    clk_disable_unprepare(priv->clk);

    return 0;
}
```

If the IRQ is a wake IRQ or must remain usable, the sequence is different. Do
not blindly disable an interrupt that is supposed to wake the device or system.

## Runtime PM And Register Caches

Many devices lose volatile register contents when clocks or rails are removed.
Drivers should separate:

- configuration state remembered by software
- hardware state currently programmed into registers
- volatile status that cannot be restored

Example:

```c
struct demo_priv {
    struct device *dev;
    void __iomem *base;
    u32 cached_mode;
    u32 cached_rate;
    struct mutex lock;
};
```

Setter:

```c
mutex_lock(&priv->lock);
priv->cached_mode = mode;
writel(mode, priv->base + DEMO_MODE);
mutex_unlock(&priv->lock);
```

Resume:

```c
mutex_lock(&priv->lock);
writel(priv->cached_mode, priv->base + DEMO_MODE);
writel(priv->cached_rate, priv->base + DEMO_RATE);
mutex_unlock(&priv->lock);
```

Subsystems such as regmap can help with register caching for suitable devices,
but the driver still needs to know which registers are volatile and when the
cache must be synchronized.

## Runtime PM And System Sleep

System suspend can happen while a device is runtime suspended, runtime active,
or transitioning.

If runtime suspend represents the same low-power state needed for system sleep,
the system sleep callbacks can delegate:

```c
static int demo_suspend(struct device *dev)
{
    return pm_runtime_force_suspend(dev);
}

static int demo_resume(struct device *dev)
{
    return pm_runtime_force_resume(dev);
}

static const struct dev_pm_ops demo_pm_ops = {
    .runtime_suspend = demo_runtime_suspend,
    .runtime_resume = demo_runtime_resume,
    .suspend = demo_suspend,
    .resume = demo_resume,
};
```

This avoids maintaining two nearly identical power-down sequences.

Do not use this blindly when system sleep needs extra work:

- enabling or disabling wake IRQs
- selecting pinctrl sleep state
- preventing a bus from being powered off
- coordinating with firmware
- preserving state across hibernation
- leaving part of the device powered for remote wake

In those cases, system sleep callbacks may wrap or extend the runtime PM
sequence.

## Sysfs Runtime PM Controls

For a probed device, runtime PM state is visible under its device sysfs
directory:

```sh
cat /sys/devices/.../power/runtime_status
cat /sys/devices/.../power/runtime_usage
cat /sys/devices/.../power/control
cat /sys/devices/.../power/autosuspend_delay_ms
```

Common values:

```text
runtime_status: active, suspended, suspending, resuming
power/control:  auto, on
```

Enable automatic runtime PM from userspace:

```sh
echo auto | sudo tee /sys/devices/.../power/control
```

Force the device to stay active:

```sh
echo on | sudo tee /sys/devices/.../power/control
```

These controls are useful for debugging, but production policy should come from
the subsystem, distribution, embedded product policy, or userspace power daemon.

## Debugging Runtime PM

Start with the device state:

```sh
cat /sys/devices/.../power/runtime_status
cat /sys/devices/.../power/runtime_usage
cat /sys/devices/.../power/control
```

Check whether callbacks are running:

```sh
dmesg | grep -i 'runtime pm'
```

Add temporary driver logs:

```c
dev_dbg(dev, "runtime suspend\n");
dev_dbg(dev, "runtime resume\n");
```

Enable dynamic debug for a driver:

```sh
echo 'file drivers/foo/demo.c +p' | sudo tee /sys/kernel/debug/dynamic_debug/control
```

Trace power events:

```sh
sudo trace-cmd record -e power sleep 5
sudo trace-cmd report
```

Trace exact callback paths with function graph tracing when needed:

```sh
cd /sys/kernel/tracing
echo 0 | sudo tee tracing_on
echo function_graph | sudo tee current_tracer
echo 'demo_runtime_*' | sudo tee set_graph_function
echo > trace
echo 1 | sudo tee tracing_on
# run the failing operation
echo 0 | sudo tee tracing_on
sudo cat trace
```

## Common Runtime PM Bugs

| Bug | Symptom | Fix |
| --- | --- | --- |
| Missing get before register access | bus fault, all-ones reads, intermittent failure | bracket operation with `pm_runtime_resume_and_get()` |
| Usage count leak | device never suspends | match every successful get with a put |
| Put too early | work/IRQ touches powered-off hardware | keep reference until async work is done |
| Runtime PM enabled too early | callback sees uninitialized driver data | enable after resources and state are ready |
| Callback sleeps in atomic path | warnings or deadlock | call only sleepable helpers from sleepable paths |
| Runtime suspend leaves IRQ active | IRQ handler reads registers after clock off | mask device IRQ or keep needed resources active |
| Resume forgets state restore | device works before idle but fails after idle | cache and restore volatile configuration |
| Parent not active | child callback cannot access bus | use bus/subsystem PM model correctly |
| Autosuspend delay too short | high latency or power churn | tune delay from workload measurements |
| Wake IRQ confused with runtime IRQ | false wakeups or missed wake | separate runtime interrupt handling from wake policy |

## Practice Exercises

1. Pick a driver that already supports runtime PM. Identify where probe calls
   `pm_runtime_enable()` and where operations call runtime PM get/put helpers.
2. Find the driver's runtime suspend callback. List every hardware dependency it
   disables.
3. Force `power/control` to `on`, run a workload, then force it to `auto` and
   compare `runtime_status` behavior.
4. Add temporary debug logs to runtime suspend/resume and verify that one user
   operation does not cause unexpected repeated power cycles.
5. Audit an operation path that queues work. Decide whether the PM reference
   must be held until the work finishes.

## Driver Review Checklist

- Does every hardware access happen while the device is runtime active?
- Are all successful runtime PM gets matched by puts?
- Can runtime suspend race with IRQ, workqueue, timer, or DMA paths?
- Are clocks, regulators, resets, and pinctrl states restored in the right order?
- Is volatile register state cached before power is removed?
- Is autosuspend delay justified by workload behavior?
- Does remove disable runtime PM only after users and async work are stopped?
- Do system sleep callbacks intentionally reuse or differ from runtime PM?

## Related Topics

- [Suspend And Resume](suspend-resume.md)
- [Regulator And Clock Power Dependencies](regulator-clock-power-dependencies.md)
- [Power Domains](power-domains.md)

## Official References

- [Runtime PM](https://docs.kernel.org/power/runtime_pm.html)
- [Device Power Management Basics](https://docs.kernel.org/driver-api/pm/devices.html)

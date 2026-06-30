---
status: draft
reviewed: false
domain: linux-kernel
difficulty: advanced
last_reviewed: null
---

# Suspend And Resume

## What Problem Does This Solve?

System suspend and resume let the whole machine enter a low-power state and
later continue running. Device drivers participate by quiescing hardware before
sleep and restoring it after wake.

A driver that ignores system sleep may appear correct during normal runtime but
fail when the system suspends:

```text
before suspend:
  device works

during suspend:
  DMA still active
  IRQ still firing
  volatile registers lost
  wake signal not configured

after resume:
  device missing, wedged, noisy, or reset
```

Suspend/resume support is therefore a driver lifecycle problem. The driver must
make the hardware safe for system sleep and then rebuild enough state to resume
normal operation.

## System Sleep States

Linux supports several system sleep states. The exact set depends on platform
support.

Common user-visible states:

```sh
cat /sys/power/state
```

Typical values:

| State | Meaning |
| --- | --- |
| `freeze` | Suspend-to-idle. The kernel freezes tasks and idles devices, but platform firmware may not enter a deep hardware sleep state. |
| `standby` | A shallow platform suspend state, if supported. |
| `mem` | Suspend-to-RAM or suspend-to-idle depending on `/sys/power/mem_sleep`. |
| `disk` | Hibernation. Memory image is saved to storage and restored on boot. |

Memory sleep variants:

```sh
cat /sys/power/mem_sleep
```

Typical values:

| Variant | Meaning |
| --- | --- |
| `s2idle` | Generic suspend-to-idle path; no special firmware state is required. |
| `shallow` | Platform-specific shallow suspend. |
| `deep` | Platform-specific deeper suspend-to-RAM, if available. |

Driver callbacks should not assume that every platform supports every state.
They should implement the device transition required by the PM core and
subsystem.

## Callback Phases

System sleep is split into phases. The PM core walks the device hierarchy and
calls driver, bus, class, and PM domain callbacks in an ordered way.

The common suspend direction is:

```text
prepare
  -> suspend
     -> suspend_late
        -> suspend_noirq
           -> platform sleep entry
```

The resume direction reverses it:

```text
platform wake
  -> resume_noirq
     -> resume_early
        -> resume
           -> complete
```

Most device drivers only need `.suspend` and `.resume`, or subsystem-specific
helpers. Late and noirq callbacks are for stricter ordering requirements.

| Phase | Typical Use |
| --- | --- |
| `prepare` | reject suspend if the device cannot sleep, prepare children, stop new enumeration |
| `suspend` | stop I/O, flush work, save state, put hardware into low power |
| `suspend_late` | actions that must happen after children or most peers are suspended |
| `suspend_noirq` | final actions after normal device interrupts are disabled |
| `resume_noirq` | earliest restore needed before interrupts resume |
| `resume_early` | undo late suspend work |
| `resume` | restore normal hardware operation |
| `complete` | cleanup after the system is fully resuming |

Do not use later phases to hide ordinary ordering mistakes. If normal
`.suspend` can quiesce the device, keep the driver simple.

## Basic Driver Shape

Simple callbacks:

```c
static int demo_suspend(struct device *dev)
{
    struct demo_priv *priv = dev_get_drvdata(dev);

    demo_stop_users(priv);
    cancel_work_sync(&priv->work);
    demo_mask_irqs(priv);
    demo_save_state(priv);

    clk_disable_unprepare(priv->clk);

    return 0;
}

static int demo_resume(struct device *dev)
{
    struct demo_priv *priv = dev_get_drvdata(dev);
    int ret;

    ret = clk_prepare_enable(priv->clk);
    if (ret)
        return ret;

    ret = demo_restore_state(priv);
    if (ret) {
        clk_disable_unprepare(priv->clk);
        return ret;
    }

    demo_unmask_irqs(priv);
    demo_restart_users(priv);

    return 0;
}

static const struct dev_pm_ops demo_pm_ops = {
    .suspend = demo_suspend,
    .resume = demo_resume,
};
```

Platform driver registration:

```c
static struct platform_driver demo_driver = {
    .probe = demo_probe,
    .remove_new = demo_remove,
    .driver = {
        .name = "demo",
        .pm = &demo_pm_ops,
        .of_match_table = demo_of_match,
    },
};
```

The actual callbacks may be provided through a subsystem macro or helper. Follow
the subsystem's established pattern when one exists.

## What Suspend Must Do

A driver's suspend callback should make these questions boring:

```text
Can new I/O start?        no
Is existing I/O finished? yes, or safely aborted
Can DMA continue?         only if explicitly supported
Can IRQ handlers run?     only paths that are safe in suspend
Is wake configured?       yes, if policy enables it
Is volatile state saved?  yes, if it will be lost
Can power be removed?     yes, after dependencies are handled
```

Common suspend sequence:

```text
block new operations
wait for in-flight operations
cancel delayed work and timers
stop DMA
mask device interrupts
save volatile state
configure wakeup if allowed
select sleep pin state if needed
disable clocks/regulators or delegate to runtime PM
```

Example:

```c
static int demo_suspend(struct device *dev)
{
    struct demo_priv *priv = dev_get_drvdata(dev);
    int ret;

    mutex_lock(&priv->lock);
    priv->suspended = true;
    mutex_unlock(&priv->lock);

    cancel_delayed_work_sync(&priv->poll_work);

    ret = demo_wait_for_idle(priv);
    if (ret)
        return ret;

    demo_save_registers(priv);
    demo_mask_device_irqs(priv);

    if (device_may_wakeup(dev))
        enable_irq_wake(priv->irq);

    pinctrl_pm_select_sleep_state(dev);

    return demo_hw_power_off(priv);
}
```

The callback should return an error if the device cannot be safely suspended.
The PM core will abort the suspend attempt and resume devices that were already
suspended.

## What Resume Must Do

Resume reverses suspend, but it is not always a simple mirror. Hardware may have
lost state, firmware may have touched devices, and interrupts may already be
pending.

Common resume sequence:

```text
power resources on
select default pin state
restore clocks and resets
restore registers
ack stale status
disable wake-only configuration
unmask interrupts
allow new operations
restart deferred work
```

Example:

```c
static int demo_resume(struct device *dev)
{
    struct demo_priv *priv = dev_get_drvdata(dev);
    int ret;

    ret = demo_hw_power_on(priv);
    if (ret)
        return ret;

    pinctrl_pm_select_default_state(dev);

    ret = demo_restore_registers(priv);
    if (ret)
        return ret;

    if (device_may_wakeup(dev))
        disable_irq_wake(priv->irq);

    demo_ack_pending_status(priv);
    demo_unmask_device_irqs(priv);

    mutex_lock(&priv->lock);
    priv->suspended = false;
    mutex_unlock(&priv->lock);

    queue_delayed_work(system_wq, &priv->poll_work, 0);

    return 0;
}
```

If resume fails, the system may already be awake. Return the error so the PM
core and logs record the failure, but also leave the device in the safest state
the driver can manage.

## Reusing Runtime PM

If runtime suspend already powers the device down correctly, system sleep can
often reuse it:

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

Use this when:

- runtime suspend and system suspend use the same low-power hardware state
- runtime resume restores all state needed after system sleep
- wakeup handling is absent or handled elsewhere
- the subsystem pattern recommends it

Do not use it blindly when:

- the device must remain partially powered as a wake source
- system sleep needs a different pinctrl state
- hibernation needs additional save/restore behavior
- firmware owns the device during sleep
- the runtime path assumes userspace or subsystem services that are unavailable
  during system suspend

You can wrap the helper:

```c
static int demo_suspend(struct device *dev)
{
    struct demo_priv *priv = dev_get_drvdata(dev);

    if (device_may_wakeup(dev))
        enable_irq_wake(priv->irq);

    return pm_runtime_force_suspend(dev);
}
```

Be careful with ordering. If the wake IRQ requires clocks or power that runtime
suspend disables, configure the hardware differently.

## Wakeup Policy

System suspend is closely tied to wakeup policy. A device may be physically
capable of waking the system, but that does not mean wakeup should always be
enabled.

Typical pattern:

```c
if (device_may_wakeup(dev))
    enable_irq_wake(priv->irq);
```

`device_may_wakeup(dev)` checks whether wakeup is currently enabled for the
device, not merely whether the hardware is capable. See
[Wake Sources](wake-sources.md) for the full model.

## Interrupt Handling During Suspend

Normal interrupt handlers should not be expected to run during late suspend or
noirq phases. If an interrupt can happen while the device is being suspended,
the driver must decide whether to:

- mask the device interrupt before clocks are disabled
- synchronize the IRQ handler with `synchronize_irq()`
- leave a wake IRQ enabled but avoid normal register access
- acknowledge stale status before entering sleep
- move final work into `suspend_noirq` only when needed

Example:

```c
disable_irq(priv->irq);
demo_mask_device_irqs(priv);
synchronize_irq(priv->irq);
```

For wakeup-capable devices, disabling the Linux IRQ and enabling wake on the
same IRQ line must be designed carefully. Some drivers use wake IRQ helpers;
some use explicit `enable_irq_wake()` in suspend and `disable_irq_wake()` in
resume.

## DMA And In-Flight I/O

DMA must be stopped or placed into a documented low-power state before suspend.

Unsafe:

```text
suspend callback disables clock
  -> DMA engine still owns descriptors
  -> memory writes continue or bus transaction hangs
```

Safer:

```c
demo_stop_queue(priv);
ret = demo_wait_for_dma_idle(priv);
if (ret)
    return ret;

dmaengine_terminate_sync(priv->rx_chan);
demo_save_dma_registers(priv);
```

Subsystems such as block, networking, audio, and media often define how traffic
is quiesced. Use those subsystem mechanisms instead of inventing independent
policy in the driver.

## Workqueues, Timers, And Polling

Delayed work and timers are a common source of suspend failures.

Bad sequence:

```text
suspend disables clock
delayed work fires
work reads register
bus fault
```

Suspend should cancel or flush work that touches the device:

```c
cancel_delayed_work_sync(&priv->poll_work);
```

Resume can restart it:

```c
schedule_delayed_work(&priv->poll_work, msecs_to_jiffies(1000));
```

If work must run during suspend for wake support, it must avoid normal hardware
access unless the required power resources are guaranteed to remain on.

## Pinctrl Sleep State

Many SoC devices need different pin states during sleep:

```dts
pinctrl-names = "default", "sleep";
pinctrl-0 = <&uart3_default_pins>;
pinctrl-1 = <&uart3_sleep_pins>;
```

Suspend:

```c
pinctrl_pm_select_sleep_state(dev);
```

Resume:

```c
pinctrl_pm_select_default_state(dev);
```

Sleep state may:

- reduce leakage
- avoid driving external devices
- keep wake pins configured
- switch pins away from a powered-off peripheral

Do not select a sleep state that prevents the configured wake source from
reaching the interrupt controller.

## Noirq Callbacks

Noirq callbacks run after normal device interrupts have been disabled on the
suspend path and before they are re-enabled on the resume path. They are useful
for final hardware operations that must happen when interrupt handlers can no
longer race.

Example:

```c
static int demo_suspend_noirq(struct device *dev)
{
    struct demo_priv *priv = dev_get_drvdata(dev);

    if (demo_status_pending(priv))
        return -EBUSY;

    return 0;
}

static const struct dev_pm_ops demo_pm_ops = {
    .suspend = demo_suspend,
    .resume = demo_resume,
    .suspend_noirq = demo_suspend_noirq,
};
```

Use noirq callbacks sparingly:

- do not perform slow optional work there
- do not rely on normal interrupt completion
- avoid bus transfers if the bus or controller may already be suspended
- keep the code small enough to reason about during failure analysis

## Hibernation

Hibernation (`disk`) differs from suspend-to-RAM because the system image is
written to storage and restored later. Some drivers need freeze/thaw or
poweroff/restore callbacks in addition to suspend/resume callbacks.

Conceptual flow:

```text
freeze devices
write memory image
power off
boot again
restore image
restore devices
```

If a device has firmware state, DMA mappings, security state, or hardware
contexts that do not survive hibernation, check subsystem expectations before
assuming normal suspend/resume callbacks are enough.

## Direct Complete

Some devices can skip suspend/resume callbacks when they are already runtime
suspended and do not need extra system sleep work. The PM core and subsystem may
use a direct-complete optimization.

Driver implication:

```text
If runtime suspended state is valid for system sleep,
  -> fewer callbacks may run
  -> runtime resume must still restore complete state when device is used later
```

Do not depend on suspend callbacks for state that is also required after runtime
suspend. Put common state restoration in runtime resume or a shared helper.

## Failure And Rollback

Suspend callbacks can fail:

```c
static int demo_suspend(struct device *dev)
{
    struct demo_priv *priv = dev_get_drvdata(dev);
    int ret;

    ret = demo_wait_for_idle(priv);
    if (ret)
        return ret;

    ret = demo_enter_low_power(priv);
    if (ret)
        return ret;

    return 0;
}
```

If one device fails suspend, the PM core resumes devices already suspended. The
driver should still clean up any partial transition before returning an error
when it can do so safely.

Example:

```c
ret = demo_enter_low_power(priv);
if (ret) {
    demo_unmask_device_irqs(priv);
    priv->suspended = false;
    return ret;
}
```

## Testing Suspend And Resume

List states:

```sh
cat /sys/power/state
cat /sys/power/mem_sleep
```

Suspend to idle:

```sh
echo freeze | sudo tee /sys/power/state
```

Suspend using the selected `mem` mode:

```sh
echo mem | sudo tee /sys/power/state
```

Use RTC wake for repeatable tests:

```sh
sudo rtcwake -m mem -s 10
```

Test only device suspend/resume callbacks without entering the deepest platform
state:

```sh
cat /sys/power/pm_test
echo devices | sudo tee /sys/power/pm_test
echo mem | sudo tee /sys/power/state
echo none | sudo tee /sys/power/pm_test
```

The exact `pm_test` modes depend on kernel configuration and platform support.

## Common Suspend/Resume Bugs

| Bug | Symptom | Fix |
| --- | --- | --- |
| New I/O not blocked | suspend races with userspace or subsystem operation | add suspend state and subsystem quiesce |
| Work not canceled | register access after clocks off | cancel or flush work before power-down |
| DMA left active | hang, corruption, IOMMU fault | stop queues and terminate DMA safely |
| Wake IRQ not configured | system never wakes | use wakeup policy and wake IRQ setup |
| Wake IRQ left asserted | immediate resume | ack/mask status before sleep |
| Volatile registers not restored | device broken after resume | cache and restore configuration |
| Pinctrl sleep state wrong | no wake or leakage | define correct `sleep` state |
| Runtime PM state ignored | callbacks double-disable resources | coordinate system sleep with runtime PM |
| noirq callback does too much | suspend hang late in path | move ordinary work to normal suspend |

## Practice Exercises

1. Pick a driver with `struct dev_pm_ops`. Identify which system sleep phases it
   implements and why.
2. Find one suspend callback and classify each line as blocking I/O, stopping
   async work, saving state, configuring wake, or powering down hardware.
3. Run `pm_test=devices` on a lab system and compare the log with a real
   suspend attempt.
4. For a wake-capable device, verify that `/sys/devices/.../power/wakeup`
   changes whether the driver arms wake in suspend.
5. Review a runtime PM capable driver and decide whether `pm_runtime_force_suspend()`
   is appropriate for its system sleep path.

## Review Checklist

- Are new operations blocked before hardware is powered down?
- Are in-flight transfers, DMA, workqueues, timers, and IRQ handlers quiesced?
- Is wakeup configured only when policy enables it?
- Are volatile registers and firmware-visible state restored?
- Are pinctrl, clock, regulator, reset, and power-domain transitions ordered?
- Are runtime PM and system sleep paths coordinated?
- Is noirq code minimal and justified?
- Can suspend failure return an error without leaving the device half suspended?

## Related Topics

- [Runtime PM](runtime-pm.md)
- [Wake Sources](wake-sources.md)
- [Suspend And Resume Debugging](suspend-resume-debugging.md)

## Official References

- [Device Power Management Basics](https://docs.kernel.org/driver-api/pm/devices.html)
- [System Sleep States](https://docs.kernel.org/admin-guide/pm/sleep-states.html)
- [Runtime PM](https://docs.kernel.org/power/runtime_pm.html)

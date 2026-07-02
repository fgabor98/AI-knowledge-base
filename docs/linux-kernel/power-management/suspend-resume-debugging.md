---
status: draft
reviewed: false
domain: linux-kernel
difficulty: advanced
last_reviewed: null
---

# Suspend And Resume Debugging

## What Problem Does This Solve?

Suspend/resume failures often look similar from the outside:

```text
system hangs
system wakes immediately
device disappears after resume
screen stays black
network is dead
watchdog resets the board
power consumption stays high
```

The first debugging job is to classify the failure by phase. A device that
prevents suspend entry is different from a device that wakes the system
immediately, and both are different from a driver that fails to restore state
after resume.

This page gives a practical workflow for collecting evidence and narrowing the
problem.

## Classify The Failure

Use this map first:

| Failure Class | Typical Symptom | First Evidence |
| --- | --- | --- |
| Suspend entry failure | `echo mem` returns error or logs name a failing device | `dmesg`, PM callback return code |
| Suspend hang | console stops before entering sleep | last PM log, ftrace, `pm_test` |
| Immediate wake | system resumes right away | wakeup sources, IRQ counters |
| Missed wake | expected button/RTC/network wake does nothing | wake policy, IRQ wake, pinctrl, domain state |
| Resume hang | system wakes but stops during resume | last PM log, ftrace, persistent logs |
| Device broken after resume | system runs but one device fails | driver logs, runtime PM state, register restore |
| High suspend or idle power | suspend works but drains power | wakeup sources, runtime PM, clocks/regulators/domains |
| Watchdog reset | board reboots during suspend/resume | persistent logs, watchdog reason, narrow phase |

Do not start by changing code. First determine which phase fails.

## Establish A Baseline

Record what the system supports:

```sh
cat /sys/power/state
cat /sys/power/mem_sleep
cat /sys/power/pm_test
uname -a
```

Record kernel logs:

```sh
dmesg -T > /tmp/dmesg.before
```

Record wake and interrupt state:

```sh
cat /proc/interrupts > /tmp/interrupts.before
sudo cat /sys/kernel/debug/wakeup_sources > /tmp/wakeup_sources.before
```

Record runtime PM state for the suspected device:

```sh
cat /sys/devices/.../power/runtime_status
cat /sys/devices/.../power/runtime_usage
cat /sys/devices/.../power/control
cat /sys/devices/.../power/wakeup
```

Record provider state if debugfs is available:

```sh
sudo cat /sys/kernel/debug/clk/clk_summary > /tmp/clk.before
sudo cat /sys/kernel/debug/regulator/regulator_summary > /tmp/regulator.before
sudo cat /sys/kernel/debug/pm_genpd/pm_genpd_summary > /tmp/genpd.before
```

## Use A Controlled Suspend Command

For manual tests:

```sh
echo mem | sudo tee /sys/power/state
```

For repeatable timed wake:

```sh
sudo rtcwake -m mem -s 10
```

For suspend-to-idle:

```sh
sudo rtcwake -m freeze -s 10
```

If `mem` can mean multiple platform modes, inspect and select:

```sh
cat /sys/power/mem_sleep
echo s2idle | sudo tee /sys/power/mem_sleep
echo deep | sudo tee /sys/power/mem_sleep
```

Use absolute test notes:

```text
kernel:
sleep state:
mem_sleep:
command:
expected wake source:
actual result:
```

This avoids mixing evidence from different sleep states.

## Use `pm_test` To Narrow The Phase

`/sys/power/pm_test` lets the kernel run parts of the suspend path and then wake
automatically without entering the final sleep state.

Inspect modes:

```sh
cat /sys/power/pm_test
```

Common modes:

| Mode | Meaning |
| --- | --- |
| `freezer` | test freezing userspace |
| `devices` | test device suspend/resume |
| `platform` | test platform suspend callbacks |
| `processors` | test CPU hotplug/processor stage where applicable |
| `core` | test core suspend stages |
| `none` | normal suspend |

Example:

```sh
echo devices | sudo tee /sys/power/pm_test
echo mem | sudo tee /sys/power/state
echo none | sudo tee /sys/power/pm_test
```

Interpretation:

```text
pm_test=freezer fails
  -> userspace freezing problem

pm_test=devices fails
  -> device suspend/resume problem

pm_test=devices passes but real suspend fails
  -> platform sleep, wake, firmware, CPU, or late-stage issue
```

Always reset `pm_test` to `none` after testing.

## Increase PM Logging

Kernel command-line options that are useful in labs:

```text
no_console_suspend
initcall_debug
ignore_loglevel
```

`no_console_suspend` keeps console output active longer during suspend. It can
change timing and power behavior, so use it for debugging rather than final
validation.

Runtime controls, when available:

```sh
cat /sys/power/pm_debug_messages
echo 1 | sudo tee /sys/power/pm_debug_messages
```

Driver dynamic debug:

```sh
echo 'file drivers/foo/demo.c +p' | sudo tee /sys/kernel/debug/dynamic_debug/control
```

Add temporary targeted logs in callbacks:

```c
dev_dbg(dev, "suspend: runtime=%s wake=%d\n",
        pm_runtime_status_suspended(dev) ? "suspended" : "active",
        device_may_wakeup(dev));
```

Keep logs specific. A few precise logs around transitions are better than a
large stream that changes timing.

## Trace Suspend And Resume

Trace power, IRQ, timer, and workqueue events:

```sh
sudo trace-cmd record -e power -e irq -e timer -e workqueue \
    rtcwake -m mem -s 10
sudo trace-cmd report > /tmp/suspend.trace
```

If `trace-cmd` is unavailable, use tracefs directly:

```sh
cd /sys/kernel/tracing
echo 0 | sudo tee tracing_on
echo nop | sudo tee current_tracer
echo > trace
echo 1 | sudo tee events/power/enable
echo 1 | sudo tee events/irq/enable
echo 1 | sudo tee tracing_on
sudo rtcwake -m mem -s 10
echo 0 | sudo tee tracing_on
sudo cat trace > /tmp/trace.txt
```

Function graph tracing for one driver:

```sh
cd /sys/kernel/tracing
echo 0 | sudo tee tracing_on
echo function_graph | sudo tee current_tracer
echo 'demo_*' | sudo tee set_graph_function
echo > trace
echo 1 | sudo tee tracing_on
sudo rtcwake -m mem -s 10
echo 0 | sudo tee tracing_on
sudo cat trace > /tmp/demo.trace
```

Use filters. Full function graph tracing across the kernel can be too large and
can perturb timing.

## Debug Suspend Entry Failures

Symptoms:

```text
echo mem returns quickly
dmesg shows "PM: Device ... failed to suspend"
suspend aborts and devices resume
```

Workflow:

1. Find the failing device in `dmesg`.
2. Identify the callback phase.
3. Check the return code.
4. Inspect whether the driver still had active I/O, DMA, work, or wakeup source.
5. Use `pm_test=devices` to reproduce without entering platform sleep.

Commands:

```sh
dmesg -T | grep -Ei 'PM:|suspend|resume|failed|error'
```

Common causes:

| Cause | Evidence | Fix |
| --- | --- | --- |
| active DMA | timeout waiting for idle | stop queues and terminate DMA |
| busy hardware | callback returns `-EBUSY` | block new work and wait/abort safely |
| runtime PM mismatch | double-disable or access while suspended | coordinate runtime/system PM |
| wake source active | suspend abort or immediate reattempt | finish event handling before suspend |
| child still active | parent cannot suspend | fix dependency or usage count |

## Debug Suspend Hangs

Symptoms:

```text
console stops during suspend
system does not enter sleep or wake
power button may not recover
```

Workflow:

1. Enable persistent logging if available.
2. Use `pm_test` to find the last passing phase.
3. Enable PM debug messages and no-console-suspend in a lab.
4. Trace the suspected phase.
5. Disable or unbind suspected drivers one at a time to confirm scope.

Useful boot arguments:

```text
no_console_suspend ignore_loglevel initcall_debug
```

Persistent logs depend on platform:

```sh
journalctl -k -b -1
sudo dmesg -T
```

Embedded systems may need:

- serial console
- pstore/ramoops
- vendor crash log
- watchdog reset reason register
- JTAG or hardware trace

Common causes:

| Cause | Evidence | Fix |
| --- | --- | --- |
| callback waits forever | trace stops in driver wait | add timeout and fix hardware condition |
| IRQ needed after disabled | no completion in noirq phase | move operation earlier or avoid IRQ-dependent wait |
| bus suspended before child transfer | I2C/SPI access hangs late | fix PM ordering or avoid late bus I/O |
| firmware call never returns | trace stops at platform call | platform/firmware debug |
| console masks timing | only fails without debug args | use tracing/persistent logs |

## Debug Immediate Wakeups

Symptoms:

```text
system enters suspend
system resumes immediately
rtcwake returns almost at once
```

Record before and after:

```sh
cat /proc/interrupts > /tmp/irqs.before
sudo cat /sys/kernel/debug/wakeup_sources > /tmp/wakeup.before

sudo rtcwake -m mem -s 30

cat /proc/interrupts > /tmp/irqs.after
sudo cat /sys/kernel/debug/wakeup_sources > /tmp/wakeup.after

diff -u /tmp/wakeup.before /tmp/wakeup.after
```

Check wakeup policy:

```sh
find /sys/devices -path '*/power/wakeup' -print -exec cat {} \;
```

Temporarily disable one wake source in a lab:

```sh
echo disabled | sudo tee /sys/devices/.../power/wakeup
```

Common causes:

| Cause | Evidence | Fix |
| --- | --- | --- |
| level IRQ still asserted | IRQ count rises, wake source count rises | clear device status before suspend |
| wrong IRQ trigger | repeated wake or no wake | fix firmware interrupt flags |
| wake enabled unexpectedly | sysfs policy ignored | gate with `device_may_wakeup()` |
| noisy GPIO | wake count without real event | bias/debounce/hardware fix |
| RTC alarm already expired | RTC wakes immediately | clear/reprogram alarm |
| shared IRQ confusion | wrong device blamed | inspect all status registers on shared line |

## Debug Missed Wakeups

Symptoms:

```text
system suspends
expected button, GPIO, RTC, network, or USB event does not wake it
```

Checklist:

- Does `/sys/devices/.../power/wakeup` exist?
- Is it `enabled`?
- Does the driver call `device_init_wakeup()` or equivalent capability setup?
- Does suspend arm the wake IRQ or wake helper?
- Is the IRQ trigger type correct?
- Is the wake pin configured in the `sleep` pinctrl state?
- Is the GPIO controller or interrupt controller wake-capable?
- Is the power domain or rail for the wake path still on?
- Does platform firmware require separate wake configuration?

Commands:

```sh
cat /sys/devices/.../power/wakeup
cat /proc/interrupts
sudo cat /sys/kernel/debug/wakeup_sources
sudo cat /sys/kernel/debug/pinctrl/*/pinmux-pins
sudo cat /sys/kernel/debug/pm_genpd/pm_genpd_summary
```

Hardware tools help here. If the wake signal never toggles at the SoC pin, the
kernel cannot fix it. If it toggles but the interrupt controller does not wake,
focus on pinctrl, IRQ type, GPIO controller wake support, and domain state.

## Debug Resume Failures

Symptoms:

```text
system wakes
resume log stops part-way
some devices never recover
watchdog resets during resume
```

Workflow:

1. Identify the last device or callback in the resume log.
2. Compare suspend and resume ordering.
3. Check whether the device was runtime suspended before system sleep.
4. Confirm clocks, regulators, resets, and domains are restored before register
   access.
5. Confirm volatile registers are restored.
6. Check whether an IRQ fires before the driver is ready.

Common causes:

| Cause | Evidence | Fix |
| --- | --- | --- |
| register access before power restored | bus fault or timeout | reorder resume sequence |
| volatile state lost | device responds but misconfigured | cache and restore registers |
| IRQ unmasked too early | handler runs before state restored | ack/mask until ready |
| runtime PM state inconsistent | device remains suspended after resume | use `pm_runtime_force_resume()` or fix state |
| reset not handled | device stuck after power cycle | assert/deassert reset in resume |
| firmware changed state | driver assumptions invalid | reinitialize hardware fully |

## Debug A Device Broken After Resume

If the system resumes but one device is dead:

```sh
dmesg -T | grep -i demo
cat /sys/devices/.../power/runtime_status
cat /sys/devices/.../power/runtime_usage
```

Force runtime active for testing:

```sh
echo on | sudo tee /sys/devices/.../power/control
```

Then test the device again. If forcing runtime active helps, the bug is probably
in runtime PM or system/runtime PM interaction.

Compare provider state:

```sh
sudo cat /sys/kernel/debug/clk/clk_summary
sudo cat /sys/kernel/debug/regulator/regulator_summary
sudo cat /sys/kernel/debug/pm_genpd/pm_genpd_summary
```

Driver-specific checks:

- Were cached registers restored?
- Was reset deasserted?
- Was firmware reloaded?
- Were DMA descriptors rebuilt?
- Was the IRQ re-enabled?
- Were subsystem queues restarted?
- Did userspace need reinitialization?

## Debug High Power After Suspend Or Idle

High power after resume or during idle usually means something is still active.

Check runtime PM:

```sh
find /sys/devices -path '*/power/runtime_status' -print -exec cat {} \; |
    grep -B1 active
```

Check wakeup sources:

```sh
sudo cat /sys/kernel/debug/wakeup_sources
```

Check clocks, regulators, and domains:

```sh
sudo cat /sys/kernel/debug/clk/clk_summary
sudo cat /sys/kernel/debug/regulator/regulator_summary
sudo cat /sys/kernel/debug/pm_genpd/pm_genpd_summary
```

Check interrupts:

```sh
watch -n1 cat /proc/interrupts
```

Common causes:

- runtime PM usage count leak
- autosuspend disabled or `power/control` forced to `on`
- wakeup source stays active
- clock prepare/enable count leak
- regulator enable count leak
- child device prevents parent domain from powering off
- frequent timer or polling work prevents CPU idle

## Bisect By Device

When the log does not clearly identify the culprit, isolate.

Options:

- disable a device in Device Tree for a lab build
- blacklist a module
- unbind a driver from sysfs
- disable wakeup for one device
- force runtime PM policy to `on` or `auto`
- compare suspend with and without a peripheral connected

Unbind example:

```sh
echo 10030000.serial | sudo tee /sys/bus/platform/drivers/demo/unbind
```

Rebind:

```sh
echo 10030000.serial | sudo tee /sys/bus/platform/drivers/demo/bind
```

Unbinding can disrupt userspace and is not safe for every device. Do it on a
lab system where losing that device is acceptable.

## `pm_trace`

Some kernels expose:

```sh
cat /sys/power/pm_trace
```

`pm_trace` can help identify the last device touched before a resume failure by
storing a hash in the RTC. Use it carefully: it can alter the system clock and
is less useful than direct logs or tracing when those are available.

Typical lab use:

```sh
echo 1 | sudo tee /sys/power/pm_trace
echo mem | sudo tee /sys/power/state
# after reboot/resume failure
dmesg | grep 'hash matches'
echo 0 | sudo tee /sys/power/pm_trace
```

Prefer persistent logs, ftrace, and `pm_test` when possible.

## Driver Instrumentation Pattern

Add temporary logs at transition boundaries:

```c
static int demo_suspend(struct device *dev)
{
    struct demo_priv *priv = dev_get_drvdata(dev);
    int ret;

    dev_dbg(dev, "suspend enter: wake=%d runtime_suspended=%d\n",
            device_may_wakeup(dev),
            pm_runtime_status_suspended(dev));

    ret = demo_wait_for_idle(priv);
    if (ret) {
        dev_dbg(dev, "suspend idle wait failed: %d\n", ret);
        return ret;
    }

    ret = demo_enter_low_power(priv);
    dev_dbg(dev, "suspend exit: %d\n", ret);
    return ret;
}
```

Log:

- callback entry and exit
- return codes
- wake policy
- runtime PM state
- IRQ status
- key hardware status bits

Do not dump large register blocks in normal logs. Use targeted debugfs, trace,
or temporary diagnostics.

## Common Root Causes

| Root Cause | Shows Up As | Where To Look |
| --- | --- | --- |
| Runtime PM usage leak | high power, domain never off | `runtime_usage`, driver get/put paths |
| Missing runtime PM get | post-resume access failure | operation paths |
| Stale wake IRQ | immediate resume | wakeup sources, IRQ status |
| Wrong pinctrl sleep state | missed wake or leakage | pinctrl debugfs, Device Tree |
| Domain powered off incorrectly | missed wake, bus fault | genpd summary, wake path map |
| Clock disabled too early | suspend/resume timeout | callback order |
| Regulator ramp delay missing | intermittent resume | hardware timing |
| DMA not stopped | suspend hang or corruption | subsystem quiesce path |
| Workqueue not canceled | access after power-down | delayed work and timers |
| Firmware ownership conflict | platform suspend failure | firmware logs, provider driver |

## Practice Exercises

1. Run `pm_test=devices` and a real suspend on a lab system. Compare logs.
2. Pick a wake-capable device and verify immediate-wake evidence by comparing
   wakeup source counters before and after suspend.
3. Trace one driver's suspend and resume callbacks with function graph tracing.
4. Force a device's runtime PM policy to `on` and decide whether a resume bug
   changes behavior.
5. Map one failure to the phase table at the top of this page and list the next
   three commands you would run.

## Debugging Checklist

- Which sleep state and `mem_sleep` mode failed?
- Did the failure happen entering suspend, staying suspended, waking, resuming,
  or after resume?
- Does `pm_test` narrow the phase?
- Which device appears last in logs or trace?
- Are wakeup sources and IRQ counters consistent with the symptom?
- Is runtime PM state sane before suspend?
- Are clocks, regulators, resets, domains, and pinctrl states correct?
- Are DMA, workqueues, timers, and IRQ handlers quiesced?
- Can the failure be isolated by disabling one device or wake source?
- Is there persistent evidence after watchdog reset or reboot?

## Related Topics

- [Suspend And Resume](suspend-resume.md)
- [Runtime PM](runtime-pm.md)
- [Wake Sources](wake-sources.md)
- [Ftrace And Tracepoints](../debugging/ftrace-and-tracepoints.md)
- [Watchdog Reset Diagnosis](../debugging/watchdog-reset-diagnosis.md)

## Official References

- [Device Power Management Basics](https://docs.kernel.org/driver-api/pm/devices.html)
- [System Sleep States](https://docs.kernel.org/admin-guide/pm/sleep-states.html)
- [Runtime PM](https://docs.kernel.org/power/runtime_pm.html)

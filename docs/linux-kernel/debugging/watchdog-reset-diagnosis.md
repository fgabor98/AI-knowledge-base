---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# Watchdog Reset Diagnosis

## What Problem Does This Solve?

Watchdog resets often erase the immediate failure context unless the system captures reset reason, logs, and timing evidence.

A watchdog reset says:

```text
someone stopped feeding the watchdog long enough for reset
```

It does not say why. The root cause could be:

- CPU hard lockup
- kernel deadlock
- interrupt storm
- storage or bus transaction stuck
- userspace watchdog daemon stopped
- application health check failed
- suspend/resume handoff bug
- bootloader/kernel/userspace ownership gap
- timeout too short for a valid operation

## Core Concepts

- hardware watchdog
- reset reason
- pretimeout
- panic-on-watchdog policy
- persistent logs
- boot counter
- systemd watchdog
- hang classification

## Mental Model

A watchdog reset is a symptom. The investigation needs evidence from before and after reset, plus a timeline of who was responsible for feeding the watchdog.

```text
before reset:
  last logs, load, interrupts, watchdog owner

reset:
  hardware reset reason and watchdog timeout

after reset:
  bootloader/kernel records reason, previous logs, boot count
```

If you only inspect the new boot, you may miss the actual failure.

## First Questions

Ask:

```text
Which watchdog reset the system?
Who was supposed to feed it?
What was the timeout?
Was there a pretimeout?
What was the last log before reset?
Was the system in boot, runtime, suspend, shutdown, or update?
Was reset reason preserved?
Did userspace intentionally stop feeding due to health failure?
```

## Reset Reason Capture

Sources vary by platform:

- bootloader reset reason print
- SoC reset status register
- PMIC reset reason
- RTC scratch registers
- pstore/ramoops
- persistent journal
- watchdog driver status
- product boot counter

Capture early. Some registers are clear-on-read or cleared by bootloader/kernel initialization.

Example boot log target:

```text
reset reason: watchdog
previous boot: kernel panic
boot count: 17
```

If the platform does not preserve reset reason, add that to the product reliability backlog.

## Watchdog Ownership Timeline

Build a timeline:

```text
bootloader starts watchdog at T0
kernel enters at T1
watchdog driver probes at T2
initramfs starts at T3
userspace watchdog daemon starts at T4
application health checks start at T5
reset at T6
```

Find the gap:

```text
who was feeding between T2 and T4?
did timeout cover slow storage/rootfs discovery?
did the watchdog daemon start before the timeout?
```

## Runtime Inspection

On a running system:

```sh
ls -l /dev/watchdog*
find /sys/class/watchdog -maxdepth 2 -type f -print
cat /sys/class/watchdog/watchdog0/timeout
cat /sys/class/watchdog/watchdog0/nowayout
cat /sys/class/watchdog/watchdog0/status
```

Userspace owner:

```sh
fuser -v /dev/watchdog0
systemctl show -p RuntimeWatchdogUSec
systemctl status '*watchdog*'
```

Availability depends on init system and target configuration.

## Previous Logs

Check previous boot logs:

```sh
journalctl -k -b -1
journalctl -b -1
```

If unavailable, plan persistent logging:

- pstore/ramoops
- serial log capture
- persistent journal
- remote logging
- netconsole in lab

Watchdog bugs are hard to solve without pre-reset evidence.

## Classifying The Hang

| Class | Clues |
| --- | --- |
| CPU hard lockup | lockup detector messages, no scheduling, NMI watchdog |
| soft lockup | task stuck on CPU, soft lockup warnings |
| deadlock | blocked tasks, lockdep reports, no progress |
| interrupt storm | high IRQ counts, CPU busy in IRQ |
| userspace hang | kernel alive, watchdog daemon stopped feeding |
| application health failure | daemon intentionally stops feeding |
| boot handoff gap | reset during boot before daemon starts |
| suspend/reset issue | reset during suspend or resume |
| hardware bus stall | last log around I2C/SPI/MMIO/storage transaction |

Different classes need different tools.

## Pretimeout

Some watchdogs can fire a pretimeout event before reset.

Use it to:

- log last state
- trigger panic for pstore/kdump
- collect lockup information
- notify a management controller

Limitations:

- not every watchdog supports it
- handler may not run during hard lockup
- remaining time may be too short for heavy logging

Treat pretimeout evidence as best effort.

## Panic-On-Watchdog Policy

Some products prefer panic before reset so crash evidence is captured.

Policy questions:

- Is pstore or kdump configured?
- Is panic timeout shorter than watchdog reset?
- Does panic path have enough time to store logs?
- Does panic-on-lockup risk reboot loops?
- Is this enabled in production, CI, or only diagnostics?

Command-line/sysctl examples may include:

```text
panic=10
softlockup_panic=1
nmi_watchdog=1
```

Use product policy, not defaults guessed during debugging.

## Controlled Reproduction

In a lab:

```text
record timeout and owner
start log capture
trigger a controlled hang or stop feeding
wait for reset
capture bootloader reset reason
capture previous logs
verify recovery path
```

Do not perform uncontrolled watchdog tests on production devices.

## Debugging Tools By Suspected Class

| Suspected Cause | Useful Tools |
| --- | --- |
| lock deadlock | lockdep, blocked task logs, sysrq stacks |
| IRQ storm | `/proc/interrupts`, irq tracepoints |
| CPU hot loop | perf, ftrace |
| workqueue stuck | workqueue tracepoints, sysrq task dump |
| userspace daemon issue | service logs, process state |
| boot handoff | serial logs, boot timestamps |
| suspend issue | PM debug logs, wakeup sources |

SysRq task dump before reset can be valuable if the system still responds:

```sh
echo t | sudo tee /proc/sysrq-trigger
echo w | sudo tee /proc/sysrq-trigger
```

This must happen before the reset.

## Common Failure Modes

| Symptom | Likely Cause | First Checks |
| --- | --- | --- |
| reset reason says watchdog, no logs | no persistent capture | pstore/serial/journal |
| reset during boot | ownership handoff gap | boot timeline |
| reset during update | timeout too short or feeder stopped | update flow |
| reset during suspend | watchdog runs through suspend | PM policy |
| system hung but no reset | wrong watchdog active or feeding unconditional | active device |
| reboot loop | watchdog fires before recovery completes | timeout and initramfs |
| pretimeout not seen | unsupported or hard lockup | driver capability |

## Practice Exercises

### Exercise 1: Ownership Timeline

Document who feeds the watchdog during:

```text
bootloader
kernel boot
initramfs
normal userspace
suspend/resume
shutdown
firmware update
```

### Exercise 2: Reset Evidence Drill

In a lab, trigger a controlled watchdog reset and verify that reset reason and previous logs survive reboot.

### Exercise 3: Classify A Reset

Given previous logs and reset reason, classify whether the likely cause is CPU lockup, userspace feeder failure, application health failure, or boot handoff gap.

## Debugging Checklist

- Check reset reason registers early.
- Check whether bootloader cleared evidence.
- Check userspace watchdog owner.
- Separate CPU lockup, userspace hang, and hardware stall cases.
- Build a watchdog ownership timeline.
- Preserve previous boot logs.
- Confirm timeout, pretimeout, and nowayout policy.
- Test recovery and update flows under watchdog.
- Verify that the active watchdog is the intended watchdog.

## Related Topics

- [Watchdog Options](../configuration-and-platform-policy/watchdog-options.md)
- [Embedded Productization](../../embedded-productization/index.md)
- [Oops, Panic, And Crash Logs](oops-panic-crash-logs.md)
- [Suspend And Resume Debugging](../power-management/suspend-resume-debugging.md)

## Official References

- [The Linux Watchdog driver API](https://docs.kernel.org/watchdog/watchdog-api.html)
- [The kernel command-line parameters](https://docs.kernel.org/admin-guide/kernel-parameters.html)

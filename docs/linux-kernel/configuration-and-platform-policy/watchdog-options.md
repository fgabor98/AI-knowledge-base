---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# Watchdog Options

## What Problem Does This Solve?

Watchdog configuration determines how the system detects hangs and recovers during boot, runtime, suspend, and shutdown.

A watchdog can turn a hang into a reboot, but it can also create reboot loops, hide root causes, or reset during controlled debugging if ownership is unclear.

Watchdog policy spans:

- bootloader
- kernel watchdog driver
- watchdog core
- userspace watchdog daemon or init system
- application health checks
- panic/oops behavior
- reset-reason storage
- field diagnostics

## Core Concepts

- hardware watchdog
- watchdog core
- timeout
- pretimeout
- nowayout
- systemd watchdog
- panic watchdog behavior
- bootloader watchdog handoff

## Mental Model

Watchdog behavior is a system policy, not just a driver option. Kernel, bootloader, userspace, and field diagnostics must agree on ownership.

```text
bootloader may start watchdog
-> kernel must keep it fed or take ownership quickly
-> userspace service may take over /dev/watchdog
-> application health policy decides whether to feed
-> reset reason must survive reboot for diagnosis
```

An enabled watchdog without a feeding owner is a reset timer. A feeding owner without health checks can hide a dead product.

## Watchdog Roles

| Role | Responsibility |
| --- | --- |
| bootloader | optional early hang protection, handoff policy |
| kernel driver | exposes hardware watchdog through watchdog framework |
| kernel policy | timeout, nowayout, panic interaction |
| userspace daemon/init | feeds watchdog based on system health |
| product application | reports health to userspace watchdog policy |
| diagnostics | preserves and interprets reset reason |

Assign a single owner for each boot phase.

## Hardware Watchdog Versus Software Watchdog

Hardware watchdog:

- resets the board even if Linux is badly stuck, depending on hardware integration
- should be used for production recovery
- may be started by bootloader
- may continue through kernel handoff

Software watchdog:

- detects some kernel scheduling stalls
- cannot recover from all hardware/kernel lockups
- useful for development and some platforms
- not equivalent to an external or SoC watchdog

Production systems usually need a real hardware watchdog.

## Kernel Configuration Areas

Common areas to review:

```text
CONFIG_WATCHDOG
CONFIG_WATCHDOG_CORE
platform-specific watchdog driver
CONFIG_WATCHDOG_NOWAYOUT
soft lockup / hard lockup detector options
panic-on-lockup options
```

Exact symbols vary by architecture and kernel version. Check the final `.config`.

Example:

```sh
grep '^CONFIG_WATCHDOG' build/.config
grep 'WATCHDOG' /proc/config.gz
```

If `/proc/config.gz` is enabled on the target, it can help inspect runtime config.

## Runtime Interfaces

Common checks:

```sh
ls -l /dev/watchdog*
cat /sys/class/watchdog/watchdog0/status
cat /sys/class/watchdog/watchdog0/timeout
cat /sys/class/watchdog/watchdog0/nowayout
```

Availability depends on driver and kernel configuration.

Opening `/dev/watchdog` often starts the watchdog. Closing behavior depends on driver support, magic close, and `nowayout` policy.

## Timeout Policy

Timeout must be long enough for normal scheduling delays and updates, but short enough to recover from hangs.

Questions:

- What is the maximum expected boot time before userspace takes over?
- How long can storage operations block during normal operation?
- How long can firmware updates or filesystem repairs take?
- How quickly must the product recover?
- Is timeout different in manufacturing, service, and production?

Example policy:

```text
bootloader timeout: 60 seconds
kernel handoff timeout: 30 seconds
userspace runtime timeout: 20 seconds
application health deadline: 10 seconds
panic reboot delay: 10 seconds
```

These numbers must come from measurements and product requirements.

## Pretimeout Policy

Some watchdogs support pretimeout: an early warning before reset.

Possible use:

- collect last logs
- trigger panic for crash dump
- notify a management controller
- store reset reason

Risk:

- pretimeout handler may not run if the system is too stuck
- pretimeout work may make timing worse
- crash collection can exceed remaining time

Treat pretimeout as best-effort evidence, not guaranteed recovery.

## `nowayout`

`nowayout` means once the watchdog is started, it cannot be stopped until reboot.

Good for:

- production systems where watchdog must not be accidentally disabled
- safety or high-availability products
- catching userspace watchdog daemon crashes

Hard for:

- kernel debugging
- firmware update flows
- long manufacturing tests
- controlled lab experiments

Policy must say which profiles use nowayout:

```text
debug: nowayout disabled
manufacturing: product-specific
production: nowayout enabled
```

## Bootloader Handoff

If the bootloader starts the watchdog, the kernel must take over before timeout.

Questions:

- Does bootloader enable watchdog?
- Is watchdog still running when Linux starts?
- Does the kernel driver probe early enough?
- What feeds watchdog between kernel start and userspace?
- What happens if kernel boot hangs before driver probe?
- Is reset reason preserved by bootloader, PMIC, RTC, or SoC registers?

Test with delayed boot:

```text
bootloader delay
kernel boot delay
initramfs delay
userspace watchdog daemon delayed start
```

Do this only in a controlled lab.

## Userspace Ownership

Userspace can feed the watchdog through:

- a dedicated watchdog daemon
- init system watchdog features
- product supervisor
- service manager integration

Policy question:

```text
Does userspace feed unconditionally, or only when health checks pass?
```

Unconditional feeding detects only total userspace death. Health-based feeding can detect partial product failure, but must avoid false resets.

## systemd Watchdog Interaction

On systemd-based systems, distinguish:

- hardware watchdog feeding by systemd manager configuration
- per-service watchdog notifications
- product-level health checks

Do not assume enabling a service watchdog feeds the hardware watchdog. Review the distribution configuration and runtime behavior.

Useful checks:

```sh
systemctl show -p RuntimeWatchdogUSec
systemctl show your.service -p WatchdogUSec
```

## Panic And Watchdog

Panic policy and watchdog policy must agree.

Questions:

- Should kernel panic reboot immediately or after delay?
- Should watchdog reset instead of panic reboot?
- Can logs flush before reset?
- Is crash dump collection required?
- Does panic path pet or stop watchdog?

Example command-line choices:

```text
panic=10
oops=panic
softlockup_panic=1
```

Use panic-on-warning only when the warning rate is controlled, such as CI or strict service profiles.

## Suspend And Shutdown

Watchdog behavior across suspend and shutdown is platform-specific.

Questions:

- Does watchdog run during suspend?
- Should wakeup sources keep the system alive?
- Can userspace feed during suspend preparation?
- Does shutdown stop watchdog or let it reset if shutdown hangs?
- Is firmware update safe under watchdog?

Test suspend/resume loops with watchdog enabled before declaring production behavior.

## Reset Reason Evidence

A watchdog reset without reset-reason evidence is hard to diagnose.

Archive after reboot:

```text
bootloader reset reason
SoC reset status register
PMIC reset reason
pstore/ramoops logs
last kernel logs
watchdog daemon logs
application health logs
boot count
```

Make reset reason capture part of early boot before later code clears status registers.

## Common Failure Modes

| Symptom | Likely Cause | First Checks |
| --- | --- | --- |
| repeated reboot loop | watchdog enabled with no feeder | `/dev/watchdog`, userspace service |
| reset during boot | bootloader handoff gap | timeout and probe timing |
| watchdog cannot be stopped in lab | nowayout enabled | config and sysfs |
| hangs forever despite watchdog | wrong watchdog device or daemon feeding unconditionally | active device and health policy |
| reset reason lost | bootloader or kernel clears status early | early boot logs |
| update interrupted | timeout too short or feeder stopped | update flow |
| suspend reset | watchdog runs during suspend | PM policy |

## Practice Exercises

### Exercise 1: Watchdog Ownership Map

For one product, document:

```text
bootloader watchdog state
kernel driver
timeout
nowayout
userspace feeder
health checks
reset reason storage
debug exception path
```

### Exercise 2: Controlled Lab Reset

In a lab, start the watchdog, stop feeding it intentionally, and verify:

```text
reset happens
timeout matches policy
reset reason is preserved
logs are useful
system recovers
```

### Exercise 3: Handoff Test

Delay bootloader, kernel, and userspace handoff separately. Identify which phase owns feeding at each point.

## Debugging Checklist

- Check bootloader handoff.
- Check `/dev/watchdog*`.
- Check systemd watchdog settings.
- Preserve reset reason evidence after reboot.
- Check final `.config` for watchdog options.
- Check timeout and nowayout at runtime.
- Check whether watchdog daemon feeds unconditionally.
- Test suspend, shutdown, and update flows.
- Verify panic policy against watchdog timeout.

## Related Topics

- [Watchdog Reset Diagnosis](../debugging/watchdog-reset-diagnosis.md)
- [Embedded Productization](../../embedded-productization/index.md)
- [Kernel Command Line Policy](kernel-command-line-policy.md)
- [Suspend And Resume](../power-management/suspend-resume.md)

## Official References

- [The Linux Watchdog driver API](https://docs.kernel.org/watchdog/watchdog-api.html)
- [The kernel command-line parameters](https://docs.kernel.org/admin-guide/kernel-parameters.html)

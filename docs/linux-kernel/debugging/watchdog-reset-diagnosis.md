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

## Practice Skeleton

- Capture reset reason after reboot.
- Enable persistent kernel logs where available.
- Test pretimeout handling.
- Correlate watchdog timeout with system load.

## Debugging Checklist

- Check reset reason registers early.
- Check whether bootloader cleared evidence.
- Check userspace watchdog owner.
- Separate CPU lockup, userspace hang, and hardware stall cases.

## Related Topics

- [Watchdog Options](../configuration-and-platform-policy/watchdog-options.md)
- [Embedded Productization](../../embedded-productization/index.md)
- [Oops, Panic, And Crash Logs](oops-panic-crash-logs.md)

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

## Practice Skeleton

- Identify the active watchdog device.
- Configure timeout and nowayout policy.
- Test normal feeding.
- Test a controlled hang in a lab environment.

## Debugging Checklist

- Check bootloader handoff.
- Check `/dev/watchdog*`.
- Check systemd watchdog settings.
- Preserve reset reason evidence after reboot.

## Related Topics

- [Watchdog Reset Diagnosis](../debugging/watchdog-reset-diagnosis.md)
- [Embedded Productization](../../embedded-productization/index.md)
- [Kernel Command Line Policy](kernel-command-line-policy.md)

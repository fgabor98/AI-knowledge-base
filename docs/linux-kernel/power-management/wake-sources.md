---
status: draft
reviewed: false
domain: linux-kernel
difficulty: advanced
last_reviewed: null
---

# Wake Sources

## What Problem Does This Solve?

Wake source configuration controls which devices can resume the system from low-power states.

## Core Concepts

- wakeup-capable devices
- wake IRQ
- `wakeup-source`
- enable wake
- disable wake
- suspend states
- wake reason
- policy ownership

## Mental Model

Wake behavior is both hardware capability and product policy. A device should wake the system only when the product needs that behavior.

## Practice Skeleton

- Mark a device as wakeup capable.
- Enable and disable wake from userspace.
- Test wake from an interrupt source.
- Capture wake reason after resume.

## Debugging Checklist

- Check interrupt trigger and wake capability.
- Check power domain state.
- Check whether wake is enabled in sysfs.
- Check platform firmware constraints.

## Related Topics

- [Suspend And Resume](suspend-resume.md)
- [IRQ Handling](../driver-interfaces/irq-handling.md)
- [Device Tree](../../device-tree/index.md)

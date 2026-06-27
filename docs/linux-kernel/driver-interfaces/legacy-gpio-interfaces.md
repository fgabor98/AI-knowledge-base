---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# Legacy GPIO Interfaces

## What Problem Does This Solve?

Legacy GPIO APIs and sysfs interfaces appear in older drivers and examples, but new driver code should use descriptor-based APIs.

## Core Concepts

- global GPIO numbers
- legacy integer GPIO API
- `/sys/class/gpio`
- GPIO export
- GPIO descriptor API
- active-low semantics
- migration

## Mental Model

Legacy GPIO interfaces identify lines by global numbers. Descriptor APIs identify lines by role and let firmware data carry board-specific mapping and polarity.

## Practice Skeleton

- Inspect an old integer-GPIO example.
- Convert it to `gpiod_*` descriptor calls.
- Replace global numbering assumptions with named GPIO properties.
- Keep sysfs GPIO use limited to diagnostics or old systems.

## Debugging Checklist

- Check whether the kernel still enables sysfs GPIO.
- Avoid hard-coding global GPIO numbers.
- Let the descriptor API handle polarity.
- Prefer `gpiod_get_optional` for optional lines.

## Related Topics

- [GPIO Consumer API](gpio-consumer-api.md)
- [GPIO Controller Drivers](gpio-controller-drivers.md)
- [Device Tree Hardware Description](../fundamentals/device-tree-hardware-description.md)

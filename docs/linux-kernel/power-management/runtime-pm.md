---
status: draft
reviewed: false
domain: linux-kernel
difficulty: advanced
last_reviewed: null
---

# Runtime PM

## What Problem Does This Solve?

Runtime PM powers devices up and down while the system remains running.

## Core Concepts

- runtime suspend
- runtime resume
- usage count
- autosuspend
- active state
- suspended state
- parent dependencies
- subsystem integration

## Mental Model

Runtime PM is reference-counted device availability. Drivers must acquire power before touching hardware and release it when the device can idle.

## Practice Skeleton

- Enable runtime PM in probe.
- Add runtime suspend and resume callbacks.
- Use autosuspend for an idle device.
- Audit register access paths for power state.

## Debugging Checklist

- Check runtime PM status in sysfs.
- Check usage count leaks.
- Check register access while suspended.
- Check parent device power dependencies.

## Related Topics

- [Clocks](../driver-interfaces/clocks.md)
- [Regulators](../driver-interfaces/regulators.md)
- [Suspend And Resume](suspend-resume.md)

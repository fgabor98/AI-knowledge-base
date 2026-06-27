---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# Sysfs Attributes

## What Problem Does This Solve?

Sysfs exposes device, driver, bus, class, and subsystem state to userspace through small text attributes.

## Core Concepts

- kobjects
- device attributes
- show method
- store method
- one-value-per-file convention
- permissions
- lifetime
- ABI documentation

## Mental Model

Sysfs is for structured device state and simple control knobs. It is not a log stream, binary protocol, or replacement for a real subsystem ABI.

## Practice Skeleton

- Add a read-only device attribute.
- Add a writable attribute with validation.
- Document expected values.
- Test concurrent reads and writes.

## Debugging Checklist

- Check file permissions.
- Validate all input from userspace.
- Avoid long-running work inside sysfs callbacks.
- Confirm the backing device still exists while callbacks run.

## Related Topics

- [Character Device Basics](character-device-basics.md)
- [Debugfs And Sysfs Inspection](../debugging/debugfs-and-sysfs-inspection.md)
- [Driver Binding, Probe, And Remove](driver-binding-probe-remove.md)

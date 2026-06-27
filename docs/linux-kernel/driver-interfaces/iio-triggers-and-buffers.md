---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# IIO Triggers And Buffers

## What Problem Does This Solve?

IIO triggers and buffers support sampled data capture where userspace needs streams instead of one-off sysfs reads.

## Core Concepts

- trigger
- triggered buffer
- scan elements
- timestamp
- buffer enable
- sample layout
- hardware FIFO overview
- userspace buffered reads

## Mental Model

Sysfs attributes are good for slow state reads. Buffers and triggers are for repeated samples with timing and layout discipline.

## Practice Skeleton

- Identify whether a device needs buffered reads.
- Define scan elements.
- Connect a trigger.
- Validate sample layout from userspace.

## Debugging Checklist

- Check enabled scan elements.
- Check timestamp handling.
- Check trigger ownership.
- Check buffer overrun behavior.

## Related Topics

- [IIO Subsystem](iio-subsystem.md)
- [IIO Channels And Sysfs](iio-channels-and-sysfs.md)
- [IRQ Handling](irq-handling.md)

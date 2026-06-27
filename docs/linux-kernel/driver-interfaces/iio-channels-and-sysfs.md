---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# IIO Channels And Sysfs

## What Problem Does This Solve?

IIO channel definitions create standardized sysfs attributes for sensor and converter data.

## Core Concepts

- channel type
- indexed channels
- modified channels
- `read_raw`
- raw attribute
- scale attribute
- offset attribute
- device name

## Mental Model

IIO sysfs files are generated from channel descriptions and callbacks. The driver should describe what each channel means, not invent a custom file layout.

## Practice Skeleton

- Add four dummy voltage channels.
- Implement raw reads.
- Add shared scale.
- Inspect generated `in_*` sysfs files.

## Debugging Checklist

- Check channel masks.
- Check generated attribute names.
- Check value return format.
- Confirm userspace sees the expected IIO device.

## Related Topics

- [IIO Subsystem](iio-subsystem.md)
- [Sysfs Attributes](../fundamentals/sysfs-attributes.md)
- [Device Classes, Uevents, And udev](../fundamentals/device-classes-uevents-and-udev.md)

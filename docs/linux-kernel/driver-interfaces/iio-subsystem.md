---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# IIO Subsystem

## What Problem Does This Solve?

The Industrial I/O subsystem supports sensors, ADCs, DACs, IMUs, and other data-acquisition devices.

## Core Concepts

- IIO device
- IIO channel
- channel attributes
- raw values
- scale and offset
- triggers
- buffers
- events
- sysfs ABI

## Mental Model

IIO separates device data channels from board details and userspace presentation. A driver describes channels and conversion metadata; userspace reads standardized attributes or buffered data.

## Practice Skeleton

- Register a minimal IIO device.
- Define several voltage channels.
- Expose raw and scale attributes.
- Inspect the device under `/sys/bus/iio/devices`.

## Debugging Checklist

- Check channel numbering and names.
- Check scale and offset semantics.
- Check whether the device needs buffered capture.
- Keep units consistent with IIO ABI expectations.

## Related Topics

- [IIO Channels And Sysfs](iio-channels-and-sysfs.md)
- [IIO Triggers And Buffers](iio-triggers-and-buffers.md)
- [SPI Device Drivers](spi-device-drivers.md)

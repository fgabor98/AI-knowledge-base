---
status: draft
reviewed: false
domain: linux-userspace
difficulty: advanced
last_reviewed: null
---

# Stage 10: Hardware-Facing Userspace And Kernel UAPI

Connect userspace programs to drivers and standard kernel subsystems through documented, versioned, and testable interfaces.

This stage is a collection of focused draft pages. Read the overview first, then study the leaf pages in order while extending one small C utility or service.

## Learning Materials

1. [Kernel Versus Userspace Boundary](kernel-versus-userspace-boundary.md)
2. [devfs, sysfs, udev, And Device Discovery](devfs-sysfs-udev-and-discovery.md)
3. [Standard UAPI Operation Patterns](standard-uapi-operation-patterns.md)
4. [ioctl ABI And Compatibility](ioctl-abi-and-compatibility.md)
5. [poll, mmap, And Device Events](poll-mmap-and-device-events.md)
6. [GPIO, I2C, And SPI Userspace](gpio-i2c-and-spi-userspace.md)
7. [Serial, Input, Sensors, And hwmon](serial-input-sensors-and-hwmon.md)
8. [CAN, Watchdog, And Control Interfaces](can-watchdog-and-control-interfaces.md)
9. [Specialized UAPI: V4L2, ALSA, DRM, UIO, And VFIO](specialized-uapi-v4l2-alsa-drm-uio-vfio.md)

## Study Pattern

For each page:

1. Read the contract and identify the libc, POSIX, Linux, kernel UAPI, or init-system layer.
2. Implement the smallest host-side example.
3. Add error, timeout, ownership, and cleanup paths.
4. Observe the result with the relevant Linux tools.
5. Repeat on the target and record differences.
6. Integrate the mechanism into the running capstone service.

## Stage Outcomes

By the end of this stage, you should be able to:

- explain and demonstrate kernel versus userspace boundary;
- explain and demonstrate devfs, sysfs, udev, and device discovery;
- explain and demonstrate standard uapi operation patterns;
- explain and demonstrate ioctl abi and compatibility;
- explain and demonstrate poll, mmap, and device events;
- connect the mechanism to an embedded Linux failure, test, or service-design decision;
- produce evidence that distinguishes application, kernel, deployment, and hardware causes.

## Completion Criteria

- The examples compile with warnings and debug information.
- Normal, interrupted, missing-resource, and teardown paths are tested.
- Resource ownership and target assumptions are documented.
- At least one failure has been diagnosed using observable evidence.
- The work is linked to the next stage or an existing capstone.

## Related Topics

- [Linux Userspace And System Programming](../index.md)
- [C Programming](../../c/index.md)
- [Linux Kernel Programming](../../linux-kernel/index.md)
- [Embedded Linux](../../embedded-linux/index.md)

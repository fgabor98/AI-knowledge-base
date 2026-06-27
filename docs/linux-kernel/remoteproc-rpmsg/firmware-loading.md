---
status: draft
reviewed: false
domain: linux-kernel
difficulty: advanced
last_reviewed: null
---

# Firmware Loading

## What Problem Does This Solve?

Remote cores and devices often require firmware files that must be available at the right time and match the kernel and hardware expectations.

## Core Concepts

- firmware search path
- `request_firmware`
- initramfs firmware
- rootfs firmware
- version compatibility
- resource table
- signing policy
- update coordination

## Mental Model

Firmware is part of the platform release. Kernel, Device Tree, firmware image, and userspace update flow must be compatible.

## Practice Skeleton

- Place firmware in the expected rootfs path.
- Load firmware through remoteproc.
- Test missing-firmware behavior.
- Record firmware version metadata.

## Debugging Checklist

- Check firmware filename.
- Check file location and permissions.
- Check whether firmware is needed before rootfs.
- Check resource table compatibility.

## Related Topics

- [Remoteproc Framework](remoteproc-framework.md)
- [Initramfs Options](../configuration-and-platform-policy/initramfs-options.md)
- [Embedded Productization](../../embedded-productization/index.md)

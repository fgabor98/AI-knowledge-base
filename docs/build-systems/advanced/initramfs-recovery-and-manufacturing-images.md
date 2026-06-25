---
status: draft
reviewed: false
domain: build-systems
difficulty: advanced
reviewer: null
last_reviewed: null
---

# Initramfs, Recovery, And Manufacturing Images

## What Problem Does This Solve?

Products often need more than one image: development images, production images, recovery images, manufacturing images, and test fixtures. This page explains how those artifacts fit into the build system.

## Image Types

- initramfs for early userspace
- recovery rootfs
- factory flashing image
- production image
- test image
- NFS development rootfs
- ramdisk diagnostics image

## Initramfs Role

Initramfs is loaded by the kernel before the real root filesystem. It can:

- load storage drivers
- unlock encrypted storage
- choose a rootfs slot
- run recovery tools
- flash internal media
- collect diagnostics

## Manufacturing Image Role

A manufacturing image may:

- boot from removable media
- test hardware
- program eMMC/flash
- write serial numbers or calibration data
- install production image
- verify programmed checksums
- save logs

Keep manufacturing logic separate from production runtime policy.

## Build Outputs To Track

- kernel with built-in or external initramfs
- initramfs archive
- recovery rootfs
- flashing scripts
- production image payload
- hardware test logs
- programmed artifact checksums

## Common Mistakes

- using production image as manufacturing image without test tooling
- leaving manufacturing credentials in production rootfs
- manually rebuilding initramfs outside CI
- forgetting to update kernel command line
- failing to test recovery after a broken update

## Related Topics

- [Filesystem Image Basics](../filesystem-image-basics.md)
- [Linux Kernel Initramfs](linux-kernel/initramfs-and-built-in-rootfs.md)
- [OTA and Update System Build Integration](ota-update-system-build-integration.md)


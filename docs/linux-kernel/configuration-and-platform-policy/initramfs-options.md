---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# Initramfs Options

## What Problem Does This Solve?

Initramfs configuration determines which early userspace files, modules, firmware, and recovery tools are available before the real root filesystem is mounted.

## Core Concepts

- built-in initramfs
- external initramfs
- early userspace
- module loading
- firmware loading
- recovery shell
- rootfs discovery
- manufacturing and rescue flows

## Mental Model

Initramfs is the bridge between kernel startup and product userspace. Keep its contents minimal but sufficient for boot, recovery, and diagnostics.

## Practice Skeleton

- Boot with and without initramfs.
- Add one module and firmware file.
- Validate rootfs discovery.
- Test a recovery path.

## Debugging Checklist

- Check initramfs contents.
- Check kernel command line root arguments.
- Check firmware and module paths.
- Check whether required drivers are built in or included in initramfs.

## Related Topics

- [Initramfs And Built-In Root Filesystem](../../build-systems/advanced/linux-kernel/initramfs-and-built-in-rootfs.md)
- [Built-In Vs Module Policy](built-in-vs-module-policy.md)
- [Embedded Linux](../../embedded-linux/index.md)

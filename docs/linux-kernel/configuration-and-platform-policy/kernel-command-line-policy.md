---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# Kernel Command Line Policy

## What Problem Does This Solve?

The kernel command line controls boot-time behavior, device initialization, root filesystem selection, logging, security, and diagnostics.

## Core Concepts

- `console=`
- `root=`
- `rootwait`
- `earlycon`
- log level
- init path
- panic behavior
- LSM parameters
- product-owned defaults

## Mental Model

The command line is part of the boot contract between bootloader, kernel, and product image. Treat it as versioned platform configuration.

## Practice Skeleton

- Capture the runtime command line.
- Map each option to its owner.
- Define development and production variants.
- Test boot with minimal required arguments.

## Debugging Checklist

- Check `/proc/cmdline`.
- Check bootloader environment or boot script.
- Check duplicate or conflicting arguments.
- Verify console and rootfs arguments first.

## Related Topics

- [Embedded Linux](../../embedded-linux/index.md)
- [Debug Vs Production Configs](debug-vs-production-configs.md)
- [Initramfs Options](initramfs-options.md)

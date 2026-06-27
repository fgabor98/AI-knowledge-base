---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# Built-In Vs Module Policy

## What Problem Does This Solve?

Product teams need consistent rules for which drivers are built into the kernel and which ship as modules.

## Core Concepts

- early boot dependencies
- root filesystem dependencies
- initramfs
- optional hardware
- field updates
- module signing
- recovery images
- support diagnostics

## Mental Model

The policy follows boot dependency and update strategy. Root-critical and recovery-critical code usually belongs in the kernel image or initramfs.

## Practice Skeleton

- List drivers needed before mounting rootfs.
- List optional or replaceable drivers.
- Define module signing requirements.
- Test boot with the rootfs driver removed from modules.

## Debugging Checklist

- Confirm boot-critical storage, filesystem, and bus drivers.
- Check firmware availability.
- Check initramfs contents.
- Check secure boot and signing requirements.

## Related Topics

- [Built-In Drivers Vs Loadable Modules](../fundamentals/built-in-vs-loadable-modules.md)
- [Initramfs Options](initramfs-options.md)
- [Module Signing And Hardening](module-signing-and-hardening.md)

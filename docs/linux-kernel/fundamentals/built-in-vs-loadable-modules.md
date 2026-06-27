---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# Built-In Drivers Vs Loadable Modules

## What Problem Does This Solve?

Drivers can be linked into the kernel image or shipped as loadable modules. The choice affects boot order, recovery, update strategy, dependencies, and product policy.

## Core Concepts

- `CONFIG_FOO=y`
- `CONFIG_FOO=m`
- initcall ordering
- module autoloading
- initramfs module loading
- firmware availability
- root filesystem dependencies
- module signing

## Mental Model

Build a driver in when the system needs it before the root filesystem or module loader is available. Use a module when late loading, replacement, diagnostics, or optional hardware support matters more.

## Practice Skeleton

- Enable one driver as built-in and one as a module.
- Compare boot logs and module lists.
- Move a module into an initramfs and confirm early availability.

## Debugging Checklist

- Check the final `.config`.
- Check whether the driver appears in `lsmod`.
- Check whether the hardware is needed before userspace starts.
- Check firmware and dependency availability at the time the driver probes.

## Related Topics

- [Kernel Configuration And Platform Policy](../configuration-and-platform-policy/index.md)
- [Kconfig And Defconfig](../../build-systems/advanced/linux-kernel/kconfig-and-defconfig.md)
- [Built-In Vs Module Policy](../configuration-and-platform-policy/built-in-vs-module-policy.md)

---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# Kernel Module Lifecycle

## What Problem Does This Solve?

Kernel modules let driver code be compiled, loaded, inspected, and removed independently from the base kernel image.

## Core Concepts

- module init function
- module exit function
- `module_init`
- `module_exit`
- module metadata
- `insmod`, `modprobe`, `rmmod`
- init failure cleanup
- module reference ownership

## Mental Model

Module init registers something with the kernel. Module exit must unregister the same thing and leave no active callbacks, devices, timers, work items, or references behind.

## Practice Skeleton

- Build a minimal external module.
- Load it and confirm the init log.
- Remove it and confirm the exit log.
- Add one intentional init failure path and clean up correctly.

## Debugging Checklist

- Check `dmesg` for init and exit logs.
- Check `lsmod` and `modinfo`.
- Confirm the module was built against the running kernel.
- Confirm every registration has a matching unregister path.

## Related Topics

- [Modules And External Modules](../../build-systems/advanced/linux-kernel/modules-and-external-modules.md)
- [Module Parameters And Driver Logging](module-parameters-and-logging.md)
- [Built-In Drivers Vs Loadable Modules](built-in-vs-loadable-modules.md)

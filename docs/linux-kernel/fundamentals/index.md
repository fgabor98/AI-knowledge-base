---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# Linux Device Driver Fundamentals

This track covers the first mental model for Linux driver work: how a driver enters the kernel, binds to hardware, exposes a user-facing interface where appropriate, and cleans up correctly.

## Learning Materials

1. [Kernel Module Lifecycle](kernel-module-lifecycle.md)
2. [Built-In Drivers Vs Loadable Modules](built-in-vs-loadable-modules.md)
3. [Device Tree Hardware Description](device-tree-hardware-description.md)
4. [Device Tree Overlays](device-tree-overlays.md)
5. [Driver Binding, Probe, And Remove](driver-binding-probe-remove.md)
6. [Platform Devices And Platform Drivers](platform-devices-and-drivers.md)
7. [Device Tree Matching From Drivers](device-tree-matching.md)
8. [Resource Lookup And Managed Allocation](resource-lookup-and-devm.md)
9. [Character Device Basics](character-device-basics.md)
10. [Device Classes, Uevents, And udev](device-classes-uevents-and-udev.md)
11. [Sysfs Attributes](sysfs-attributes.md)
12. [Kobjects And Sysfs Groups](kobjects-and-sysfs-groups.md)
13. [Pollable Sysfs Attributes](pollable-sysfs-attributes.md)
14. [Module Parameters And Driver Logging](module-parameters-and-logging.md)
15. [User-Space Hardware Access Vs Kernel Drivers](userspace-hardware-access-vs-kernel-drivers.md)

## Mental Model

A practical embedded Linux driver usually has three connected surfaces:

```text
hardware description
-> kernel device object
-> driver probe path
-> subsystem or user-facing interface
```

The driver should not guess board details. It should bind through the kernel device model, read resources from firmware data such as Device Tree, request those resources through kernel APIs, and release them deterministically.

## Completion Criteria

- Build and load a minimal external module.
- Explain when a driver should be built in instead of loaded as a module.
- Describe hardware with Device Tree without hard-coding board details in the driver.
- Trace a platform driver from Device Tree `compatible` string to `probe`.
- Use managed allocation for memory and device resources.
- Expose a minimal character device or sysfs attribute without leaking lifetime assumptions.

## Related Topics

- [Common Driver Interfaces](../driver-interfaces/index.md)
- [Kernel Execution And Concurrency](../execution-and-concurrency/index.md)
- [Linux Kernel Build System](../../build-systems/advanced/linux-kernel/index.md)
- [Device Tree](../../device-tree/index.md)

---
status: draft
reviewed: false
domain: linux-kernel
difficulty: beginner
last_reviewed: null
---

# Kernel Mental Model

## What Problem Does This Solve?

Kernel programming is easier to learn when the learner has a small model of what the kernel owns and why driver code is different from userspace code.

## Core Concepts

- kernel space
- user space
- system calls
- processes and tasks
- interrupts
- hardware resources
- drivers
- subsystems
- kernel objects
- userspace ABI

## Mental Model

Userspace programs ask the kernel to perform privileged work. Drivers are kernel code that integrate hardware into kernel subsystems and expose controlled interfaces to the rest of the system.

```text
userspace
-> syscall, device node, sysfs, netlink, input event, or subsystem ABI
-> kernel subsystem
-> driver
-> hardware resource
```

## Practice Skeleton

- Trace one userspace command to the kernel interface it uses.
- Identify whether a device appears as a character device, network interface, input event device, IIO device, or sysfs object.
- Draw the ownership path from userspace to driver to hardware.

## Debugging Checklist

- Identify whether the problem is userspace policy, kernel subsystem behavior, driver behavior, firmware data, or hardware state.
- Check what kernel object represents the device.
- Check which subsystem owns the userspace ABI.
- Avoid treating driver code like a normal userspace library.

## Related Topics

- [Device Model Primer](device-model-primer.md)
- [Driver Development Workflow](driver-development-workflow.md)
- [Linux Device Driver Fundamentals](../fundamentals/index.md)

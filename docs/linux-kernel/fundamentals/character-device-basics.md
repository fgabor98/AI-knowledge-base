---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# Character Device Basics

## What Problem Does This Solve?

Character devices expose byte-stream or command-style kernel interfaces to userspace through device nodes.

## Core Concepts

- major and minor numbers
- `struct cdev`
- `file_operations`
- `open`
- `read`
- `write`
- `unlocked_ioctl`
- `poll`
- `copy_to_user`
- `copy_from_user`
- `udev`

## Mental Model

A character device is a userspace ABI. Once userspace depends on it, changes need compatibility discipline.

## Practice Skeleton

- Allocate a device number.
- Register a `cdev`.
- Create a device node.
- Implement minimal `open`, `read`, and `write` behavior.

## Debugging Checklist

- Check `/dev` node ownership and permissions.
- Check return values and partial I/O behavior.
- Validate userspace pointers with copy helpers.
- Keep ABI structures explicitly sized.

## Related Topics

- [Userspace Copy And ioctl ABI](../memory-and-io/userspace-copy-and-ioctl-abi.md)
- [Sysfs Attributes](sysfs-attributes.md)
- [Kernel Module Lifecycle](kernel-module-lifecycle.md)

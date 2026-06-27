---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# Userspace Copy And ioctl ABI

## What Problem Does This Solve?

Drivers that expose character devices must safely move data between kernel and userspace and preserve ABI compatibility.

## Core Concepts

- userspace pointers
- `copy_to_user`
- `copy_from_user`
- `get_user`
- `put_user`
- `ioctl`
- fixed-width types
- ABI versioning
- compatibility

## Mental Model

Userspace memory is untrusted and may fault. ABI structures are contracts, not internal implementation details.

## Practice Skeleton

- Add a read path that copies data to userspace.
- Add a write path that validates copied input.
- Define a small ioctl structure with fixed-width fields.
- Test 32-bit compatibility concerns if relevant.

## Debugging Checklist

- Check partial copy return values.
- Validate lengths and reserved fields.
- Avoid exposing kernel pointers or padding.
- Keep ABI structures independent from internal structs.

## Related Topics

- [Character Device Basics](../fundamentals/character-device-basics.md)
- [Wait Queues And Completions](../execution-and-concurrency/wait-queues-and-completions.md)
- [Kernel Debugging Basics](../debugging/index.md)

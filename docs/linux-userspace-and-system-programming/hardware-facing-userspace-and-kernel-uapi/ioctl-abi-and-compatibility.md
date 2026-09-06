---
status: draft
reviewed: false
domain: linux-userspace
difficulty: advanced
last_reviewed: null
---

# ioctl ABI And Compatibility

## What problem does this solve?

`ioctl` is flexible but creates a binary ABI that is difficult to change once shipped.
Command numbers, direction/size encoding, structure padding, pointers, and 32/64-bit
compatibility must be designed before clients depend on them.

## Command design

Linux conventions use `_IO`, `_IOR`, `_IOW`, and `_IOWR` with a type and data type.
The encoded size describes the user-visible argument size; it is not a guarantee that
the kernel may blindly copy or trust that many bytes. The kernel validates direction,
size, pointer ranges, reserved fields, and permissions.

```c
#define SENSOR_GET_INFO _IOR('S', 0x01, struct sensor_info)
```

Use fixed-width integers, explicit reserved fields, version/size fields where needed,
and stable command numbers. Initialize output structures fully to avoid leaking
padding or stale data.

## Pointers and compatibility

Pointers in UAPI structures are problematic across 32/64-bit processes and future
kernel implementations. Prefer offsets, `__u64` handles, or separate data transfers
with explicit sizes. If pointers are unavoidable, implement and test compat behavior;
do not assume a native structure layout works for a 32-bit client.

## Error and side-effect contract

Document `EINVAL`, `ENOTTY`, `ENODEV`, `EFAULT`, `EINTR`, `EAGAIN`, and device-specific
errors. Define whether an error can follow a partial side effect and how the caller
reconciles state. The ioctl number alone is not the protocol.

## Common mistakes

- Copying compiler-layout structs with pointers or implicit padding.
- Reusing ioctl numbers for changed semantics.
- Trusting encoded size without validating actual user memory.
- Returning uninitialized padding to userspace.
- Ignoring 32/64-bit compat and endianness.
- Treating ENOTTY as a generic hardware fault.

## Debugging checklist

- Record command number, structure size, architecture, kernel, and UAPI headers.
- Trace ioctl arguments and return values with redaction.
- Test old/new clients, invalid sizes, reserved fields, pointers, and compat builds.
- Inspect kernel logs and driver state for side effects after failure.
- Keep a versioned UAPI header and compatibility test matrix.

## Related topics

- [Stage 10: Hardware-Facing Userspace And Kernel UAPI](index.md)
- [Standard UAPI Operation Patterns](standard-uapi-operation-patterns.md)
- [poll, mmap, And Device Events](poll-mmap-and-device-events.md)

## References

- [`ioctl(2)`](https://man7.org/linux/man-pages/man2/ioctl.2.html)
- [Linux kernel ioctl-based interfaces](https://docs.kernel.org/driver-api/ioctl.html)
- [Linux ioctl UAPI guidance](https://www.kernel.org/doc/html/latest/userspace-api/ioctl/ioctl-number.html)

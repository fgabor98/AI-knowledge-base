---
status: draft
reviewed: false
domain: linux-userspace
difficulty: advanced
last_reviewed: null
---

# Kernel Versus Userspace Boundary

## What problem does this solve?

Userspace cannot safely perform privileged hardware access directly. It must request
operations through a kernel UAPI that defines validation, access control, memory
ownership, wakeups, and errors. Keeping the boundary explicit makes upgrades and
failure diagnosis possible.

## What belongs where

The kernel should own interrupts, DMA, register access, power/clock sequencing,
concurrency with other clients, and hardware-specific safety. Userspace should own
product policy, configuration, protocol state, retries, user-visible errors, and
service lifecycle. A userspace helper may implement policy but must not recreate a
private driver by poking registers.

## UAPI forms

| Form | Use | Boundary concern |
| --- | --- | --- |
| Character/block device | Data/control stream | FD ownership, blocking, ioctl, poll |
| sysfs | Device-model attributes | Text ABI, units, lifetime, not bulk data |
| procfs | Process/kernel observation | Snapshot and permission semantics |
| netlink | Kernel event/config protocol | Message versioning, credentials |
| `ioctl` | Structured device operations | Binary ABI and compat layout |
| `mmap` | Large/shared/device buffers | Protection, cache, lifetime, synchronization |
| standard subsystem | GPIO, IIO, V4L2, ALSA, DRM, CAN | Prefer common semantics over private UAPI |

## Memory crossing the boundary

Never pass a userspace pointer as a permanent kernel/device address. The kernel must
validate ranges and access user memory safely; asynchronous operations need a defined
copy, pin, or buffer ownership model. Fixed-width UAPI fields and explicit padding
are safer than compiler-dependent C structs.

## Errors and completion

Distinguish `ENODEV`/`ENXIO` (device/resource unavailable), `EACCES`/`EPERM` (policy),
`EAGAIN` (not ready), `EINTR` (interrupted), `EIO` (lower-layer error), and `ETIMEDOUT`
(deadline). A return from the kernel may mean queued, accepted, or completed; the
subsystem documentation decides.

## Security boundary

Device-node permissions are coarse. Run a service with the smallest device set,
capabilities, and namespace view. Treat all device data as untrusted and validate
length, ranges, status, sequence, and timestamps. A writable UAPI can be equivalent
to hardware privilege.

## Common mistakes

- Treating `/dev` access as direct register access.
- Using `/dev/mem` or private sysfs attributes instead of a documented UAPI.
- Assuming a successful ioctl means hardware completion.
- Passing raw pointers or compiler-layout structs across the ABI.
- Debugging userspace before checking driver binding and power/firmware state.

## Debugging checklist

- Identify subsystem, driver, kernel version, UAPI header, and device identity.
- Trace calls and correlate with kernel logs and sysfs state.
- Check credentials, capabilities, namespaces, and device-node ownership.
- Test missing, busy, reset, unplug, timeout, and malformed-data paths.
- Document what the userspace trace proves and what requires hardware evidence.

## Related topics

- [Stage 10: Hardware-Facing Userspace And Kernel UAPI](index.md)
- [Standard UAPI Operation Patterns](standard-uapi-operation-patterns.md)
- [ioctl ABI And Compatibility](ioctl-abi-and-compatibility.md)
- [Userspace, Kernel, And Hardware Boundary](../environment-and-mental-model/userspace-kernel-and-hardware-boundary.md)

## References

- [Linux kernel userspace API](https://www.kernel.org/doc/html/latest/userspace-api/index.html)
- [`ioctl(2)`](https://man7.org/linux/man-pages/man2/ioctl.2.html)
- [Kernel ABI documentation](https://www.kernel.org/doc/html/latest/admin-guide/abi.html)

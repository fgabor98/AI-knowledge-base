---
status: draft
reviewed: false
domain: linux-userspace
difficulty: beginner
last_reviewed: null
---

# Pseudo-filesystems And Device Nodes

## What problem does this solve?

Embedded targets expose process state, kernel state, devices, and runtime resources
through filesystem-shaped interfaces. These objects look like files but do not have
the same persistence, buffering, or semantics as regular files. Treating them as
ordinary storage leads to invalid writes, missing mounts, incorrect permissions, and
boot-order bugs.

## The important pseudo-filesystems

| Filesystem | Typical mount | Purpose | Persistence |
| --- | --- | --- | --- |
| `proc` | `/proc` | Processes, kernel information, selected sysctls | Generated; volatile |
| `sysfs` | `/sys` | Kernel device model, drivers, buses, attributes | Generated; volatile |
| `devtmpfs` | `/dev` | Kernel-created device nodes | Generated; volatile |
| `tmpfs` | `/run`, `/tmp` | RAM-backed files and directories | Volatile; limited by memory/swap policy |
| `debugfs` | `/sys/kernel/debug` | Developer diagnostics | Optional; often absent or restricted in production |
| `configfs` | `/sys/kernel/config` | Userspace creates kernel configuration objects | Runtime state; subsystem-specific |
| `cgroup2` | `/sys/fs/cgroup` | Resource control and accounting | Runtime hierarchy; policy managed by init |
| `mqueue` | `/dev/mqueue` | POSIX message queues | Kernel-managed; namespace/lifetime dependent |

Mount options and namespace membership affect visibility. A directory can exist while
the intended filesystem is not mounted there; a service can have a private mount
namespace; and a container can expose a filtered `/proc`.

## `/proc` and `/sys` are interfaces

`/proc/<pid>/status`, `/proc/meminfo`, and `/proc/interrupts` expose kernel-derived
state. Many values are snapshots and can change immediately after reading. A write to
a writable proc entry may change global or namespace-local kernel policy, so it needs
privilege and explicit product justification.

`/sys` represents the kernel device model. A path can expose a device attribute,
driver state, firmware loading interface, or subsystem control. Attribute text is
not automatically stable across kernels or products. Prefer a documented subsystem
UAPI and treat sysfs attributes as versioned kernel interfaces.

```sh
mount | grep -E ' on /(proc|sys|dev|run) '
findmnt -t proc,sysfs,devtmpfs,tmpfs,debugfs,configfs,cgroup2
cat /proc/self/mountinfo
ls -l /sys/class /sys/devices 2>/dev/null
cat /sys/class/net/lo/operstate 2>/dev/null
```

## Device nodes

A device node is a filesystem object that identifies a kernel device interface. Its
major/minor numbers are interpreted by the kernel; they are not a portable hardware
address. `udev`, `mdev`, `devtmpfs`, or a static device policy creates names and sets
permissions according to the target design.

```sh
stat /dev/null
ls -l /dev/null /dev/ttyS0
udevadm info --query=all --name=/dev/ttyS0 2>/dev/null || true
```

A node can exist while the driver is absent or the hardware is unavailable. Opening
it may succeed and a later operation may return `ENODEV`, `ENXIO`, `EIO`, or a
subsystem-specific error. Conversely, a missing node can indicate missing devtmpfs,
discovery policy, permissions, or driver binding.

Never infer a driver API from a device node name. Read the subsystem documentation
and inspect the device’s sysfs links, UAPI, and kernel configuration.

## `tmpfs` and memory pressure

`tmpfs` consumes memory as data is written and may use swap where available. A
write can fail with `ENOSPC` because the mount size limit is reached even when the
persistent filesystem has free space. `/run` and `/tmp` therefore need size and
cleanup policy on constrained devices. Runtime directories should be recreated on
boot, not backed up as configuration.

## Access and security

Pseudo-filesystem paths can reveal process arguments, environment, memory maps,
device topology, or kernel configuration. Use mount options such as `hidepid`, service
namespaces, mode/ACL policy, and capability restrictions where appropriate. Do not
grant write access to all of `/sys` or `/proc` to avoid one narrow permission check.

Treat file content as untrusted input: validate sizes, numeric ranges, encodings, and
state transitions. A successful write of text to sysfs means the kernel accepted the
attribute operation; it does not necessarily mean hardware reached the requested
state.

## Minimal diagnostic sequence

```sh
test -f /proc/self/status && echo proc-mounted
test -d /sys/class && echo sysfs-visible
test -c /dev/null && echo dev-node-present
findmnt -T /proc/self/status
findmnt -T /sys/class
findmnt -T /dev/null
```

Run this as the actual service user and, when relevant, in its service namespace.
Compare the result with an interactive root shell only after recording the identity
difference.

## Common mistakes

- Treating `/proc` or `/sys` as durable configuration storage.
- Assuming a device node proves driver binding or hardware readiness.
- Hard-coding major/minor numbers or desktop udev names in a product.
- Writing sysfs attributes without validating units, ranges, and failure behavior.
- Forgetting that `tmpfs` can fill independently of persistent storage.
- Assuming a container’s `/proc` or `/dev` is equivalent to the target’s.
- Enabling `debugfs` in production without a security and support policy.

## Debugging checklist

- Confirm filesystem type, mount options, namespace, and visibility.
- Check device-node type, owner, mode, major/minor, and symlink target.
- Follow `/sys/class/...` links to the underlying device and driver.
- Check kernel logs, uevents, firmware, and subsystem state.
- Test as the service UID/GIDs with the same sandbox and capabilities.
- Check `tmpfs` size, inode use, and cleanup after repeated restarts.

## Related topics

- [Stage 1: Linux Runtime, Filesystem, And Rootfs](index.md)
- [Mounts, Initramfs, And Rootfs Layout](mounts-initramfs-and-rootfs-layout.md)
- [Kernel Versus Userspace Boundary](../hardware-facing-userspace-and-kernel-uapi/kernel-versus-userspace-boundary.md)
- [Devfs, Sysfs, Udev, And Discovery](../hardware-facing-userspace-and-kernel-uapi/devfs-sysfs-udev-and-discovery.md)

## References

- [`proc(5)`](https://man7.org/linux/man-pages/man5/proc.5.html)
- [Linux kernel `/proc` documentation](https://www.kernel.org/doc/html/latest/filesystems/proc.html)
- [Linux kernel sysfs documentation](https://www.kernel.org/doc/html/latest/filesystems/sysfs.html)
- [`tmpfs(5)`](https://man7.org/linux/man-pages/man5/tmpfs.5.html)
- [`udev(7)`](https://man7.org/linux/man-pages/man7/udev.7.html)

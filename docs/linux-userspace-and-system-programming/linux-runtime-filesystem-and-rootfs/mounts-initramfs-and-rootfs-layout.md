---
status: draft
reviewed: false
domain: linux-userspace
difficulty: intermediate
last_reviewed: null
---

# Mounts, Initramfs, And Rootfs Layout

## What problem does this solve?

Boot-time failures often look like application failures: a configuration path is
missing, a device node is absent, or a writable directory is read-only. The cause is
frequently that the expected filesystem has not been mounted, was mounted in another
namespace, or was replaced during initramfs handoff.

## Mount composition

Linux presents a hierarchy assembled from mounts. A mount attaches a filesystem tree
at a directory in a mount namespace. The directory’s old contents become hidden
while the mount is active. A bind mount attaches an existing directory or file at a
second location; it does not copy data.

```text
mount namespace
  /
  +-- /usr       read-only system filesystem
  +-- /etc       image or configuration filesystem
  +-- /var       persistent or volatile filesystem
  +-- /run       tmpfs
  +-- /proc      procfs
  +-- /sys       sysfs
  +-- /dev       devtmpfs
  +-- /data      persistent application partition
```

The same mount point can have different options or a different source in another
namespace. `/proc/self/mountinfo` describes the reader’s namespace; inspect the
service PID for service-specific evidence.

## Initramfs handoff

An initramfs is an early root filesystem loaded with or by the kernel. Its job may
include loading modules, obtaining keys, discovering storage, assembling RAID or
LVM, unlocking encryption, loading firmware, and selecting an A/B root slot. It then
hands control to the real root filesystem using a product-specific mechanism such as
`switch_root` or `pivot_root`.

```text
kernel unpacks initramfs
        |
        v
early /init discovers the real root
        |
        v
mount real root at a staging path
        |
        v
move required mounts / switch root
        |
        v
new /sbin/init or PID 1 starts normal userspace
```

Files in the initramfs are not necessarily present after handoff. Open FDs and
processes that survive a handoff can still refer to old mounts, which is why early
userspace must define what is preserved and what is closed. An application should
not assume it can run equally from initramfs and the final root.

## Mount configuration and readiness

`/etc/fstab` is one possible source of mount policy; embedded products may generate
mounts from init scripts, systemd units, Buildroot scripts, or a read-only image
manifest. Record the actual authority. A dependency on `/data` should express that
the mount is available, not merely that `/data` exists as a directory.

Useful commands:

```sh
findmnt
findmnt -T /var/lib/my-service
mountpoint /proc
cat /proc/self/mountinfo
cat /proc/self/mounts
df -hT
```

`/proc/mounts` is a compatibility view; `mountinfo` includes more namespace and
propagation detail. `df` reports filesystem capacity, while an application can still
fail because of permissions, quotas, inode exhaustion, or a read-only remount.

## Mount flags and propagation

Important flags include `ro`/`rw`, `nosuid`, `nodev`, `noexec`, `noatime`, and
`relatime`. They change what applications can do even when mode bits look correct.
Mount propagation (`private`, `shared`, `slave`) controls whether mount events move
between namespaces. Service isolation often uses private mounts so that a service
cannot alter the host’s view.

Do not grant `CAP_SYS_ADMIN` or mount privileges to an application merely to make a
missing directory work. Fix the boot and service dependency, or use a narrowly
defined helper with an auditable policy.

## Rootfs layout decisions

An embedded rootfs commonly separates:

- immutable boot and system content;
- mutable configuration, sometimes with factory defaults and an overlay;
- persistent application state and calibration;
- volatile runtime state and sockets;
- logs and crash evidence with bounded capacity;
- recovery and update metadata.

The layout determines update atomicity, rollback behavior, wear, recovery, and what a
factory reset means. Document whether a missing directory is a fatal deployment error
or should be created on first boot.

## Diagnosing “path exists but mount is missing”

```sh
test -d /data && echo directory-present
findmnt -T /data || echo no-filesystem-mounted
stat -f -c 'type=%T blocks=%b free=%a' /data
```

An empty directory on the root filesystem can conceal a missing data mount. Compare
filesystem type, device/source, mount options, and expected marker files. Do not
create application state in the fallback directory unless the product explicitly
allows degraded operation; it may disappear when the mount later arrives.

## Common mistakes

- Treating a directory’s existence as proof that its mount is ready.
- Assuming `/etc/fstab` is used on an appliance image.
- Starting services before storage, `/run`, `/dev`, or network readiness.
- Leaving initramfs processes or descriptors attached to obsolete mounts.
- Granting mount privileges to a service instead of fixing dependency ordering.
- Ignoring read-only, `nosuid`, `nodev`, `noexec`, quota, and inode limits.
- Writing state into a mountpoint fallback directory.

## Debugging checklist

- Capture the kernel command line, initramfs logs, root selection, and handoff point.
- Compare `/proc/<pid>/mountinfo` for the failing service and an interactive shell.
- Identify filesystem source, type, options, propagation, and capacity.
- Check service ordering and readiness, not just process start timestamps.
- Test missing storage, delayed storage, read-only remount, and full filesystem cases.
- Verify recovery and update behavior when a partition is absent or corrupt.

## Related topics

- [Stage 1: Linux Runtime, Filesystem, And Rootfs](index.md)
- [Pseudo-filesystems And Device Nodes](pseudo-filesystems-and-device-nodes.md)
- [Read-Only Rootfs, Overlayfs, And Persistent State](read-only-rootfs-overlayfs-and-persistent-state.md)
- [Initramfs, Recovery, And Manufacturing Images](../../build-systems/advanced/initramfs-recovery-and-manufacturing-images.md)

## References

- [`mount(8)`](https://man7.org/linux/man-pages/man8/mount.8.html)
- [`mount_namespaces(7)`](https://man7.org/linux/man-pages/man7/mount_namespaces.7.html)
- [`fstab(5)`](https://man7.org/linux/man-pages/man5/fstab.5.html)
- [Linux kernel initramfs documentation](https://www.kernel.org/doc/html/latest/driver-api/early-userspace/early_userspace_support.html)

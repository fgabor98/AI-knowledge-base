---
status: draft
reviewed: false
domain: linux-userspace
difficulty: beginner
last_reviewed: null
---

# Stage 1: Linux Runtime, Filesystem, And Rootfs

Userspace programs do not run inside source trees. They run inside a process view of
a mounted filesystem, with a particular loader, libraries, configuration, device
nodes, credentials, and boot state. This chapter builds the filesystem and runtime
model that every later process and I/O chapter relies on.

## The central mental model

There are three distinct objects to keep separate:

```text
pathname  --resolved in a process's namespace-->  directory entry / inode
inode     --represented by a filesystem-->       file data and metadata
mount     --composes filesystems-->              the visible hierarchy
```

`/etc/app.conf` is not “a string pointing to a file.” It is a lookup performed using
the calling process’s root, current directory, mount namespace, symlink rules, and
permissions. The resulting object can be a regular file, directory, device node,
socket, FIFO, or another special file. A different service namespace can resolve the
same pathname to a different object—or to no object at all.

## Learning materials

1. [Filesystem Hierarchy And Path Resolution](filesystem-hierarchy-and-path-resolution.md)
2. [Files, Inodes, Metadata, And Links](files-inodes-metadata-and-links.md)
3. [Safe Paths And Temporary File Operations](safe-path-and-temporary-file-operations.md)
4. [Pseudo-filesystems And Device Nodes](pseudo-filesystems-and-device-nodes.md)
5. [Mounts, Initramfs, And Rootfs Layout](mounts-initramfs-and-rootfs-layout.md)
6. [Read-Only Rootfs, Overlayfs, And Persistent State](read-only-rootfs-overlayfs-and-persistent-state.md)
7. [ELF Executables And Dynamic Linking](elf-executables-and-dynamic-linking.md)

## Filesystem classes in a product

| Class | Typical locations | Lifetime and policy |
| --- | --- | --- |
| Immutable system | `/usr`, `/bin`, `/sbin`, `/lib*` | Delivered by the image; normally read-only and replaced by an update |
| Configuration | `/etc` or a separate configuration partition | Versioned, validated, backed up or regenerated according to product policy |
| Runtime state | `/run`, service sockets, PID files | Volatile; recreated at boot and never treated as durable configuration |
| Persistent data | `/var/lib`, `/data`, application partition | Survives reboot/update according to a schema and power-loss policy |
| Logs | `/var/log`, journal storage, remote sink | Bounded, rotated, and safe when storage is full or unavailable |
| Cache | `/var/cache`, application cache | Rebuildable; deletion must not destroy user intent |
| Temporary | `/tmp`, private `RuntimeDirectory`, `tmpfs` | Volatile and permission-controlled; never used for irreplaceable state |
| Kernel views | `/proc`, `/sys`, `/dev` | Generated or managed at runtime; not ordinary persistent files |

The location alone does not establish the policy. An embedded image may put `/var` on
tmpfs, use a read-only `/usr`, place persistent data under `/data`, or omit a
directory entirely. The image definition, mount table, and service contract are the
authority.

## Boot-to-application sequence

```text
firmware / bootloader
        |
        v
kernel + command line + initramfs
        |
        v
early userspace: discover storage, mount real root, hand off
        |
        v
PID 1 / init: mount pseudo-filesystems, create runtime dirs, start services
        |
        v
service: loader -> libc -> application -> devices and persistent state
```

Every arrow can fail independently. A service that starts too early can see an
existing directory but not the filesystem that should be mounted there. A program
that starts from an initramfs can find `/bin/app` but not the libraries or config that
exist only on the final root. A program can load successfully and still fail when
`/dev`, `/proc`, or `/sys` is not mounted.

## A filesystem investigation sequence

```sh
pwd
findmnt -T /etc/hostname
stat /etc/hostname
readlink -e /etc/hostname
cat /proc/self/mountinfo
df -hT
df -ih
namei -l /etc/hostname
```

Use `findmnt -T` or `/proc/self/mountinfo` to answer which mount supplies a path.
Use `namei` to expose each component and symlink. Use `stat` to inspect the resolved
object. These commands describe the shell’s namespace; inspect the service’s PID or
enter its namespace when debugging a sandboxed service.

## Stage lab

Inspect a new disposable directory from a shell:

```sh
lab_dir=$(mktemp -d /tmp/userspace-rootfs-lab.XXXXXX)
mkdir -p "$lab_dir/root/etc" "$lab_dir/data"
printf 'mode=lab\n' > "$lab_dir/root/etc/app.conf"
ln -s etc/app.conf "$lab_dir/root/app.conf"
namei -l "$lab_dir/root/app.conf"
stat -L "$lab_dir/root/app.conf"
findmnt -T "$lab_dir/root/etc/app.conf"
```

The symlink target is relative to its containing `root` directory, so
`etc/app.conf` reaches the intended file. `../etc/app.conf` would escape
that directory and normally be dangling in this fixture. `findmnt -T` reports
the filesystem containing the path; it does not claim the fixture is a mountpoint.
The lab leaves its private directory for inspection.

Then compare the host root with the target rootfs manifest. The important deliverable
is not a list of paths; it is a mapping from each path to its owner, mount, lifetime,
writability, and recovery policy.

## Completion criteria

You can complete this stage when you can:

- explain path resolution using a process’s root, current directory, dirfd, mounts,
  symlinks, and permissions;
- distinguish a pathname, directory entry, inode, open file description, mount, and
  device node;
- select race-resistant APIs for untrusted paths and safe temporary files;
- identify `/proc`, `/sys`, `/dev`, `tmpfs`, and `devtmpfs` by purpose and lifetime;
- explain initramfs handoff and why boot ordering changes path availability;
- classify data as immutable, configuration, runtime, cache, log, temporary, or
  persistent and choose a power-loss policy;
- diagnose an executable failure by inspecting its ELF interpreter and dependencies.

## Related topics

- [Stage 0: Environment And Mental Model](../environment-and-mental-model/index.md)
- [System Calls, Files, And File Descriptors](../system-calls-files-and-file-descriptors/index.md)
- [Filesystem Images](../../build-systems/filesystem-image-basics.md)
- [Install Rules And Staging](../../build-systems/install-rules-and-staging.md)

## References

- [Filesystem Hierarchy Standard](https://refspecs.linuxfoundation.org/fhs.shtml)
- [`path_resolution(7)`](https://man7.org/linux/man-pages/man7/path_resolution.7.html)
- [`mount_namespaces(7)`](https://man7.org/linux/man-pages/man7/mount_namespaces.7.html)
- [Linux kernel filesystems documentation](https://www.kernel.org/doc/html/latest/filesystems/index.html)

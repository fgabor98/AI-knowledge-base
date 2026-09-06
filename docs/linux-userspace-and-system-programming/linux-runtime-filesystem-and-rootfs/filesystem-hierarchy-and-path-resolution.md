---
status: draft
reviewed: false
domain: linux-userspace
difficulty: beginner
last_reviewed: null
---

# Filesystem Hierarchy And Path Resolution

## What problem does this solve?

Programs fail because a pathname is wrong, a component is missing, a symlink points
somewhere unexpected, a mount is absent, or the service sees a different namespace.
Path handling becomes predictable when you model lookup as a sequence of component
operations rather than treating a path as an opaque string.

## The standard hierarchy, with embedded variations

| Path | Usual role | Embedded caveat |
| --- | --- | --- |
| `/` | Root of the visible hierarchy | May initially be an initramfs and later replaced or overmounted |
| `/bin`, `/sbin` | Essential commands and administrative tools | Often symlinks into `/usr/bin` and `/usr/sbin` under merged-`/usr` layouts |
| `/usr` | Most installed programs, libraries, and read-only data | Commonly a separate read-only partition |
| `/lib`, `/lib64` | Essential shared libraries and loader paths | Exact names vary by architecture and ABI |
| `/etc` | Host configuration | May be immutable, generated, or on persistent storage |
| `/dev` | Device nodes and runtime device objects | Usually `devtmpfs` plus udev/mdev or a static policy |
| `/proc` | Process and kernel view | Requires a procfs mount in the relevant namespace |
| `/sys` | Device model, drivers, and kernel attributes | Requires sysfs and is not a generic configuration database |
| `/run` | Volatile boot/runtime state | Usually tmpfs and created early by init |
| `/var` | Variable data, logs, spool, state, and caches | May be split, compressed, or partly volatile |
| `/tmp` | General temporary files | Often tmpfs; permissions and cleanup policy matter |
| `/home` | User data | May be omitted on appliance targets |

Check the product image rather than assuming the desktop hierarchy. A minimal target
may omit man pages, shells, `/home`, package metadata, or even `/usr` as a separate
mount. The application should depend on an explicit product contract, not on a path
that happened to exist on the development workstation.

## How lookup starts

For a pathname:

- `/a/b` starts at the process’s root directory, which may differ from the host’s
  `/` after `chroot` or namespace isolation;
- `a/b` starts at the process’s current working directory;
- an empty pathname is invalid for ordinary path lookup;
- a path containing `.` keeps the current directory and `..` moves to its parent,
  subject to root and namespace boundaries;
- each component must be searchable (`x` permission) on the containing directory;
- the final component’s required permission depends on the operation, such as read,
  write, create, or execute;
- a symlink may redirect lookup to an absolute path or a path relative to the symlink;
- lookup can cross a mount point, while `..` from a mounted filesystem follows the
  mount topology rather than treating the mount as an ordinary directory.

This is why checking `access(path, ...)` and then using `open(path, ...)` is unsafe:
the object can change between the two lookups. Open the object once and check the
resulting FD or use descriptor-relative, race-resistant operations.

## Current directory and directory FDs

The current working directory is process state inherited across `fork` and preserved
across `exec` unless changed. A service started by a supervisor may have `/` as its
working directory; an interactive shell may have the source tree. Never make a
daemon’s configuration or data location depend on `cwd` unless the service contract
explicitly sets it.

A directory FD is a stable lookup anchor:

```c
int config_dir = open("/etc/my-service", O_RDONLY | O_DIRECTORY | O_CLOEXEC);
int fd = openat(config_dir, "settings.conf", O_RDONLY | O_CLOEXEC);
```

`openat` makes the relationship explicit and helps avoid races involving a mutable
absolute path. The directory FD remains meaningful even if the process changes its
current directory. Newer Linux applications can additionally use `openat2` with
resolution restrictions such as `RESOLVE_BENEATH` or `RESOLVE_NO_SYMLINKS` when the
target kernel supports them.

## Symlinks, bind mounts, and namespaces

A symlink is a pathname redirect. It has its own inode and does not contain the target
file’s data. `readlink` reads the link text without following it; `stat` follows the
final link; `lstat` reports the link itself. A bind mount exposes an existing object
at another mount point and can make two paths reach the same underlying object while
preserving different namespace views.

Namespaces virtualize global resources. A service may see a private mount namespace,
different `/proc`, or a restricted `/dev`; its `/etc/app.conf` is resolved inside that
view. Use `/proc/<pid>/root`, `/proc/<pid>/cwd`, `/proc/<pid>/mountinfo`, and namespace
handles when comparing a service with a shell.

## Useful commands

```sh
namei -l /var/lib/my-service/state.db
readlink /etc/resolv.conf
readlink -e /etc/resolv.conf
stat -Lc '%F %a %u:%g %s %n' /etc/resolv.conf
findmnt -T /var/lib/my-service/state.db
lsns -t mnt -p "$pid"
```

`readlink -e` requires every component to exist; that is useful for diagnosis but not
as a security check before a later open. `findmnt` and `lsns` may be absent on a
minimal image, in which case `/proc` is the fallback.

## Common mistakes

- Assuming `/bin`, `/lib`, or `/var` has the desktop layout.
- Building paths by concatenating untrusted strings with `/`.
- Using `access()` as a permission or existence pre-check.
- Following symlinks unintentionally in a privileged service.
- Assuming a host shell and a service share the same mount namespace or `cwd`.
- Treating a bind mount as a copy of the data.
- Using `realpath` as a substitute for an atomic open; the result can become stale.

## Debugging checklist

- Print the exact path, process root, `cwd`, UID/GIDs, and namespace IDs.
- Inspect every component with `namei -l` and the final object with `stat`/`lstat`.
- Identify the supplying mount with `findmnt -T` or `/proc/<pid>/mountinfo`.
- Check execute permission on directories, not only read permission on the final file.
- Confirm symlink targets and whether the service is allowed to cross them.
- Replace existence checks with one operation that obtains and validates the object.

## Related topics

- [Stage 1: Linux Runtime, Filesystem, And Rootfs](index.md)
- [Safe Paths And Temporary File Operations](safe-path-and-temporary-file-operations.md)
- [Files, Inodes, Metadata, And Links](files-inodes-metadata-and-links.md)
- [Mounts, Initramfs, And Rootfs Layout](mounts-initramfs-and-rootfs-layout.md)

## References

- [`path_resolution(7)`](https://man7.org/linux/man-pages/man7/path_resolution.7.html)
- [`openat(2)`](https://man7.org/linux/man-pages/man2/openat.2.html)
- [`openat2(2)`](https://man7.org/linux/man-pages/man2/openat2.2.html)
- [`mount_namespaces(7)`](https://man7.org/linux/man-pages/man7/mount_namespaces.7.html)

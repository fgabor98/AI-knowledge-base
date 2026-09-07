---
status: draft
reviewed: false
domain: linux-userspace
difficulty: beginner
last_reviewed: null
---

# Files, Inodes, Metadata, And Links

## What problem does this solve?

“The file” is an imprecise phrase. A directory entry maps a name to an inode; an
inode records metadata and references file data; an open FD refers to an open-file
description for that inode. Understanding the distinctions explains hard links,
unlink behavior, atomic replacement, device nodes, permissions, and why an open file
can survive after its name disappears.

## Names, inodes, and open state

```text
directory entry:     name --> inode number
inode:               type, mode, owner, times, size, link count, data mapping
open file description: inode + current offset + status flags
file descriptor:     process-local number --> open file description
```

Two names can refer to one inode through hard links. Two FDs can refer to the same
open-file description after `dup` or `fork`, sharing an offset and status flags. A
separate `open` of the same pathname normally creates a separate open-file
description. Later chapters develop the FD part; here the filesystem identity is the
focus.

## File types and metadata

The inode mode encodes the object type and permission bits:

| Type | `stat` display | Meaning |
| --- | --- | --- |
| Regular | `regular file` | Byte sequence stored by the filesystem |
| Directory | `directory` | Mapping from names to inodes |
| Symbolic link | `symbolic link` | Pathname text used during lookup |
| Character device | `character special file` | Driver-defined byte-oriented interface |
| Block device | `block special file` | Driver-defined block-oriented interface |
| FIFO | `fifo` | Kernel pipe with a pathname |
| Socket | `socket` | IPC/network endpoint represented in the filesystem |

Relevant metadata includes mode, UID/GID, size, link count, inode number, device ID,
access/modification/status-change timestamps, and filesystem-specific flags. The
meaning of timestamps and update granularity depends on filesystem and mount policy;
never use an mtime equality test as a perfect change detector.

```sh
stat file
stat -c 'type=%F mode=%A uid=%u gid=%g size=%s links=%h inode=%i dev=%D' file
stat -- symlink       # GNU stat: inspect the symlink itself
stat -L -- symlink    # GNU stat: follow the symlink
```

The C APIs `stat(2)` and `lstat(2)` respectively follow and inspect the final
symlink. GNU `stat(1)` has a different default, shown above; `lstat` is not
a standard shell command. `fstat(fd)` inspects the object already opened
and avoids a second pathname lookup.

## Hard links and unlink

Creating a hard link adds another directory entry for the same inode:

```sh
printf 'one inode\n' > original
ln original alias
stat original alias
rm original
cat alias
```

The data remains while a hard link **or** a live open/mapping reference keeps
the object alive. `unlink` removes a name; it does not necessarily erase data
immediately. A process can write an unlinked log or temporary file through its open
FD, while a new process cannot find it by pathname. This is useful for private
temporary storage, but it can also consume space invisibly until the owner exits.

Directories have special link-count rules and cannot be hard-linked by ordinary
applications. Cross-filesystem hard links fail because an inode cannot be shared that
way between filesystems.

## Symbolic links

Symbolic links are flexible but introduce lookup and security concerns:

- the target can be relative to the link’s containing directory;
- the target can be absent or change between inspection and use;
- loops and excessive link depth can cause lookup failure;
- a privileged program can be redirected to an attacker-controlled object;
- `O_NOFOLLOW` applies to the final component for `open`, not automatically to every
  path component.

Use `lstat`, `readlink`, `O_NOFOLLOW`, directory FDs, and `openat2` resolution policy
according to the threat model. Do not resolve a path into a string and then trust the
string later.

## Atomic namespace operations

`rename` changes directory names atomically within one filesystem. Readers see the
old name or the new name, not a half-written filename. It does not by itself make the
new file durable across power loss; use file and directory synchronization when the
product requires durability.

```text
write complete data to temporary file
        |
        v
fsync temporary file if required
        |
        v
rename temporary file over target
        |
        v
fsync containing directory if required
```

The temp file must be created in the target directory and have the intended mode.
Cross-filesystem rename fails with `EXDEV`; copying then deleting is not atomic.

## Permissions and ownership

The kernel evaluates mode bits, ACLs, credentials, capabilities, and security policy.
The process umask affects creation mode; it does not retroactively change an existing
file. A directory’s write and execute permissions control creation, removal, and
lookup independently of the file’s own mode.

For a service, document:

- the installation owner and mode;
- the runtime user and supplementary groups;
- which directories it may read or write;
- whether a privilege or capability is required;
- whether ownership changes happen during install, boot, or first use.

## Minimal C inspection pattern

```c
struct stat st;
if (fstat(fd, &st) == -1) {
    int saved_errno = errno;
    fprintf(stderr, "fstat: %s\n", strerror(saved_errno));
    return -1;
}

if (!S_ISREG(st.st_mode)) {
    fprintf(stderr, "expected regular file\n");
    return -1;
}
```

Validate the object after opening, not only before opening. A configuration reader
may reject directories, device nodes, or unexpected sizes even if the pathname was
correct.

## Common mistakes

- Equating a pathname with an inode or an FD.
- Assuming `unlink` immediately destroys open data.
- Using hard links as if they were independent copies.
- Relying on timestamps without considering filesystem granularity and clock policy.
- Checking mode bits while ignoring ACLs, capabilities, namespaces, or LSM policy.
- Writing a target file directly and assuming a crash cannot expose partial contents.
- Using `stat(path)` and then operating on `path` after an attacker can replace it.

## Debugging checklist

- Compare `stat` and `lstat` for suspicious links.
- Check inode, device, link count, type, owner, mode, and size.
- Use `fstat` on the actual FD and verify the type before consuming data.
- Check whether the object is on the expected mount and filesystem.
- Check deleted-but-open files with `lsof` or `/proc/<pid>/fd` when storage is full.
- Check directory permissions and service credentials.
- For replacement, verify temporary-file directory, rename result, and durability
  policy.

## Related topics

- [Stage 1: Linux Runtime, Filesystem, And Rootfs](index.md)
- [Filesystem Hierarchy And Path Resolution](filesystem-hierarchy-and-path-resolution.md)
- [Safe Paths And Temporary File Operations](safe-path-and-temporary-file-operations.md)
- [Durability, Locking, And Power Loss](../system-calls-files-and-file-descriptors/durability-locking-and-power-loss.md)

## References

- [`stat(2)`](https://man7.org/linux/man-pages/man2/stat.2.html)
- [`inode(7)`](https://man7.org/linux/man-pages/man7/inode.7.html)
- [`link(2)`](https://man7.org/linux/man-pages/man2/link.2.html)
- [`unlink(2)`](https://man7.org/linux/man-pages/man2/unlink.2.html)
- [`rename(2)`](https://man7.org/linux/man-pages/man2/rename.2.html)

---
status: draft
reviewed: false
domain: linux-userspace
difficulty: intermediate
last_reviewed: null
---

# Regular-File I/O And Metadata

## What problem does this solve?

Regular files look simple but involve buffering, offsets, permissions, metadata,
partial I/O, filesystem limits, and durability. A reliable program chooses between
stdio and descriptors deliberately and validates the object and result at every
boundary.

## `FILE *` versus file descriptors

`FILE *` provides libc buffering, formatted I/O, and line-oriented operations. A raw
FD provides descriptor-level control needed for `poll`, `fcntl`, `fsync`, `ioctl`,
`mmap`, and precise ownership. Do not mix them casually:

```c
FILE *stream = fdopen(fd, "r");
/* stream now owns the FD: fclose(stream) closes it. */
```

If both stdio and raw `read`/`write` operate on the same underlying object, buffered
data and offsets can disagree. Flush with `fflush` before switching in a carefully
controlled design, and define who owns closure. For binary or exact-offset I/O, raw
descriptors are usually clearer.

## Open and validate

```c
int fd = open(path, O_RDONLY | O_CLOEXEC);
if (fd == -1) {
    /* save errno */
}

struct stat st;
if (fstat(fd, &st) == -1 || !S_ISREG(st.st_mode)) {
    /* close fd and reject the object */
}
```

Creation mode is filtered by umask. Use `O_CREAT|O_EXCL` for exclusive creation,
`O_TRUNC` only when destruction of old contents is intended, and `O_APPEND` when
each write must be positioned at end. Do not use `O_TRUNC` as a substitute for
atomic replacement of configuration.

## Read and write loops

```c
for (;;) {
    ssize_t n = read(fd, buffer, sizeof buffer);
    if (n > 0) {
        consume(buffer, (size_t)n);
    } else if (n == 0) {
        break;
    } else if (errno == EINTR) {
        continue;
    } else {
        /* classify the saved errno */
        break;
    }
}
```

Writes need an offset and short-write loop. `pread`/`pwrite` avoid changing a shared
open-file offset. Check for overflow when converting sizes and offsets, and reject
files too large for the parser before allocating based on their size.

## Metadata operations

Useful interfaces include `stat`/`fstat`, `chmod`, `fchmod`, `chown`, `fchown`,
`utimensat`, `rename`, `unlink`, `link`, `mkdir`, and directory iteration. Metadata
updates have their own permissions and failure paths. An application should not
assume it can preserve ownership or timestamps after replacement unless install and
runtime policy grants it.

Directory FDs and `*at` interfaces keep operations anchored and reduce path races.
After replacing a file, check the resulting object through a fresh FD when the
consumer needs to know exactly what it opened.

## Atomic replacement

```text
create temp in destination directory
write complete validated bytes
fsync temp if required
rename temp over destination
fsync destination directory if required
```

This prevents readers from seeing an incompletely written named file. It does not
automatically preserve hard links, extended attributes, ownership, or power-loss
durability. Cross-filesystem replacement is not atomic.

## Direct and zero-copy I/O

`sendfile`, `splice`, `copy_file_range`, direct I/O, and memory mapping can reduce
copies or alter caching, but they add alignment, filesystem, fallback, and partial
progress rules. Use them only after measuring and documenting the target filesystem
and workload. A performance shortcut must retain the same error and durability proof.

## Common mistakes

- Mixing stdio buffering and raw descriptors without synchronization.
- Assuming one `read` or `write` transfers the requested length.
- Using `stat(path)` instead of validating the opened FD.
- Truncating a live configuration file before the replacement is ready.
- Treating metadata timestamps as a reliable transaction marker.
- Ignoring `ENOSPC`, `EDQUOT`, `EROFS`, and `EIO` during persistence.
- Introducing direct I/O or mmap without alignment and fallback tests.

## Debugging checklist

- Record path, FD, object type, mount, flags, offset, owner, and mode.
- Check short counts, EOF, `EINTR`, and conversion overflows.
- Inspect `/proc/<pid>/fdinfo` for offset and flags.
- Compare stdio and raw I/O ownership and buffering.
- Test concurrent readers, replacement, crash, read-only filesystems, and full disks.
- Check metadata and directory synchronization for durable replacement.

## Related topics

- [Stage 3: System Calls, Files, And File Descriptors](index.md)
- [File Descriptors And Open-File Descriptions](file-descriptors-and-open-file-descriptions.md)
- [Durability, Locking, And Power Loss](durability-locking-and-power-loss.md)
- [Safe Paths And Temporary File Operations](../linux-runtime-filesystem-and-rootfs/safe-path-and-temporary-file-operations.md)

## References

- [`open(2)`](https://man7.org/linux/man-pages/man2/open.2.html)
- [`read(2)`](https://man7.org/linux/man-pages/man2/read.2.html)
- [`write(2)`](https://man7.org/linux/man-pages/man2/write.2.html)
- [`stat(2)`](https://man7.org/linux/man-pages/man2/stat.2.html)
- [`stdio(3)`](https://man7.org/linux/man-pages/man3/stdio.3.html)

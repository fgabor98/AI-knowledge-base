---
status: draft
reviewed: false
domain: linux-userspace
difficulty: intermediate
last_reviewed: null
---

# Safe Paths And Temporary File Operations

## What problem does this solve?

Pathnames are mutable shared state. Between checking a path and using it, another
process can replace a component, redirect a symlink, or mount a different filesystem.
This is a time-of-check/time-of-use (TOCTOU) race. It becomes a security issue when a
service runs with privileges or writes into a shared directory, and a correctness
issue whenever a file can be rotated or replaced concurrently.

## The unsafe pattern

```c
if (access(path, W_OK) == 0) {
    int fd = open(path, O_WRONLY);
    /* The object checked may not be the object opened. */
}
```

The check adds a race and may also use different credentials or access semantics than
the real operation. Prefer one operation and handle its result:

```c
int fd = open(path, O_WRONLY | O_CLOEXEC | O_NOFOLLOW);
if (fd == -1) {
    /* Classify errno from open itself. */
}
```

If the application needs to verify type, owner, or device after opening, use
`fstat(fd, ...)`. If the operation is relative to a trusted directory, anchor it
with a directory FD and `openat`.

## Directory-relative operations

```c
int root_fd = open("/var/lib/my-service", O_RDONLY | O_DIRECTORY | O_CLOEXEC);
if (root_fd == -1) {
    /* report and stop */
}

int state_fd = openat(root_fd, "state.db", O_RDWR | O_CLOEXEC);
```

The trusted directory FD makes the intended scope visible. Use `mkdirat`, `unlinkat`,
`renameat`, `fstatat`, and `readlinkat` for the same reason. `AT_FDCWD` means the
current working directory; avoid it for security-sensitive operations when a stable
anchor is available.

On sufficiently new Linux targets, `openat2` can enforce policy in the kernel:

- `RESOLVE_BENEATH` prevents resolution from escaping beneath a directory FD;
- `RESOLVE_IN_ROOT` treats the directory FD as a temporary root;
- `RESOLVE_NO_SYMLINKS` rejects symlink traversal;
- `RESOLVE_NO_XDEV` prevents crossing mount points.

These are Linux-specific and must be isolated behind a capability/version check or a
minimum-kernel requirement. A fallback must preserve the security property; silently
downgrading to unsafe string checks is not a valid fallback.

## Untrusted path components

For a path supplied by a client, assume it may contain `..`, symlinks, empty
components, unusual bytes, control characters, a very long name, or a name that is
being replaced concurrently. Decide first whether the client should supply a
pathname at all. An object ID, pre-opened FD, or fixed allow-list can be safer.

If a pathname is required:

1. Define the trusted root directory.
2. Define whether symlinks and mount crossing are allowed.
3. Define maximum component and total lengths.
4. Use directory-relative APIs and kernel resolution restrictions where available.
5. Open with the required flags, then validate the resulting object with `fstat`.
6. Never use an untrusted path as a shell command or format string.

`realpath` can canonicalize a path, but it does not reserve the result. It is useful
for display and diagnostics, not as a substitute for an atomic secure open.

## Safe temporary files

Never construct a temporary name with `sprintf("/tmp/app-%d", getpid())`. Names can
collide, be guessed, or be replaced by another user. Use an API that creates the
object atomically:

```c
char template[] = "/tmp/my-service.XXXXXX";
int fd = mkstemp(template);
if (fd == -1) {
    /* report errno */
}

/* The fd is owned here. Unlink immediately if the file is private scratch space. */
if (unlink(template) == -1) {
    /* preserve the original creation result and handle cleanup */
}
```

`mkstemp` creates a regular file with exclusive creation semantics and returns an FD.
Its mode and the process umask still matter. Keep the template mutable and do not
assume the returned name is suitable for disclosure. Prefer a private service
runtime directory over shared `/tmp`; use `O_TMPFILE` where supported and useful.

For a file that will become a named configuration or state file:

```text
create exclusive temporary file in the target directory
write and validate complete contents
fchmod/fchown if policy requires it
fsync file if durability is required
rename temporary name over destination
fsync containing directory if durability is required
```

The temporary file must be on the same filesystem as the destination for atomic
rename. On failure, close and unlink it. A crash can leave a named temporary file, so
the startup policy should recognize and clean stale names safely.

## Permission and disclosure concerns

Temporary files can leak secrets through names, modes, contents, logs, backups, or
core dumps. Use a restrictive umask, avoid sensitive data in filenames, and decide
whether the file may be visible to other users. A private directory should have
appropriate ownership and mode before it is used.

Do not use `/tmp` as a lock directory, persistent store, or trusted location merely
because every process can reach it. World-writable directories are precisely where
symlink and replacement attacks are common.

## Common mistakes

- `access`/`stat` followed by `open` as if the pair were atomic.
- `mktemp`, PID-based names, or random names created without exclusive creation.
- `realpath` followed by a later open.
- Applying `O_NOFOLLOW` while assuming it protects intermediate components.
- Creating a temporary file in `/tmp` and renaming across filesystems.
- Forgetting to unlink a named temporary after an error or crash.
- Logging secret path components or file contents during diagnostics.

## Debugging checklist

- Identify the trusted directory and path source.
- Reproduce with a concurrent renamer or symlink in a disposable test directory.
- Capture `openat`/`openat2` flags with `strace`.
- Inspect the final object using `fstat` and compare owner, mode, type, and mount.
- Check umask, directory mode, service credentials, and namespace.
- Test crashes between write, `fsync`, rename, and directory synchronization.
- Confirm temporary files do not fill the target filesystem or survive unexpectedly.

## Related topics

- [Stage 1: Linux Runtime, Filesystem, And Rootfs](index.md)
- [Filesystem Hierarchy And Path Resolution](filesystem-hierarchy-and-path-resolution.md)
- [Files, Inodes, Metadata, And Links](files-inodes-metadata-and-links.md)
- [Durability, Locking, And Power Loss](../system-calls-files-and-file-descriptors/durability-locking-and-power-loss.md)

## References

- [`openat(2)`](https://man7.org/linux/man-pages/man2/openat.2.html)
- [`openat2(2)`](https://man7.org/linux/man-pages/man2/openat2.2.html)
- [`mkstemp(3)`](https://man7.org/linux/man-pages/man3/mkstemp.3.html)
- [`O_TMPFILE`](https://man7.org/linux/man-pages/man2/open.2.html)
- [OWASP: Path Traversal](https://owasp.org/www-community/attacks/Path_Traversal)

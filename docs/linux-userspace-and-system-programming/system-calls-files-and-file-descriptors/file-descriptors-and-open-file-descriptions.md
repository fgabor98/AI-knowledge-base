---
status: draft
reviewed: false
domain: linux-userspace
difficulty: intermediate
last_reviewed: null
---

# File Descriptors And Open-File Descriptions

## What problem does this solve?

Descriptor bugs come from confusing a process-local integer with the kernel state it
references. `dup`, `fork`, and `exec` make sharing and inheritance explicit concerns;
concurrent close/reuse makes stale descriptor integers dangerous.

## Two levels of state

```text
FD table entry:       open-file-description pointer + descriptor flags
open-file description: current offset + file status flags + object reference
object:                inode, pipe, socket, terminal, device, event source, ...
```

Descriptor flags include `FD_CLOEXEC`. File status flags include `O_APPEND` and
`O_NONBLOCK`; they belong to the open-file description and can be shared. The current
offset is also shared by descriptors referencing that description. `dup` creates a
new descriptor entry pointing to the same description. A separate `open` normally
creates a different description even for the same pathname.

## Creation and ownership

```c
int fd = open(path, O_RDONLY | O_CLOEXEC);
if (fd == -1) {
    /* save errno */
}
/* caller owns fd and must close it exactly once */
```

Prefer atomic creation flags: `O_CLOEXEC`, `O_NONBLOCK`, and `O_DIRECTORY` where
needed. The creator owns the FD until an explicit transfer. Use wrapper names such
as `take_fd`, `borrow_fd`, and `close_fd` to make ownership visible.

## Offsets and concurrent I/O

`read`, `write`, and `lseek` use the open-file description’s offset. Two threads or
processes sharing it can interfere with sequential operations. Use `pread`/`pwrite`
for position-specific operations that must not modify the shared offset. `O_APPEND`
requests that each write be positioned at the end as part of the write operation,
but does not make an arbitrary multi-write record protocol atomic.

For regular files, concurrent offset and append behavior also depends on kernel and
filesystem semantics. Define record boundaries, locking, or a single writer rather
than assuming “one write equals one log line” across all users.

## `dup`, `dup2`, and `dup3`

```c
if (dup2(input_fd, STDIN_FILENO) == -1) {
    /* child setup failed */
}
if (dup3(output_fd, STDERR_FILENO, O_CLOEXEC) == -1) {
    /* Linux-specific; check desired close-on-exec semantics */
}
```

`dup2` atomically closes the destination if needed and makes it refer to the source.
`dup3` rejects equal descriptors and can set `O_CLOEXEC`. After redirection, close
the original descriptor if it is no longer needed. In a child before `exec`, use
`_exit` on failure.

## Blocking status

`O_NONBLOCK` changes how operations report lack of immediate progress. It does not
make regular file access universally asynchronous and does not guarantee that a
device, socket, or pipe can complete a request. `fcntl(F_SETFL)` changes file status
flags on the shared open-file description, so changing flags in one owner can affect
another owner after `dup`/`fork`.

Readiness means an operation should not block at that instant. It does not guarantee
that a subsequent read/write succeeds, returns the requested size, or remains valid
after another thread consumes the data.

## Descriptor limits and leaks

```sh
ulimit -n
cat /proc/$$/limits
ls -l /proc/$$/fd
ls -l /proc/$$/fdinfo
```

`EMFILE` is the per-process limit; `ENFILE` is a system-wide limit. Leaks commonly
occur on retry paths, failed initialization, and children that inherit descriptors.
Track FD ownership in code review and test repeated start/stop cycles.

## Close-on-exec discipline

Set close-on-exec at creation time. A separate `fcntl(F_SETFD, FD_CLOEXEC)` can race
with another thread’s `fork`/`exec`. Deliberately clear it only for descriptors that
must cross `exec`, such as standard streams or a documented activation FD.

At process startup, validate standard FDs and decide whether missing 0/1/2 should be
opened to `/dev/null`, rejected, or supplied by the supervisor. Do not let accidental
descriptor numbers become an API.

## Modern handles

Linux offers `close_range` for bounded cleanup and pidfds for process references. They
are useful in carefully versioned Linux code, but do not replace ownership. A pidfd
avoids PID reuse; a descriptor still needs one owner and a close policy.

## Common mistakes

- Storing an FD after its owner closed it and assuming the integer remains valid.
- Assuming `dup` creates an independent offset or status flags.
- Changing `O_NONBLOCK` without realizing another owner shares the description.
- Setting close-on-exec after `open` in a multithreaded launcher.
- Assuming readiness means full successful I/O.
- Closing an FD from one thread while another thread may be using or waiting on it.
- Retrying `close` after failure and accidentally closing a reused descriptor.

## Debugging checklist

- Record FD number, creation site, owner, object type, flags, and transfer points.
- Inspect `/proc/<pid>/fd` and `fdinfo` before the process changes state.
- Check offsets and status flags after `dup`/`fork`.
- Test exec inheritance, repeated initialization, and FD-limit exhaustion.
- Check concurrent close/reuse and event-loop ownership rules.
- Use `fstat` to validate the object reached by a descriptor.

## Related topics

- [Stage 3: System Calls, Files, And File Descriptors](index.md)
- [Descriptor Inheritance And Redirection](descriptor-inheritance-and-redirection.md)
- [System-Call Contracts And Errors](system-call-contracts-and-errors.md)
- [Blocking, Nonblocking, And Partial I/O](blocking-nonblocking-and-partial-io.md)

## References

- [`open(2)`](https://man7.org/linux/man-pages/man2/open.2.html)
- [`dup(2)`](https://man7.org/linux/man-pages/man2/dup.2.html)
- [`fcntl(2)`](https://man7.org/linux/man-pages/man2/fcntl.2.html)
- [`close_range(2)`](https://man7.org/linux/man-pages/man2/close_range.2.html)

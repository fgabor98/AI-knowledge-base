---
status: draft
reviewed: false
domain: linux-userspace
difficulty: intermediate
last_reviewed: null
---

# System-Call Contracts And Errors

## What problem does this solve?

System calls are not ordinary Boolean functions. They can make partial progress,
block, be interrupted, leave side effects, or fail because the target’s kernel,
credentials, resources, or device state differ. Correct code checks the documented
return shape and turns the result into a deliberate state transition.

## libc wrappers versus direct system calls

Most applications call libc functions such as `open`, `read`, `clock_gettime`, and
`pthread_create`. libc may translate arguments, select an implementation, maintain
thread-local state, or use a vDSO fast path. A direct `syscall(2)` bypasses higher-level
wrappers and is generally appropriate only when a Linux interface has no libc wrapper
or a low-level ABI is explicitly required.

Direct system calls are difficult to make portable: argument types, structure layout,
architecture calling conventions, time64 transitions, restart behavior, and libc
bookkeeping matter. Do not replace a wrapper with `syscall` merely because its name
looks closer to the kernel.

## Return-value discipline

```c
ssize_t count = read(fd, buffer, capacity);
if (count > 0) {
    /* count bytes are valid */
} else if (count == 0) {
    /* EOF or interface-specific empty result */
} else {
    int saved_errno = errno;
    /* classify saved_errno before calling another function */
}
```

Common shapes include `-1`/`errno`, `NULL`, zero/nonzero, a positive count, a PID,
or a value with output fields. Read the RETURN VALUE section for each function. A
successful call may leave `errno` unchanged; `errno` is meaningful only after the
documented failure indication.

## Error categories

| Error | Meaning to investigate |
| --- | --- |
| `EINTR` | A signal interrupted the call; progress/side effects depend on the interface |
| `EAGAIN` / `EWOULDBLOCK` | Nonblocking operation cannot progress now |
| `ETIMEDOUT` | A specified wait or protocol deadline expired |
| `ECONNRESET` / `EPIPE` | Peer or stream lifecycle ended |
| `ENODEV` / `ENXIO` | Device or requested hardware resource is unavailable |
| `EIO` | Lower-level I/O error; preserve context and consider recovery |
| `ENOSPC` / `EDQUOT` | Storage, quota, or filesystem capacity exhausted |
| `EMFILE` / `ENFILE` | Per-process or system-wide FD limit exhausted |
| `ENOMEM` | Allocation or kernel resource unavailable |
| `EINVAL` | Invalid argument or state; usually a programming/contract error |
| `EACCES` / `EPERM` | Permission, credential, capability, or policy failure |
| `ENOSYS` | Kernel/libc interface is unavailable in the deployed environment |
| `EOPNOTSUPP` / `ENOTTY` | Object or implementation does not support the requested operation |

An error category is a policy input. Retrying `EINVAL` is not recovery; retrying
`EAGAIN` without readiness or a bound can create a busy loop.

## `EINTR` and restart policy

Some blocking calls may return `EINTR`, some may be transparently restarted depending
on signal disposition and flags, and some interfaces have side effects before the
interruption. A safe loop preserves the operation’s state and deadline:

```text
deadline = monotonic_now + budget
while incomplete:
    result = operation(remaining(deadline))
    if EINTR: retry only if the contract permits
    if short success: advance by the returned amount
    if timeout: transition to timed-out state
    otherwise: classify and stop
```

Do not restart blindly after a partially completed write, device command, or
transaction. Make retries idempotent or include a request ID/sequence number.

## Feature visibility and ABI types

Headers may hide declarations behind feature-test macros. Use the target headers and
compiler mode, and compile with warnings that reject implicit declarations. Use
`size_t` for object sizes, `ssize_t` for signed byte counts, `off_t` for file offsets,
and the UAPI’s fixed-width types where specified. Never cast away a width mismatch to
silence a warning.

Time interfaces have 32/64-bit and clock-domain considerations. Use the interface
and types supported by the target libc/kernel, and do not serialize an internal
`struct timespec` layout as a product protocol without defining widths and encoding.

## Cleanup errors

The primary operation and cleanup both have contracts. `close` can report an earlier
write error on some filesystems; `fsync` can fail after data was accepted; `munmap`
and `pthread_*` functions have their own return conventions. Preserve the first
failure, report material cleanup failures, and do not retry `close` on an FD that may
have been reused.

```c
int saved_errno = errno;
if (close(fd) == -1) {
    fprintf(stderr, "close failed: %s\n", strerror(errno));
}
errno = saved_errno;
```

## Contract table

```text
Call and object:
Success shape:
Failure sentinel:
Meaning of zero:
Can block:
Can return partial progress:
May be interrupted/cancelled:
Side effects before failure:
Retryable errors and bound:
Deadline source:
Owner before/after:
Cleanup and cleanup errors:
Evidence to log:
```

Fill this out for every hardware, file, process, and IPC boundary rather than only
for the most obvious call.

## Common mistakes

- Treating all system calls as `0` success or `-1` failure.
- Reading `errno` after logging or cleanup.
- Retrying every `EINTR` without considering side effects or deadlines.
- Using `int` for byte counts, offsets, or sizes.
- Calling raw `syscall` when a libc wrapper supplies ABI adaptation.
- Ignoring errors from `close`, `fsync`, `pthread_*`, and `munmap`.
- Returning a generic error without preserving the operation and target context.

## Debugging checklist

- Capture call, arguments, return value, saved `errno`, duration, and process identity.
- Check short counts, EOF, interruption, and side effects.
- Check target kernel/libc versions and feature macros.
- Reproduce capacity, permission, missing-device, timeout, and peer-death cases.
- Verify retry bounds and monotonic deadlines.
- Preserve the first failure when cleanup produces a second failure.

## Related topics

- [Stage 3: System Calls, Files, And File Descriptors](index.md)
- [Blocking, Nonblocking, And Partial I/O](blocking-nonblocking-and-partial-io.md)
- [File Descriptors And Open-File Descriptions](file-descriptors-and-open-file-descriptions.md)
- [POSIX, Linux, libc, And Manual Pages](../environment-and-mental-model/posix-linux-libc-and-manual-pages.md)

## References

- [`syscall(2)`](https://man7.org/linux/man-pages/man2/syscall.2.html)
- [`errno(3)`](https://man7.org/linux/man-pages/man3/errno.3.html)
- [`intro(2)`](https://man7.org/linux/man-pages/man2/intro.2.html)
- [`signal(7)`](https://man7.org/linux/man-pages/man7/signal.7.html)

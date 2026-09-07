---
status: draft
reviewed: false
domain: linux-userspace
difficulty: beginner
last_reviewed: null
---

# POSIX, Linux, libc, And Manual Pages

## What problem does this solve?

The name of a function does not tell you the complete contract. `open()` may be
declared by libc headers, specified by POSIX, implemented using a Linux system call,
and used against a product-specific device node. Each layer can add constraints or
extensions. This page teaches you to identify the authoritative contract before
writing code or promising portability.

## Contract layers

| Layer | Example | What to ask |
| --- | --- | --- |
| ISO C | `malloc`, `memcpy`, `fopen`, `printf` | What does the language/library standard guarantee? |
| libc implementation | glibc, musl, uClibc-ng | Which implementation, version, headers, ABI, and extensions are present? |
| POSIX | `pthread_create`, `open`, `clock_gettime` | What behavior is portable across conforming Unix-like systems? |
| Linux system interface | `epoll`, `eventfd`, `signalfd`, `pidfd_open` | Which Linux kernel version and configuration are required? |
| Kernel UAPI | `/dev`, `ioctl`, sysfs, netlink, V4L2, ALSA | What is the intentionally exposed binary or textual interface? |
| Init/distribution | systemd unit, BusyBox init, package layout | How are credentials, mounts, limits, logs, and restart policy supplied? |
| Product contract | calibration file, board wiring, update state | Which assumptions belong to this product and must be versioned or tested? |

“Portable Linux” is not one layer. A program may be portable at the POSIX source
level while its deployment depends on an ELF ABI, a particular libc symbol version,
systemd, `/sys`, or a vendor UAPI.

## Read a manual page as a contract

Start with the section, not a web search snippet:

```sh
man 2 open       # system-call interface
man 3 pthread_create
man 5 proc       # file format or filesystem documentation
man 7 socket     # overview/concepts
```

Read in this order:

1. **NAME and SYNOPSIS:** Identify the declaration, required headers, types, and
   feature-test macros. The synopsis is not decoration; it tells you which ABI types
   and compile-time gates are involved.
2. **DESCRIPTION:** Determine what the operation acts on and what state it changes.
3. **RETURN VALUE:** Record every success shape, including short counts, zero as EOF,
   `NULL`, and sentinel values. Never infer success from `errno`.
4. **ERRORS:** Separate expected operational conditions such as `EAGAIN` from
   programmer errors such as `EINVAL`, and from deployment conditions such as
   `ENOSYS` or `ENODEV`.
5. **NOTES and BUGS:** Look for Linux-specific behavior, historical traps, alignment,
   signal restart, thread cancellation, and version requirements.
6. **ATTRIBUTES / MT-Safe:** Check whether the interface is safe to call from
   multiple threads and whether it is a cancellation point.
7. **VERSIONS and STANDARDS:** Record when the interface appeared and whether it is
   POSIX, Linux, GNU, or another extension.
8. **SEE ALSO:** Follow the referenced overview and related interface before deciding
   how to use the function.

For a target bug, read the target’s manual pages or libc documentation when
available. A host’s man page may describe a newer kernel or a different libc than the
target.

## Feature-test macros

Feature-test macros select declarations and constants exposed by system headers. They
are compile-time policy, not runtime capability checks. Define them before any system
header, preferably in the source or central build flags:

```c
#define _POSIX_C_SOURCE 200809L
#include <fcntl.h>
#include <unistd.h>
```

Typical choices include:

- `_POSIX_C_SOURCE 200809L` for a deliberate POSIX.1-2008 source boundary;
- `_XOPEN_SOURCE` for interfaces gated by an X/Open profile;
- `_GNU_SOURCE` when intentionally using GNU/Linux extensions;
- `_DEFAULT_SOURCE` for the libc implementation’s normal default set.

Do not scatter feature macros inconsistently across translation units. A header may
expose one declaration in one file and a different declaration or no declaration in
another. Treat the chosen macro set as part of the build contract, and use compiler
warnings to catch implicit declarations.

Feature visibility does not prove runtime availability. A declaration can compile
while the target kernel is too old, the libc is missing a symbol, or a service sandbox
disallows the operation. Conversely, a runtime `ENOSYS` or `EOPNOTSUPP` is evidence
about the deployed environment, not something a feature macro can prevent.

## Return values and `errno`

The standard pattern is:

```c
int fd = open(path, O_RDONLY | O_CLOEXEC);
if (fd == -1) {
    int saved_errno = errno;
    fprintf(stderr, "open %s: %s\n", path, strerror(saved_errno));
    return -1;
}
```

Important rules:

- Check the function’s documented failure sentinel first.
- Read `errno` immediately after that failure, before another call can overwrite it.
- Save `errno` before cleanup or logging if the original cause matters.
- Do not test `errno == 0` to decide whether a call succeeded; successful calls may
  leave an old value unchanged.
- Do not assume numeric values are portable; use symbolic names.
- Only rely on errors documented for that interface. An implementation detail from
  one libc or driver is not a general contract.
- `EAGAIN` and `EWOULDBLOCK` may have the same value, so handle them as the same
  nonblocking condition when the interface documents either.

For count-returning interfaces, success is often a range, not a Boolean:

```c
ssize_t n = read(fd, buffer, capacity);
if (n > 0) {
    /* n bytes are valid; n may be less than capacity. */
} else if (n == 0) {
    /* End of stream or interface-specific empty result. */
} else {
    /* n == -1; inspect errno. */
}
```

Do not reuse a buffer or advance a protocol state machine until the returned count
has been interpreted.

## Cancellation and thread safety

An interface can be thread-safe yet still be a cancellation point. A thread cancelled
while it owns a mutex or an FD can leave the process inconsistent unless cleanup was
designed around that point. Read the attributes and cancellation notes for blocking
POSIX functions, and prefer cooperative shutdown with explicit state transitions.

Thread-safe also does not mean logically safe. Two calls may be individually safe but
still race when they operate on a shared file offset, shared protocol state, or a
resource whose lifetime another thread controls.

## POSIX versus Linux extensions

Choose the narrowest interface that satisfies the product:

```text
portable requirement --> POSIX interface --> Linux extension --> product UAPI
```

This is a decision sequence, not a hierarchy of quality. `poll()` may be sufficient
for a small portable utility; `epoll` may be the appropriate Linux choice for many
descriptors; a subsystem-specific `ioctl` may be unavoidable for hardware control.
Document the choice and the minimum target requirement.

When using an extension, isolate it behind a small adapter. The rest of the program
can then test a semantic interface such as `sensor_read_sample()` rather than making
every module know about `ioctl` numbers, sysfs paths, or Linux-only types.

## Minimal comparison exercise

Compare these sources before implementing a feature:

```sh
man 2 open
man 3 fopen
man 7 path_resolution
cc -dM -E -x c /dev/null | sort | less
```

Answer in writing:

1. Which interfaces are ISO C, POSIX, or Linux-specific?
2. What does `O_CLOEXEC` protect against, and at which boundary is it applied?
3. Which calls can block or be interrupted?
4. Which return values mean partial success?
5. Which behavior comes from the target’s filesystem or driver rather than the
   function’s general contract?

## API contract template

For each interface used by a service, record:

```text
Interface:       read(2) on the sensor FD
Contract layer:  POSIX semantics plus the sensor subsystem UAPI
Preconditions:   fd is open, buffer has capacity, device is configured
Success:         1..capacity bytes; format and timestamp are validated separately
Empty result:    0 means end-of-stream / not a sample (confirm UAPI)
Failure:         EINTR, EAGAIN, ENODEV, EIO, ...
Blocking:        yes in blocking mode; bounded by the surrounding deadline
Ownership:       caller owns buffer and FD; driver owns device state
Cancellation:    shutdown path must not leak fd or leave state half-configured
Recovery:        retry EAGAIN, reinitialize on disconnect, fail closed on bad data
Evidence:        fd, device path, sequence, monotonic timestamp, errno, duration
```

The template exposes assumptions that would otherwise be hidden in a call site.

## Common mistakes

- Using a prototype copied from a blog instead of the target headers and manual page.
- Adding `_GNU_SOURCE` everywhere without recording why a GNU extension is needed.
- Treating a declared function as proof that the target supports it.
- Reading `errno` after `printf`, `close`, or another cleanup call.
- Ignoring short reads, short writes, `EINTR`, or cancellation because the desktop
  test usually completes in one call.
- Assuming “MT-Safe” means a higher-level protocol is race-free.
- Mixing POSIX and Linux types or structure layouts across an ABI boundary.

## Debugging checklist

- Identify the exact manual-page section and implementation version.
- Check headers, feature macros, compiler mode, and warnings for implicit declarations.
- Capture the function’s return value and saved `errno` at the failure site.
- Check whether the failure is compile-time visibility, link-time symbol resolution,
  runtime kernel support, permissions, or UAPI semantics.
- Keep a portability note beside every Linux-only or product-specific call.
- Test with missing, interrupted, short, timed-out, and disconnected conditions.

## Related topics

- [Stage 0: Environment And Mental Model](index.md)
- [System-Call Contracts And Errors](../system-calls-files-and-file-descriptors/system-call-contracts-and-errors.md)
- [C Memory Model And Concurrency](../../c/advanced-c/c-memory-model-and-concurrency.md)
- [Cross Compilation](../../build-systems/cross-compilation.md)

## References

- [The Open Group Base Specifications, Issue 8](https://pubs.opengroup.org/onlinepubs/9799919799/)
- [GNU C Library: Feature Test Macros](https://sourceware.org/glibc/manual/latest/html_node/Feature-Test-Macros.html)
- [GNU C Library: Checking for Errors](https://sourceware.org/glibc/manual/latest/html_node/Checking-for-Errors.html)
- [`errno(3)`](https://man7.org/linux/man-pages/man3/errno.3.html)
- [`feature_test_macros(7)`](https://man7.org/linux/man-pages/man7/feature_test_macros.7.html)

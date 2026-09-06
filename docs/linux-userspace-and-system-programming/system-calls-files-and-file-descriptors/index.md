---
status: draft
reviewed: false
domain: linux-userspace
difficulty: intermediate
last_reviewed: null
---

# Stage 3: System Calls, Files, And File Descriptors

File descriptors are the common currency of Linux userspace: regular files, pipes,
sockets, terminals, device nodes, event sources, and many kernel objects are all
accessed through descriptor-based interfaces. Reliable code treats each call as a
contract with return values, partial progress, blocking, ownership, and durability
consequences.

## The descriptor model

```text
process descriptor table
  fd 0  ----+                         +--> open-file description
  fd 1  --- |-------------------------+       current offset
  fd 2  --- |                         |       file status flags
  fd 3  ------------------------------+       inode/socket/pipe/device state
```

The descriptor number is a process-local handle. The open-file description is kernel
state that can be shared by `dup`, `fork`, and related operations. A descriptor can
be closed and its number immediately reused, so an integer stored without ownership
and lifetime rules is not a safe resource identity.

## Learning materials

1. [System-Call Contracts And Errors](system-call-contracts-and-errors.md)
2. [File Descriptors And Open-File Descriptions](file-descriptors-and-open-file-descriptions.md)
3. [Descriptor Inheritance And Redirection](descriptor-inheritance-and-redirection.md)
4. [Regular-File I/O And Metadata](regular-file-io-and-metadata.md)
5. [Durability, Locking, And Power Loss](durability-locking-and-power-loss.md)
6. [Pipes, FIFOs, And Backpressure](pipes-fifos-and-backpressure.md)
7. [Blocking, Nonblocking, And Partial I/O](blocking-nonblocking-and-partial-io.md)

## I/O proof obligations

For every descriptor operation, answer:

| Obligation | Example question |
| --- | --- |
| Identity | What object does this FD refer to, and how was that verified? |
| Ownership | Which component closes it, and when is that transfer complete? |
| Flags | Is it blocking, append-only, close-on-exec, read/write, or sync? |
| Progress | Can a call return short success, zero, or readiness without completion? |
| Failure | Which errors are retryable, terminal, or programmer bugs? |
| Lifetime | Can another thread close or reuse the FD while this operation runs? |
| Ordering | What must happen before a command, response, close, rename, or commit? |
| Durability | Does success mean memory accepted bytes, filesystem committed them, or power-loss survival? |
| Shutdown | How does a blocked operation wake and how is the peer notified? |

## Three different meanings of “done”

```text
write returned                 -> the interface accepted some bytes
fsync returned                 -> the filesystem reported durability to its contract
device protocol acknowledged   -> the device accepted a command
```

These may be different events. A pipe write can block because the reader is slow. A
socket write can succeed before a remote peer receives data. A file write can return
before power-loss persistence. A driver write can queue work while hardware is still
busy. The API and product protocol must define which event the caller needs.

## Study method

1. Read the exact man page and identify libc, POSIX, Linux, or subsystem layers.
2. Draw the FD and open-file-description ownership graph.
3. Write the complete loop for short reads/writes and interrupted calls.
4. Add an absolute deadline and a shutdown path for every blocking wait.
5. Test descriptor inheritance and closure with `/proc/<pid>/fd`.
6. Separate data transfer, filesystem durability, and application commit.

## Stage lab

```sh
cc -std=c11 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -g \
    examples/c/linux-userspace-fd-pipes.c -o /tmp/fd-pipes
/tmp/fd-pipes
```

Inspect the same concepts in a shell pipeline:

```sh
printf 'alpha\nbeta\n' | wc -l
```

The shell created a pipe and connected standard input/output. Reproduce that wiring
in C, then deliberately keep one write end open and observe why the reader does not
receive EOF.

## Completion criteria

You can complete this stage when you can:

- distinguish libc wrappers, system calls, descriptor tables, and open-file state;
- preserve and classify `errno`, `EINTR`, `EAGAIN`, short counts, and EOF;
- prevent FD leaks across `exec` and reason about `dup`/`fork` sharing;
- perform safe regular-file replacement and define metadata/durability policy;
- explain why advisory locks do not enforce cooperation;
- construct a pipe/FIFO protocol with EOF, broken-pipe, backpressure, and shutdown;
- implement bounded blocking/nonblocking I/O around a monotonic deadline.

## Related topics

- [Stage 2: Processes And Program Lifetime](../processes-and-program-lifetime/index.md)
- [Stage 4: Process Memory And Mapping](../process-memory-and-mapping/index.md)
- [Stage 7: IPC And Event-Driven Design](../ipc-and-event-driven-design/index.md)
- [Stage 1: Linux Runtime, Filesystem, And Rootfs](../linux-runtime-filesystem-and-rootfs/index.md)

## References

- [Linux man-pages project](https://www.kernel.org/doc/man-pages/)
- [`open(2)`](https://man7.org/linux/man-pages/man2/open.2.html)
- [`fcntl(2)`](https://man7.org/linux/man-pages/man2/fcntl.2.html)
- [`pipe(7)`](https://man7.org/linux/man-pages/man7/pipe.7.html)

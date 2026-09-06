---
status: draft
reviewed: false
domain: linux-userspace
difficulty: intermediate
last_reviewed: null
---

# Pipes, FIFOs, And Backpressure

## What problem does this solve?

Pipes are simple byte streams with finite kernel buffers. They naturally provide
backpressure, but the producer and consumer must close the right ends, define message
boundaries, and handle EOF, broken pipes, partial I/O, and shutdown. A named FIFO adds
pathname and open-order behavior that can surprise service code.

## Anonymous pipes

```c
int pipe_fds[2];
if (pipe2(pipe_fds, O_CLOEXEC) == -1) {
    /* report errno */
}
/* pipe_fds[0] is read; pipe_fds[1] is write */
```

An anonymous pipe is a kernel buffer with a read end and write end. After `fork`, each
process owns copies of both descriptors until it closes the ends it does not use.
The reader sees EOF only after every write reference is closed. A writer with no
readers receives `SIGPIPE` by default or `EPIPE` if SIGPIPE is ignored/blocked.

## Stream framing

A pipe preserves byte order, not application messages. One `write` does not imply one
`read`. Define a framing protocol:

```text
fixed-size record       -> read exactly the fixed size
length + payload        -> validate length before allocation/read
delimiter                -> handle delimiter split across reads
close                    -> EOF means no more bytes, not necessarily an error
```

Bound maximum frame size and reject malformed lengths. For bidirectional request/
response, a `socketpair` often communicates intent better than two pipes.

## Backpressure

When the pipe buffer is full, a blocking writer sleeps. With `O_NONBLOCK`, it returns
`EAGAIN`. This is useful flow control: the producer must wait, shed work, coalesce,
or fail according to policy. An unbounded user-space queue merely moves the memory
problem out of the kernel.

```text
producer --> bounded queue --> pipe/socket --> consumer
                 ^                              |
                 +------ slow consumer --------+
```

Choose queue bounds from memory budget and worst-case service time. Expose drops,
rejections, and queue depth in diagnostics; silent loss makes backpressure invisible.

## FIFOs

A FIFO is a named pipe created with `mkfifo`. Its pathname is discoverable and subject
to permissions and path races. Opening a FIFO read-only or write-only can block until
the peer opens the other side, depending on flags and implementation. Use a private
directory and explicit startup/shutdown protocol; do not use a world-writable FIFO
as an authentication boundary.

```sh
mkfifo /tmp/example.pipe
```

For services, Unix-domain sockets usually provide clearer peer lifecycle and
credentials. Use FIFOs for simple, intentionally stream-like compatibility paths.

## Shutdown sequence

```text
producer stops accepting new work
producer drains or explicitly drops bounded queue
producer closes write end
consumer drains remaining bytes
consumer observes EOF and exits
parent waits/reaps both sides
```

If a blocked writer must wake during shutdown, close or signal through an owned
mechanism and define who performs that action. Closing an FD from another thread can
have reuse races; event loops should own descriptor operations or use a separate
wakeup FD.

## `SIGPIPE`

A process writing after all readers close may be terminated by SIGPIPE before it can
inspect `EPIPE`. A service can ignore or block SIGPIPE and handle `EPIPE`, or use
socket options such as `MSG_NOSIGNAL` where applicable. The policy must be consistent
with other libraries in the process.

## Common mistakes

- Forgetting one inherited write end and waiting forever for EOF.
- Assuming pipe writes and reads preserve message boundaries.
- Ignoring `SIGPIPE`/`EPIPE` and misreporting peer shutdown as a crash.
- Using an unbounded queue in front of a slow consumer.
- Opening a FIFO from a service without defining peer absence and startup order.
- Treating a FIFO pathname as authenticated or race-free.
- Closing descriptors from unrelated threads without an ownership protocol.

## Debugging checklist

- Inspect `/proc/<pid>/fd` and identify every pipe end owner.
- Check pipe capacity and blocked system calls with `strace`.
- Test slow consumer, full buffer, short writes, EOF, SIGPIPE, EPIPE, and peer crash.
- Test malformed and oversized frames.
- Verify queue bounds, drop policy, and shutdown drain behavior.
- Consider Unix sockets when peer identity or bidirectional protocol matters.

## Related topics

- [Stage 3: System Calls, Files, And File Descriptors](index.md)
- [Descriptor Inheritance And Redirection](descriptor-inheritance-and-redirection.md)
- [Blocking, Nonblocking, And Partial I/O](blocking-nonblocking-and-partial-io.md)
- [Pipes, Socketpairs, And Unix Sockets](../ipc-and-event-driven-design/pipes-socketpairs-and-unix-sockets.md)

## References

- [`pipe(7)`](https://man7.org/linux/man-pages/man7/pipe.7.html)
- [`pipe(2)`](https://man7.org/linux/man-pages/man2/pipe.2.html)
- [`fifo(7)`](https://man7.org/linux/man-pages/man7/fifo.7.html)
- [`signal(7)`](https://man7.org/linux/man-pages/man7/signal.7.html)

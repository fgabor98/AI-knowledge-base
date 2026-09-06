---
status: draft
reviewed: false
domain: linux-userspace
difficulty: intermediate
last_reviewed: null
---

# Blocking, Nonblocking, And Partial I/O

## What problem does this solve?

An I/O call can wait, return fewer bytes than requested, be interrupted, or report
that it cannot progress now. Event-driven code is correct only when it treats
readiness as a hint to attempt I/O and handles every result without losing framing,
deadlines, or ownership.

## Blocking versus nonblocking

A blocking descriptor waits inside an operation until progress, end-of-stream, an
error, or an event-specific condition occurs. A nonblocking descriptor returns
immediately when it cannot progress, commonly with `EAGAIN` or `EWOULDBLOCK`.

Nonblocking does not mean asynchronous completion or a hard execution-time bound.
For interfaces that implement it, the operation avoids waiting for ordinary data
readiness. Regular-file I/O generally ignores `O_NONBLOCK`; page faults, internal
locks, and driver behavior may still delay a call. The program needs a readiness
mechanism, a retry policy, and a deadline appropriate to the actual object.

## Partial I/O

```c
size_t sent = 0U;
while (sent < length) {
    ssize_t n = write(fd, data + sent, length - sent);
    if (n > 0) {
        sent += (size_t)n;
        continue;
    }
    if (n == -1 && errno == EINTR) {
        continue;
    }
    if (n == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
        /* Wait for writable readiness, preserving sent and the deadline. */
        break;
    }
    /* The operation is incomplete or failed. */
    break;
}
```

A stream reader must preserve incomplete frames across calls. `read == 0` generally
means EOF for a stream, but a device or special file may define another meaning.
Use the object’s contract. Never process uninitialized bytes beyond the returned count.

## Readiness APIs

`select`, `poll`, `ppoll`, and `epoll` report that descriptors have an event worth
trying. Readiness can be invalidated by another thread, another consumer, a peer
close, or a state change before the I/O call. Always handle `EAGAIN`, short counts,
EOF, and errors after readiness.

Level-triggered loops process until `EAGAIN` or a bounded work budget. Edge-triggered
loops must drain the source or risk missing another notification. `EPOLLERR` and
`EPOLLHUP` are reported as conditions to inspect; they do not replace reading the
pending error or data.

## Timeouts and signals

Use an absolute monotonic deadline across all retries:

```text
deadline = clock_gettime(CLOCK_MONOTONIC) + budget
while incomplete:
    remaining = deadline - now
    if remaining <= 0: timeout
    wait(remaining)
    if EINTR: recompute remaining; do not reset deadline
    attempt I/O; preserve partial count
```

`poll`/`ppoll` timeout units and rounding matter. A timeout usually means the caller
stopped waiting; an in-flight lower-layer operation may still exist. Pair timeout
with cancellation, request IDs, or a state transition that prevents late data from
being accepted.

## Fairness and backpressure

An event loop that drains one busy FD forever can starve all others. Bound bytes,
messages, or time spent per readiness event, then return to the wait set. For output,
enable writable interest only while data is queued; always-writable descriptors can
otherwise cause a wakeup storm.

## Nonblocking connect and accept

Nonblocking `connect` can return `EINPROGRESS`; wait for writability and inspect
`SO_ERROR` to determine whether it completed. A writable event alone is not success.
Nonblocking `accept` can race with another consumer or a connection disappearing;
handle `EAGAIN`, `EINTR`, and resource exhaustion. Apply the same rules to device and
FIFO descriptors after confirming that the object supports readiness.

## Common mistakes

- Assuming readiness means the full requested operation will complete.
- Treating `EAGAIN` as a fatal error or spinning without a wait.
- Resetting the timeout after every interruption or short transfer.
- Losing partial frame bytes between event-loop iterations.
- Using edge-triggered epoll without draining until `EAGAIN`.
- Leaving writable interest enabled with an empty output queue.
- Treating timeout as proof that a device or peer stopped processing.

## Debugging checklist

- Log FD, event mask, requested bytes, returned bytes, errno, and remaining deadline.
- Test short reads/writes with a deliberately small pipe/socket buffer.
- Test `EINTR`, `EAGAIN`, EOF, peer reset, full queue, and descriptor closure.
- Check level/edge-trigger mode and drain behavior.
- Check event-loop fairness and CPU use under a permanently ready FD.
- Verify shutdown wakes every blocked wait and does not reuse an FD accidentally.

## Related topics

- [Stage 3: System Calls, Files, And File Descriptors](index.md)
- [System-Call Contracts And Errors](system-call-contracts-and-errors.md)
- [Pipes, FIFOs, And Backpressure](pipes-fifos-and-backpressure.md)
- [Event Loops, select, poll, And epoll](../ipc-and-event-driven-design/event-loops-select-poll-and-epoll.md)
- [Clocks, Time Bases, And Deadlines](../time-clocks-and-signals/clocks-time-bases-and-deadlines.md)

## References

- [`poll(2)`](https://man7.org/linux/man-pages/man2/poll.2.html)
- [`epoll(7)`](https://man7.org/linux/man-pages/man7/epoll.7.html)
- [`read(2)`](https://man7.org/linux/man-pages/man2/read.2.html)
- [`write(2)`](https://man7.org/linux/man-pages/man2/write.2.html)

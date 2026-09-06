---
status: draft
reviewed: false
domain: linux-userspace
difficulty: intermediate
last_reviewed: null
---

# eventfd, timerfd, signalfd, And inotify

## What problem does this solve?

Linux exposes counters, timers, signals, and filesystem notifications as file
descriptors. This lets one event loop own the wait set, but each FD has different
read formats, coalescing, overflow, and lifetime semantics.

## `eventfd`

An eventfd contains a 64-bit counter. A write adds to it; a read returns and clears
the counter in normal mode, or returns one unit in semaphore mode. It is a wakeup or
count channel, not a byte stream and not a general payload transport.

Use it to notify an event loop that work is available in a protected queue. Define
whether multiple notifications coalesce and ensure producers cannot overflow the
counter. Create with `EFD_CLOEXEC` and `EFD_NONBLOCK` as appropriate.

## `timerfd`

`timerfd_create` makes timer expiration readable. A read returns the number of
expirations since the last read. Use the right clock and an absolute/periodic arm
policy. If the count is greater than one, record an overrun and apply the work policy;
do not silently execute one task as if no ticks were missed.

## `signalfd`

Block selected signals and create a signalfd for that mask. Add it to the loop and
read `signalfd_siginfo` records. The signals must remain blocked in threads that
should not handle them; create the mask before spawning workers. The event loop owns
translation from signal to shutdown/reload/reap state.

## `inotify`

Inotify reports filesystem events such as create, modify, move, delete, and queue
overflow. It reports changes, not complete durable state. An editor may write a temp
file and rename it; multiple events can coalesce; a watched directory can be moved or
deleted; and the queue can overflow. On `IN_Q_OVERFLOW`, rescan authoritative state.

Never treat “file modified” as “configuration is valid.” Read, parse, validate, and
atomically publish a new configuration generation.

## Common event-loop rules

- set close-on-exec at creation;
- use nonblocking mode when the handler must drain without blocking;
- read until `EAGAIN` where the event model requires draining;
- bound records, work, and queue depth;
- close/unregister from the owner thread or define reuse rules;
- preserve generation/context when an FD is replaced.

## Common mistakes

- Treating eventfd as a payload queue.
- Reading only one timerfd expiration and hiding overruns.
- Creating signalfd without blocking the same signals in workers.
- Treating inotify as a complete configuration database.
- Ignoring queue overflow and watch invalidation.
- Closing an event FD while another thread can receive a stale readiness event.

## Debugging checklist

- Record FD type, flags, owner, event count, and generation.
- Test counter overflow, timer overrun, signal bursts, and inotify overflow.
- Inspect `/proc/<pid>/fdinfo` and event-loop registrations.
- Test peer/process shutdown and FD close/reuse.
- Rescan state after any notification ambiguity or overflow.

## Related topics

- [Stage 7: IPC And Event-Driven Design](index.md)
- [Event Loops: select, poll, And epoll](event-loops-select-poll-and-epoll.md)
- [Signal-Safe Shutdown And Event Integration](../time-clocks-and-signals/signal-safe-shutdown-and-event-integration.md)
- [Timers And Periodic Work](../time-clocks-and-signals/timers-and-periodic-work.md)

## References

- [`eventfd(2)`](https://man7.org/linux/man-pages/man2/eventfd.2.html)
- [`timerfd_create(2)`](https://man7.org/linux/man-pages/man2/timerfd_create.2.html)
- [`signalfd(2)`](https://man7.org/linux/man-pages/man2/signalfd.2.html)
- [`inotify(7)`](https://man7.org/linux/man-pages/man7/inotify.7.html)

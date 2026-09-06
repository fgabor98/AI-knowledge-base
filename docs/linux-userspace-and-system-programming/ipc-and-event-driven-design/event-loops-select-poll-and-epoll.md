---
status: draft
reviewed: false
domain: linux-userspace
difficulty: intermediate
last_reviewed: null
---

# Event Loops: select, poll, And epoll

## What problem does this solve?

An event loop waits on many I/O sources without dedicating one thread per source. Its
correctness depends on readiness semantics, nonblocking drain loops, fairness,
descriptor ownership, deadlines, and shutdown.

## API choices

| API | Model | Main tradeoff |
| --- | --- | --- |
| `select` | Rebuild FD sets each wait | Portable but FD-set size and scan limits |
| `poll` | Array of `pollfd` entries | Simple, scans the array each wait |
| `ppoll` | `poll` plus signal mask/time precision | Safer signal/deadline integration |
| `epoll` | Kernel interest and ready lists | Linux-specific and scalable, more lifecycle rules |
| `pselect` | `select` plus signal mask | Atomic signal-mask transition |

Use the simplest API that meets descriptor count and target constraints. An event
loop’s complexity comes from protocol/state ownership, not merely the wait syscall.

## Level and edge triggering

Level-triggered readiness remains reported while the condition persists. Edge-triggered
readiness reports transitions, so a handler must drain reads/writes until `EAGAIN` or
it can miss future work. One-shot modes require explicit rearming after the handler
returns.

## Handler pattern

```text
wait with a monotonic deadline
for each ready FD:
    if error/hangup: inspect pending data/error, then close or reconnect
    if readable: read/drain bounded input, parse frames, enqueue bounded work
    if writable: write/drain bounded output; disable write interest when empty
    if timer/signal/wakeup: consume and update state
return to wait
```

Readiness does not guarantee success. A peer can close, another consumer can drain,
or a device can report an error before the operation. Every handler handles `EAGAIN`,
EOF, short counts, and errors.

## Fairness and budgets

Limit bytes, messages, or time spent per FD. A permanently readable source must not
starve control traffic. Avoid always monitoring `EPOLLOUT`; it is commonly ready and
can cause a busy loop. Bound parsing and queue admission before returning to wait.

## Signals and races

Use `pselect`/`ppoll` with signal masks or `signalfd` so signal arrival and entering a
wait do not have a flag-check race. The same event loop should own wakeup, timer,
signal, and descriptor registration when possible.

## Close and FD reuse

If one thread closes an FD while an event loop has a pending event, the numeric FD can
be reused for a different object. The stale event can then be applied to the wrong
connection. Centralize close/unregister/reuse, use generation tokens in user data,
and ensure the event loop owns descriptor lifetime.

## Common mistakes

- Using edge-triggered epoll without draining to `EAGAIN`.
- Treating `EPOLLIN` as complete message availability.
- Leaving `EPOLLOUT` armed with no pending output.
- Ignoring `EPOLLERR`, `EPOLLHUP`, or `EPOLLRDHUP`.
- Closing/reusing FDs across threads without event ownership.
- Letting one busy source starve all others.
- Checking a signal flag and entering `poll` without an atomic mask transition.

## Debugging checklist

- Record wait timeout, FD, generation, event mask, operation result, and queue depth.
- Test readiness followed by peer close, `EAGAIN`, short I/O, and error.
- Test edge/level/one-shot modes and rearming.
- Test fairness under a permanently ready source.
- Test signal, timer, wakeup, close/reuse, and shutdown races.
- Inspect `/proc/<pid>/fd` and event registration ownership.

## Related topics

- [Stage 7: IPC And Event-Driven Design](index.md)
- [eventfd, timerfd, signalfd, And inotify](eventfd-timerfd-signalfd-and-inotify.md)
- [Blocking, Nonblocking, And Partial I/O](../system-calls-files-and-file-descriptors/blocking-nonblocking-and-partial-io.md)
- [Signal-Safe Shutdown And Event Integration](../time-clocks-and-signals/signal-safe-shutdown-and-event-integration.md)

## References

- [`select(2)`](https://man7.org/linux/man-pages/man2/select.2.html)
- [`poll(2)`](https://man7.org/linux/man-pages/man2/poll.2.html)
- [`epoll(7)`](https://man7.org/linux/man-pages/man7/epoll.7.html)
- [`epoll_ctl(2)`](https://man7.org/linux/man-pages/man2/epoll_ctl.2.html)

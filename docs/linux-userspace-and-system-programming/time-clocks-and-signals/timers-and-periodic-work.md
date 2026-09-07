---
status: draft
reviewed: false
domain: linux-userspace
difficulty: intermediate
last_reviewed: null
---

# Timers And Periodic Work

## What problem does this solve?

Sleeping for a period after work completes accumulates drift. Timer signals can
overrun, be delivered to an unexpected thread, or complicate shutdown. Periodic work
needs an explicit time base, missed-period policy, ownership, and observability.

## Periodic scheduling

Prefer an absolute schedule:

```text
next = monotonic_now + period
loop:
    wait until next
    now = monotonic_now
    run bounded work
    missed = floor((now - next) / period) when now > next
    record missed
    next += (missed + 1) * period
```

This avoids drift from work duration. Choose what to do after an overrun:

- skip missed ticks and run once for the current state;
- run every missed tick if each represents required work and backlog is bounded;
- coalesce work into “latest state” semantics;
- enter degraded mode or report a deadline failure.

Never let an unbounded catch-up loop starve shutdown or other events.

## Timer mechanisms

| Mechanism | Strength | Caution |
| --- | --- | --- |
| `clock_nanosleep` | Simple one-owner periodic loop | Thread remains dedicated to the wait |
| POSIX timer | Expiration notification and timer IDs | Signal/thread notification ownership is subtle |
| `timerfd` | FD integrates with `poll`/`epoll` | Linux-specific; must read expiration count |
| `alarm`/`setitimer` | Legacy simple process timers | Limited semantics and signal coupling |
| Hardware/watchdog timer | External recovery or hardware deadline | Product/driver UAPI and reset semantics apply |

For an event-driven service, `timerfd_create` with `CLOCK_MONOTONIC` or
`CLOCK_BOOTTIME` can make timer expiry an ordinary event. A read returns an
expiration count; if the loop was delayed, consume it and apply the missed-period
policy rather than pretending one expiration occurred.

## Timer ownership

A timer owner controls creation, arming, disarming, event consumption, and closure.
Do not let one thread close a timer FD while another is processing events. A timer
callback must not outlive the object it references. Associate a generation or context
ID when timers can be cancelled and recreated quickly.

## Watchdogs and deadlines

A watchdog is a recovery mechanism, not proof that work is correct. Feed it only
after the service has demonstrated meaningful health: event loop progress, device
communication, queue bounds, and required persistence. A separate thread that feeds
the watchdog while the main service is dead defeats the safety purpose.

Define startup grace, normal interval, suspend behavior, hardware reset behavior, and
what evidence is preserved before a reset.

## Jitter and measurement

Measure release time, start time, completion time, period, and deadline miss. Jitter
comes from scheduling, interrupts, page faults, locks, CPU frequency, thermal policy,
and I/O. Average period is not a worst-case guarantee. Test under realistic load and
power modes.

## Common mistakes

- Sleeping for `period` after work and accumulating drift.
- Ignoring timer expiration counts and overruns.
- Running unbounded catch-up after a long stall.
- Feeding a watchdog from a thread that does not prove service health.
- Using realtime for a periodic elapsed-time loop.
- Closing/cancelling a timer from a thread that does not own its event source.
- Treating timer expiration as task completion.

## Debugging checklist

- Record clock, period, absolute deadlines, actual release/completion, and overruns.
- Check timer FD reads and expiration counts.
- Test signal interruption, delayed scheduling, suspend/resume, and clock changes.
- Test cancellation while work is queued or in flight.
- Verify watchdog feed criteria and reset evidence.
- Check queue/backpressure policy when periodic producers outrun consumers.

## Related topics

- [Stage 5: Time, Clocks, And Signals](index.md)
- [Clocks, Time Bases, And Deadlines](clocks-time-bases-and-deadlines.md)
- [Timers, eventfd, signalfd, And inotify](../ipc-and-event-driven-design/eventfd-timerfd-signalfd-and-inotify.md)
- [Watchdog And Control Interfaces](../hardware-facing-userspace-and-kernel-uapi/can-watchdog-and-control-interfaces.md)

## References

- [`timerfd_create(2)`](https://man7.org/linux/man-pages/man2/timerfd_create.2.html)
- [`timer_create(2)`](https://man7.org/linux/man-pages/man2/timer_create.2.html)
- [`clock_nanosleep(2)`](https://man7.org/linux/man-pages/man2/clock_nanosleep.2.html)
- [`time(7)`](https://man7.org/linux/man-pages/man7/time.7.html)

---
status: draft
reviewed: false
domain: linux-userspace
difficulty: intermediate
last_reviewed: null
---

# Stage 5: Time, Clocks, And Signals

Time is not one global number, and a signal is not a general-purpose message queue.
Reliable userspace code chooses the right clock for the question, expresses waits as
bounded deadlines, and turns asynchronous signals into controlled state transitions.

## Two axes of time

```text
REALTIME       calendar / externally synchronized / can jump
MONOTONIC      elapsed time since a kernel-defined point / does not go backward
BOOTTIME       monotonic-style elapsed time including suspend
RAW            hardware-derived monotonic measurement, without NTP adjustments
```

Use `CLOCK_REALTIME` for timestamps that humans or protocols interpret as calendar
time. Use `CLOCK_MONOTONIC` for ordinary timeouts. Use `CLOCK_BOOTTIME` when a timeout
must include system suspend. State the clock domain in every timestamped interface.

## Learning materials

1. [Clocks, Time Bases, And Deadlines](clocks-time-bases-and-deadlines.md)
2. [Timers And Periodic Work](timers-and-periodic-work.md)
3. [Signal Model And sigaction](signal-model-and-sigaction.md)
4. [Signal-Safe Shutdown And Event Integration](signal-safe-shutdown-and-event-integration.md)

## The event model

```text
clock/deadline --> wait --> event/signal --> inspect state --> bounded work
                                      \--> recompute deadline
```

Signals can interrupt a wait, but they do not carry a reliable application payload by
default. Timers can produce expirations faster than a consumer handles them. A
correct loop records state and counts, drains pending events, and remains bounded.

## Timing contract

For each time-sensitive operation record:

| Field | Decision |
| --- | --- |
| Clock | Which clock and why? |
| Domain | Calendar timestamp, elapsed timeout, device timestamp, or CPU time? |
| Deadline | Absolute deadline and behavior after expiry |
| Suspend | Does suspend consume the budget? |
| Adjustment | Can synchronization or manual changes move the clock? |
| Resolution | What precision is meaningful and what jitter is acceptable? |
| Overrun | What happens when periodic work misses one or more periods? |
| Shutdown | How is a blocked wait interrupted safely? |

## Stage lab

```sh
cc -std=c11 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -g \
    examples/c/linux-userspace-time-signal.c -o /tmp/time-signal
/tmp/time-signal
```

Send SIGINT during the run and observe that the handler only records a flag while
the main loop performs shutdown work.

## Completion criteria

You can complete this stage when you can:

- choose wall-clock, monotonic, boottime, raw, CPU, and device time correctly;
- implement an absolute deadline that survives `EINTR` without extending the budget;
- design periodic work without accumulating drift or hiding overruns;
- install signal dispositions with `sigaction` and respect async-signal safety;
- integrate signals through a self-pipe or `signalfd` without unsafe handlers;
- define signal, timer, shutdown, reload, and fatal-error ownership.

## Related topics

- [Stage 3: System Calls, Files, And File Descriptors](../system-calls-files-and-file-descriptors/index.md)
- [Stage 6: Threads And Userspace Concurrency](../threads-and-userspace-concurrency/index.md)
- [Stage 7: IPC And Event-Driven Design](../ipc-and-event-driven-design/index.md)
- [Timers And Periodic Work](timers-and-periodic-work.md)

## References

- [`time(7)`](https://man7.org/linux/man-pages/man7/time.7.html)
- [`clock_gettime(2)`](https://man7.org/linux/man-pages/man2/clock_gettime.2.html)
- [`signal(7)`](https://man7.org/linux/man-pages/man7/signal.7.html)
- [POSIX signal concepts](https://pubs.opengroup.org/onlinepubs/9799919799/basedefs/V1_chap03.html)

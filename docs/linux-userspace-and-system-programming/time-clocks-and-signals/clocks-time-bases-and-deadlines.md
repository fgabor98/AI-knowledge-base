---
status: draft
reviewed: false
domain: linux-userspace
difficulty: intermediate
last_reviewed: null
---

# Clocks, Time Bases, And Deadlines

## What problem does this solve?

Using calendar time for a timeout makes behavior dependent on NTP, RTC correction,
manual clock changes, and suspend. Mixing device timestamps with host timestamps
makes latency and freshness calculations meaningless. This page defines the clock
and deadline choices that make timing behavior explainable.

## Clock selection

| Clock | Use | Important property |
| --- | --- | --- |
| `CLOCK_REALTIME` | Calendar timestamps, human logs, wall-clock protocols | Can jump forward/backward |
| `CLOCK_MONOTONIC` | Most elapsed-time deadlines and intervals | Does not go backward during normal operation; suspend behavior must be considered |
| `CLOCK_BOOTTIME` | Budgets that include suspend | Includes time spent suspended |
| `CLOCK_MONOTONIC_RAW` | Measurement of raw clock behavior | Not adjusted like the normal monotonic clock; Linux-specific |
| `CLOCK_PROCESS_CPUTIME_ID` | CPU consumed by the process | Does not measure wall time |
| `CLOCK_THREAD_CPUTIME_ID` | CPU consumed by one thread | Does not measure waiting or device latency |

Check target kernel/libc support and whether the product has a time namespace or
virtualized clock. `clock_gettime` can be accelerated through vDSO, but its contract
is still the selected clock’s semantics.

## Absolute deadlines

```text
deadline = clock_now(CLOCK_MONOTONIC) + 500 ms
repeat:
    remaining = deadline - clock_now(CLOCK_MONOTONIC)
    if remaining <= 0: report timeout
    wait for at most remaining
    if interrupted: repeat with the same deadline
```

Absolute deadlines avoid timeout extension when a wait is interrupted or a loop does
small pieces of work. Recompute after every return. Convert time units with overflow
checks; a millisecond-to-nanosecond multiplication can overflow a 32-bit or signed
intermediate.

`clock_nanosleep` with `TIMER_ABSTIME` expresses this directly. For `poll`, `ppoll`,
and other APIs with relative timeouts, compute remaining time yourself.

## Clock domains at interfaces

Every timestamp should identify its domain and units. A sensor may timestamp samples
with its own oscillator; a network packet may carry UTC; a kernel event may use host
monotonic time. Do not subtract values from different domains without synchronization
and conversion. Store both a sequence/freshness indicator and a timestamp when stale
data is dangerous.

## Suspend and clock adjustment

If the system suspends, a monotonic timeout may or may not include suspend according
to the chosen clock and platform behavior. Choose `BOOTTIME` when “five seconds after
request” includes suspend. Use `REALTIME` only when the requirement is calendar based.

NTP can slew or step realtime. A backwards wall-clock jump can make a relative loop
wait too long; a forward jump can make certificates or scheduled jobs appear expired.

## Logging timing evidence

Log the clock domain, operation start/end, deadline, duration, sequence, and result.
Human-readable realtime is useful context but insufficient for duration diagnosis.
Avoid logging every timer tick on constrained flash; aggregate counts and retain
deadline-miss evidence.

## Minimal C pattern

```c
struct timespec deadline;
clock_gettime(CLOCK_MONOTONIC, &deadline);
deadline.tv_nsec += 500000000L;
if (deadline.tv_nsec >= 1000000000L) {
    deadline.tv_sec += 1;
    deadline.tv_nsec -= 1000000000L;
}

for (;;) {
    int result = clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME,
                                 &deadline, NULL);
    if (result == EINTR) {
        continue;
    }
    if (result != 0) {
        /* POSIX pthread-style calls return an error number directly. */
    }
    break;
}
```

Do not assume every POSIX interface reports errors through `errno`; read its return
contract. Normalize time arithmetic in a helper and test boundary carries/borrows.

## Common mistakes

- Using `time()` or realtime for an elapsed timeout.
- Restarting an interrupted wait with the original full duration.
- Comparing UTC, monotonic, and device timestamps as if they share an epoch.
- Ignoring suspend behavior in a watchdog or deadline.
- Logging wall-clock time without a monotonic duration.
- Performing nanosecond arithmetic through overflowing narrow integers.
- Assuming timer resolution equals scheduling or hardware accuracy.

## Debugging checklist

- Record clock ID, start, deadline, end, duration, and suspend state.
- Check `clock_getres` and target clock support.
- Test forward/backward realtime changes in a controlled environment.
- Test `EINTR`, scheduler delay, suspend/resume, and deadline expiry.
- Verify all producers and consumers use the same timestamp domain.
- Check time synchronization, RTC, PTP/NTP, and device clock assumptions separately.

## Related topics

- [Stage 5: Time, Clocks, And Signals](index.md)
- [Timers And Periodic Work](timers-and-periodic-work.md)
- [System-Call Contracts And Errors](../system-calls-files-and-file-descriptors/system-call-contracts-and-errors.md)
- [Serial Protocols, Timeouts, And Testing](../terminals-ttys-and-serial-userspace/serial-protocols-timeouts-and-testing.md)

## References

- [`clock_gettime(2)`](https://man7.org/linux/man-pages/man2/clock_gettime.2.html)
- [`clock_nanosleep(2)`](https://man7.org/linux/man-pages/man2/clock_nanosleep.2.html)
- [`time(7)`](https://man7.org/linux/man-pages/man7/time.7.html)
- [`clock_getres(2)`](https://man7.org/linux/man-pages/man2/clock_getres.2.html)

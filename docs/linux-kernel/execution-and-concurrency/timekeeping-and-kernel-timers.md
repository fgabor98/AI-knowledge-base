---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# Timekeeping And Kernel Timers

## What Problem Does This Solve?

Kernel drivers need safe ways to measure time, schedule timeouts, delay briefly, and avoid busy waiting.

## Core Concepts

- jiffies
- monotonic time
- real time
- `ktime_t`
- standard timers
- hrtimers
- sleeping delays
- busy-wait delays
- time comparison helpers

## Mental Model

Use the time API that matches the job: timers for future callbacks, sleeping delays for sleepable contexts, busy waits only for short hardware timing requirements, and monotonic time for elapsed intervals.

## Practice Skeleton

- Replace open-coded jiffies comparisons with helper macros.
- Add a standard timeout.
- Add an hrtimer for a precision case.
- Audit delay calls by context.

## Debugging Checklist

- Check wraparound-safe comparisons.
- Check whether the current context may sleep.
- Check units and conversion helpers.
- Avoid long busy waits.

## Related Topics

- [Timers](timers.md)
- [Hrtimers](hrtimers.md)
- [Sleepable Vs Atomic Code](sleepable-vs-atomic-code.md)

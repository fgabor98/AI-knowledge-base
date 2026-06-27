---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# Hrtimers

## What Problem Does This Solve?

High-resolution timers provide precise timer callbacks when jiffies-based timers are not accurate enough.

## Core Concepts

- high-resolution timer
- monotonic time
- relative expiry
- absolute expiry
- callback context
- restart behavior
- cancellation
- precision and overhead

## Mental Model

Use hrtimers for precise timing, not for sleepable work. If the callback needs to sleep, use the hrtimer only to schedule sleepable follow-up work.

## Practice Skeleton

- Initialize an hrtimer.
- Start it with a relative timeout.
- Restart it from the callback.
- Cancel it during module removal.

## Debugging Checklist

- Check callback context.
- Check time unit conversions.
- Cancel timers before freeing state.
- Avoid excessive high-frequency timers.

## Related Topics

- [Timers](timers.md)
- [Timekeeping And Kernel Timers](timekeeping-and-kernel-timers.md)
- [Workqueues](workqueues.md)

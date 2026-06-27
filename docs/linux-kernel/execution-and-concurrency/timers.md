---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# Timers

## What Problem Does This Solve?

Kernel timers schedule callbacks for future execution, commonly for timeouts, periodic polling, and debounce logic.

## Core Concepts

- timer callback
- jiffies
- high-resolution timers
- delayed work
- cancellation
- timer lifetime
- process context alternatives

## Mental Model

Timers are asynchronous callbacks. They are useful for deadlines and wakeups, but any sleepable follow-up work must be moved elsewhere.

## Practice Skeleton

- Add a one-shot timeout.
- Add periodic delayed work.
- Cancel timers during remove.
- Validate behavior under rapid bind and unbind.

## Debugging Checklist

- Check callback context before calling APIs.
- Confirm timers are canceled before freeing state.
- Avoid periodic timers when hardware interrupts are available.
- Check time unit conversions.

## Related Topics

- [Workqueues](workqueues.md)
- [Context Rules](context-rules.md)
- [Reference Counting And Lifetime](reference-counting-and-lifetime.md)

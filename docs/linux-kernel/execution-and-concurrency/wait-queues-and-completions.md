---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# Wait Queues And Completions

## What Problem Does This Solve?

Wait queues and completions let sleepable code wait for a condition or one-time event without busy waiting.

## Core Concepts

- wait queue
- condition predicate
- wakeup
- completion
- timeout
- interruptible sleep
- poll integration
- lost wakeups

## Mental Model

The condition is the truth. Wakeups are only notifications that the condition may have changed.

## Practice Skeleton

- Wait for data from an interrupt handler.
- Add timeout handling.
- Implement `poll` for a character device.
- Convert a one-time hardware-ready event to a completion.

## Debugging Checklist

- Check the condition under the right lock.
- Handle signals and timeouts.
- Avoid sleeping while holding a spinlock.
- Check for teardown paths that must wake waiters.

## Related Topics

- [Character Device Basics](../fundamentals/character-device-basics.md)
- [IRQ Handling](../driver-interfaces/irq-handling.md)
- [Locking And Atomics](locking-and-atomics.md)

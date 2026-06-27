---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# Bottom Halves, Softirqs, And Tasklets

## What Problem Does This Solve?

Bottom-half mechanisms defer interrupt-related work out of the hard IRQ path.

## Core Concepts

- top half
- bottom half
- softirq
- tasklet
- threaded IRQ
- workqueue
- atomic context
- deprecation and migration concerns

## Mental Model

Hard IRQ code should be short. Deferred work belongs in the mechanism that matches the context requirement: threaded IRQ or workqueue for sleepable work, softirq-style mechanisms for atomic deferred work.

## Practice Skeleton

- Identify hard IRQ work that should be deferred.
- Compare tasklet and workqueue constraints.
- Move sleepable work to a threaded IRQ or workqueue.
- Confirm teardown cancels deferred callbacks.

## Debugging Checklist

- Check whether the deferred handler can sleep.
- Check CPU serialization assumptions.
- Check teardown ordering.
- Prefer modern alternatives when tasklets are not required by existing code.

## Related Topics

- [IRQ Handling](../driver-interfaces/irq-handling.md)
- [Threaded Interrupts](../driver-interfaces/threaded-interrupts.md)
- [Workqueues](workqueues.md)

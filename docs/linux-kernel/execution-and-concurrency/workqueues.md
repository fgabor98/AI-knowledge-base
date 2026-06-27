---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# Workqueues

## What Problem Does This Solve?

Workqueues defer work from contexts that cannot sleep or should not run long operations inline.

## Core Concepts

- work item
- delayed work
- system workqueues
- custom workqueues
- cancellation
- flushing
- teardown ordering
- work item lifetime

## Mental Model

Workqueues run later in process context. That solves context constraints but introduces lifetime and teardown constraints.

## Practice Skeleton

- Schedule work from an IRQ handler.
- Add delayed work for polling.
- Cancel and flush work during remove.
- Confirm no work runs after driver data is freed.

## Debugging Checklist

- Check whether work can requeue itself.
- Use the right cancel or flush primitive.
- Stop hardware before freeing state used by work.
- Avoid unbounded work in shared system workqueues.

## Related Topics

- [Threaded Interrupts](../driver-interfaces/threaded-interrupts.md)
- [Reference Counting And Lifetime](reference-counting-and-lifetime.md)
- [Driver Binding, Probe, And Remove](../fundamentals/driver-binding-probe-remove.md)

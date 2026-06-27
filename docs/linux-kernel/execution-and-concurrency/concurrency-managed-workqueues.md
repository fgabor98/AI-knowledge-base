---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# Concurrency Managed Workqueues

## What Problem Does This Solve?

Concurrency managed workqueues let the kernel manage worker creation and concurrency so deferred work can run efficiently without each driver creating ad hoc threads.

## Core Concepts

- work item
- worker pool
- shared workqueue
- dedicated workqueue
- ordered workqueue
- delayed work
- flushing
- cancellation
- CPU affinity overview

## Mental Model

Use the common workqueue infrastructure unless the driver has a clear ordering, concurrency, or isolation requirement that needs a dedicated queue.

## Practice Skeleton

- Queue work on a system workqueue.
- Create a dedicated workqueue.
- Use delayed work.
- Flush and destroy the queue during teardown.

## Debugging Checklist

- Check whether work can requeue itself.
- Check cancellation and flush semantics.
- Avoid long blocking work on shared queues.
- Use ordered workqueues only when ordering is required.

## Related Topics

- [Workqueues](workqueues.md)
- [Bottom Halves, Softirqs, And Tasklets](bottom-halves-softirqs-and-tasklets.md)
- [Reference Counting And Lifetime](reference-counting-and-lifetime.md)

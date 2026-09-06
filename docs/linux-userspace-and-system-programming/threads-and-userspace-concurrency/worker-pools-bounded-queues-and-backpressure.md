---
status: draft
reviewed: false
domain: linux-userspace
difficulty: intermediate
last_reviewed: null
---

# Worker Pools, Bounded Queues, And Backpressure

## What problem does this solve?

Worker threads can keep an event loop responsive, but an unbounded queue only hides a
slow consumer until memory is exhausted. A production pool needs a bound, admission
policy, shutdown marker, ownership transfer, and worker failure behavior.

## Queue contract

```text
producer owns item before enqueue
successful enqueue transfers item ownership
worker owns item while processing
worker releases or returns item exactly once
full queue causes block, reject, drop, coalesce, or priority policy
```

The queue’s mutex protects its indices, count, storage, and shutdown predicate.
Condition variables wake producers when space exists and consumers when work exists.
The capacity is part of the system’s memory budget.

## Shutdown markers

Use a `stopping` predicate or explicit marker. A typical sequence is:

```text
stop accepting external work
set stopping under queue lock
wake all producers and consumers
workers drain accepted work or discard by policy
workers exit after queue empty and stopping is true
join every worker
destroy queue storage and synchronization objects
```

Do not destroy a queue while a worker can still wake and access it. Decide whether
shutdown drains, cancels, or rejects queued operations and how in-flight device work
is cancelled.

## Backpressure choices

| Condition | Policy | Tradeoff |
| --- | --- | --- |
| Queue full | Block producer | Preserves work but can propagate latency/deadlock |
| Queue full | Reject | Explicit overload; caller must handle it |
| Queue full | Drop newest | Keeps old work; may be stale |
| Queue full | Drop oldest | Keeps latest work; loses history |
| Queue full | Coalesce | Efficient for state updates; unsuitable for commands |
| Queue full | Priority admission | Protects critical work; can starve low priority |

Choose by semantic type. “Set current brightness” can coalesce; “erase record” cannot
be silently dropped. Expose queue depth, rejected count, age, and processing time.

## Worker failure

A worker can crash, return an error, deadlock, or become stuck in a device call. The
pool needs a health owner that notices missing progress, preserves the failed item’s
identity, and decides whether to restart a worker, fail the service, or degrade. Do
not restart a worker while its old thread may still access shared state.

## Fairness and priority

Separate queues or budgets may be needed for control traffic and bulk work. A single
FIFO can let telemetry starve safety commands; strict priorities can starve low-
priority maintenance. Bound per-class work and test overload explicitly.

## Common mistakes

- Making queue capacity unlimited.
- Signaling one waiter when shutdown requires waking all waiters.
- Destroying queue state before workers join.
- Releasing an item on both producer timeout and worker completion.
- Retrying failed work forever without age or attempt bounds.
- Letting bulk work starve control or shutdown traffic.
- Assuming a thread pool cancels an in-flight blocking syscall.

## Debugging checklist

- Record capacity, depth, enqueue/dequeue age, owner, and item ID.
- Test full queue, producer cancellation, worker failure, and shutdown races.
- Measure wait and service time under target load.
- Verify item ownership through every rejection, drop, retry, and shutdown path.
- Check worker stacks, blocked calls, CPU use, and join deadlines.
- Validate that backpressure reaches the true producer rather than an unbounded layer.

## Related topics

- [Stage 6: Threads And Userspace Concurrency](index.md)
- [Mutexes, Condition Variables, And Semaphores](mutexes-condition-variables-and-semaphores.md)
- [Cancellation, Priority, And Real-Time Scheduling](cancellation-priority-and-realtime-scheduling.md)
- [IPC Protocols And Versioning](../ipc-and-event-driven-design/ipc-protocols-and-versioning.md)

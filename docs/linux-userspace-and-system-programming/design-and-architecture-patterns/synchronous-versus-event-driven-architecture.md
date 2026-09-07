---
status: draft
reviewed: false
domain: linux-userspace
difficulty: advanced
last_reviewed: null
---

# Synchronous Versus Event-Driven Architecture

## Synchronous control

Blocking calls are often the clearest design for a utility or a low-concurrency service. They make sequencing and error propagation obvious, but each blocked thread consumes resources and cancellation can be difficult. Define timeouts and interruption behavior for every blocking boundary.

## Event-driven control

An event loop fits many descriptors, timers, signals, and stateful protocols. It can reduce thread count and make readiness explicit, but callback/state complexity, reentrancy, starvation, and error propagation require discipline. Keep handlers short; move CPU-heavy work to a bounded worker pool and return completion events.

## Decision criteria

Choose based on concurrency, latency deadlines, blocking APIs, CPU work, cancellation, memory budget, and team familiarity. Hybrid designs are common: an event loop owns I/O and a small worker pool handles computation. Do not let a blocking device call run on the loop thread without a measured bound.

Every queue needs ownership, capacity, ordering, backpressure, and overload behavior. Decide whether to reject, drop, coalesce, or prioritize work. Shutdown should stop admission, cancel or drain work according to policy, close descriptors, and report incomplete operations.

## Budget the whole path

For an illustrative 20 ms response requirement, reserve 3 ms for intake/parsing,
5 ms queue delay, 8 ms device work and 4 ms response/cleanup. These are design
budgets to verify, not measured guarantees. If the device call sometimes blocks
for a second, moving it to a worker preserves loop responsiveness but cannot
make the request meet 20 ms.

Choose synchronous execution when bounded concurrency and known blocking
behavior make sequencing simple. Choose a readiness loop when many connections
spend most of their time waiting. Add workers for bounded CPU work or APIs that
cannot be integrated directly, and expose worker saturation before the queue
consumes the response budget.

## A complete event-loop iteration

```text
process pending shutdown/control commands
service a bounded number of application-ready connections
attempt nonblocking reads/writes and retain partial progress
expire deadlines and retire stale generations
admit work only while capacity remains
wait only if no locally ready work remains
```

Edge-triggered sources need an application-ready queue when a work budget stops
processing before `EAGAIN`. Level triggering can simplify this obligation.
Workers publish results through a synchronized bounded queue; an eventfd is
a wakeup, not the result storage or memory synchronization protocol.

Cancellation needs an owner and acknowledgement. A timed-out caller cannot
free a buffer that a worker still uses. Prefer immutable owned messages or
explicitly transferred buffers and retire them only after completion/cancellation
is resolved. See the [event-loop chapter](../ipc-and-event-driven-design/event-loops-select-poll-and-epoll.md).

## Related topics

- [Stage 17 overview](index.md)
- [IPC protocols and versioning](../ipc-and-event-driven-design/ipc-protocols-and-versioning.md)
- [Lifecycle and hardware tests](../testing-and-verification/lifecycle-update-and-hardware-in-the-loop-tests.md)

# Synchronous Versus Event-Driven Architecture

## Synchronous control

Blocking calls are often the clearest design for a utility or a low-concurrency service. They make sequencing and error propagation obvious, but each blocked thread consumes resources and cancellation can be difficult. Define timeouts and interruption behavior for every blocking boundary.

## Event-driven control

An event loop fits many descriptors, timers, signals, and stateful protocols. It can reduce thread count and make readiness explicit, but callback/state complexity, reentrancy, starvation, and error propagation require discipline. Keep handlers short; move CPU-heavy work to a bounded worker pool and return completion events.

## Decision criteria

Choose based on concurrency, latency deadlines, blocking APIs, CPU work, cancellation, memory budget, and team familiarity. Hybrid designs are common: an event loop owns I/O and a small worker pool handles computation. Do not let a blocking device call run on the loop thread without a measured bound.

Every queue needs ownership, capacity, ordering, backpressure, and overload behavior. Decide whether to reject, drop, coalesce, or prioritize work. Shutdown should stop admission, cancel or drain work according to policy, close descriptors, and report incomplete operations.

---
status: draft
reviewed: false
domain: algorithms-data-models
difficulty: advanced
last_reviewed: null
---

# Atomic And Lock-Free Algorithm Patterns

Roadmap for algorithms that coordinate shared state with atomic operations instead of a conventional mutex-protected critical section.

## Coverage

- atomic load, store, and read-modify-write operations
- compare-and-swap loops
- lock-free versus wait-free claims
- memory ordering and publication
- ABA and lifetime hazards
- bounded queues and counters
- fallback and contention policy

## Programming Examples

- C: add a carefully scoped C11 atomic counter and SPSC publication example.
- Python: use a sequential reference model rather than pretending Python reproduces memory ordering.

## Embedded And Systems Angle

- state the producer/consumer model before choosing atomics
- avoid claiming lock-free behavior without target and memory-model evidence
- bound retry loops or expose contention failure
- separate object lifetime reclamation from pointer publication

## Future Material

- compare-and-swap counter walkthrough
- acquire/release ring-buffer publication
- ABA example and tagged-generation mitigation
- lock-free, wait-free, and obstruction-free comparison

## Related Topics

- [Parallel And Dataflow Algorithms](index.md)
- [Synchronization Costs And Result Merging](synchronization-costs-and-result-merging.md)
- [Ring Buffers](../data-structures-for-algorithms/ring-buffers.md)
- [Interrupt-Safe Queues And Buffers](../embedded-linux-algorithmic-constraints/interrupt-safe-queues-and-buffers.md)

---
status: draft
reviewed: false
domain: algorithms-data-models
difficulty: intermediate
last_reviewed: null
---

# Ring Buffers

Scaffold for bounded FIFO storage with wraparound indexing.

## Coverage

- ring buffer invariants
- head and tail indexes
- full and empty conditions
- producer-consumer handoff
- overwrite vs reject policy

## Programming Examples

- C: add the primary implementation example, with bounds, error handling, and memory behavior visible.
- Python: add a reference implementation when it helps explain the algorithm, generate test cases, or compare behavior.

## Embedded And Systems Angle

- use ring buffers for interrupt-to-thread and stream handoff
- define full-buffer behavior before implementation
- choose capacity and index types that avoid ambiguous states

## Future Material

- single-producer single-consumer example
- invariant diagrams
- exercises for wraparound and full/empty handling

## Related Topics

- [Data Structures For Algorithms](index.md)
- [Interrupt-Safe Queues And Buffers](../embedded-linux-algorithmic-constraints/interrupt-safe-queues-and-buffers.md)
- [Breadth-First Search](../graph-algorithms/breadth-first-search.md)


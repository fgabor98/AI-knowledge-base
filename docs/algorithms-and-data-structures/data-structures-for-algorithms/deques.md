---
status: draft
reviewed: false
domain: algorithms-data-models
difficulty: intermediate
last_reviewed: null
---

# Deques

Roadmap for double-ended queues that support insertion and removal at both the front and back.

## Coverage

- deque invariants and endpoint operations
- ring-buffer and linked-block representations
- sliding-window maximum and minimum
- work queues and 0-1 BFS
- full and empty policy
- ownership and concurrency variants

## Programming Examples

- C: add a fixed-capacity ring-backed deque with checked push and pop operations.
- Python: use `collections.deque` as a semantic reference for window algorithms.

## Embedded And Systems Angle

- prefer a bounded ring-backed deque when maximum work is known
- define which end owns or drops an item when full
- avoid pointer-heavy blocks when contiguous storage is sufficient
- make multi-producer use explicit rather than assuming endpoint operations are safe

## Future Material

- deque API and invariant walkthrough
- monotonic deque for sliding-window extrema
- 0-1 BFS with a deque
- bounded work-stealing design comparison

## Related Topics

- [Data Structures For Algorithms](index.md)
- [Ring Buffers](ring-buffers.md)
- [Practical Sequence Patterns](../basic-algorithm-schemes/practical-sequence-patterns.md)
- [Breadth-First Search](../graph-algorithms/breadth-first-search.md)

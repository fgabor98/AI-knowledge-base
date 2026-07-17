---
status: draft
reviewed: false
domain: algorithms-data-models
difficulty: intermediate
last_reviewed: null
---

# Backtracking

Scaffold for building, checking, undoing, and resuming partial solutions.

## Coverage

- backtracking
- recursive backtracking
- iterative backtracking with an explicit stack
- constraint checking
- failure propagation

## Programming Examples

- C: add the primary implementation example, with bounds, error handling, and memory behavior visible.
- Python: add a reference implementation when it helps explain the algorithm, generate test cases, or compare behavior.

## Embedded And Systems Angle

- prefer explicit stacks when recursion depth is not tightly bounded
- keep undo operations symmetric with apply operations
- expose counters and diagnostics for search blowups

## Future Material

- recursive and iterative skeletons
- N-queens style learning example
- exercises for constraint ordering

## Related Topics

- [Searching And Backtracking](index.md)
- [Search-Space Modeling](search-space-modeling.md)
- [Recursion Fundamentals](../control-flow-and-recursion/recursion-fundamentals.md)


---
status: draft
reviewed: false
domain: algorithms-data-models
difficulty: intermediate
last_reviewed: null
---

# Searching And Backtracking

Roadmap for explicit search over candidate solutions.

## Coverage

- search-space modeling
- exhaustive search
- guided search
- constraint checking
- pruning
- partial solutions
- backtracking
- recursive backtracking
- iterative backtracking with an explicit stack
- N-queens style problems
- failure propagation
- search order and heuristics

## Scaffold Pages

- [Search-Space Modeling](search-space-modeling.md)
- [Backtracking](backtracking.md)
- [Pruning And Search Heuristics](pruning-and-search-heuristics.md)

## Programming Examples

- C: add the primary implementation example, with bounds, error handling, and memory behavior visible.
- Python: add a reference implementation when it helps explain the algorithm, generate test cases, or compare behavior.

## Embedded And Systems Angle

- bound search depth and runtime before using backtracking on target hardware
- separate candidate generation from constraint checking
- prefer deterministic search order for reproducible diagnostics
- record enough state to resume, cancel, or explain a failed search

## Related Topics

- [Control Flow And Recursion](../control-flow-and-recursion/index.md)
- [Graph Algorithms](../graph-algorithms/index.md)
- [Complexity And Efficiency](../complexity-and-efficiency/index.md)

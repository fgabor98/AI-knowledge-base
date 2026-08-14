---
status: draft
reviewed: false
domain: algorithms-data-models
difficulty: intermediate
last_reviewed: null
---

# Disjoint-Set Union

Roadmap for maintaining a partition of elements into non-overlapping sets under repeated connectivity queries and unions.

## Coverage

- make-set, find, and union operations
- parent-array representation
- path compression
- union by rank or size
- connectivity and component counting
- Kruskal minimum spanning tree
- fixed-capacity and reset policy

## Programming Examples

- C: add an index-bounded union-find implementation with checked parent indexes.
- Python: use a compact reference for connectivity and component tests.

## Embedded And Systems Angle

- size parent and rank arrays from the maximum identifier count
- validate indexes before pointer chasing through parents
- define reset and reuse cost for repeated graph batches
- make component identity deterministic when diagnostics depend on it

## Future Material

- path-compression trace
- Kruskal MST example
- component-count and cycle-detection use cases
- comparison with BFS/DFS for one-shot connectivity

## Related Topics

- [Data Structures For Algorithms](index.md)
- [Advanced Graph Algorithms](../graph-algorithms/advanced-graph-algorithms.md)
- [Graph Representations](../graph-algorithms/graph-representations.md)
- [Bounded Memory And Allocation Failure](../embedded-linux-algorithmic-constraints/bounded-memory-and-allocation-failure.md)

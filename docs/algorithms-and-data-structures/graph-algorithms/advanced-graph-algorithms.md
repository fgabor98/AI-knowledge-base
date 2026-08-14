---
status: draft
reviewed: false
domain: algorithms-data-models
difficulty: advanced
last_reviewed: null
---

# Advanced Graph Algorithms

Roadmap for graph problems beyond basic traversal and single-source shortest paths.

## Coverage

- topological sorting and DAG processing
- strongly connected components
- minimum spanning trees
- Kruskal and Prim algorithm selection
- disjoint-set union for connectivity
- A* and goal-directed search
- graph algorithm selection by weight, direction, and objective

## Programming Examples

- C: add bounded topological-sort and connectivity implementations.
- Python: use small graph generators and reference implementations for SCC, MST, and A* behavior.

## Embedded And Systems Angle

- validate vertex and edge bounds before allocating work state
- distinguish a cycle from an ordinary “not schedulable” result
- define unreachable, disconnected, and negative-weight policies
- make heuristic search limits and incomplete results observable

## Future Material

- dependency graph and topological-order example
- Kruskal with union-find and deterministic edge ordering
- SCC analysis for dependency and service graphs
- A* heuristic admissibility and bounded-work policy

## Related Topics

- [Graph Algorithms](index.md)
- [Graph Representations](graph-representations.md)
- [Shortest Path Algorithms](shortest-path-algorithms.md)
- [Disjoint-Set Union](../data-structures-for-algorithms/disjoint-set-union.md)

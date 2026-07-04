---
status: draft
reviewed: false
domain: algorithms-data-models
difficulty: intermediate
last_reviewed: null
---

# Shortest Path Algorithms

Scaffold for choosing and implementing shortest-path algorithms according to graph weights and constraints.

## Coverage

- unweighted shortest paths
- path reconstruction
- Dijkstra's algorithm
- Bellman-Ford algorithm
- negative weights and overflow

## Programming Examples

- C: add the primary implementation example, with bounds, error handling, and memory behavior visible.
- Python: add a reference implementation when it helps explain the algorithm, generate test cases, or compare behavior.

## Embedded And Systems Angle

- choose integer widths that cannot overflow silently
- avoid priority-queue complexity unless edge weights require it
- decide how unreachable vertices are represented

## Future Material

- algorithm selection table
- Dijkstra walkthrough
- exercises for overflow, unreachable nodes, and predecessor tracking

## Related Topics

- [Graph Algorithms](index.md)
- [Breadth-First Search](breadth-first-search.md)
- [Heaps And Priority Queues](../data-structures-for-algorithms/heaps-and-priority-queues.md)


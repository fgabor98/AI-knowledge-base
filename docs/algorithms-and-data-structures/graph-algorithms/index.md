---
status: draft
reviewed: false
domain: algorithms-data-models
difficulty: intermediate
last_reviewed: null
---

# Graph Algorithms

Roadmap for modeling relationships and traversing graph-shaped problems.

## Coverage

- graph modeling
- vertices and edges
- directed and undirected graphs
- weighted and unweighted graphs
- adjacency lists
- adjacency matrices
- edge lists
- visited-state tracking
- depth-first search
- breadth-first search
- path reconstruction
- unweighted shortest paths
- Dijkstra's algorithm
- Bellman-Ford algorithm

## Scaffold Pages

- [Graph Representations](graph-representations.md)
- [Depth-First Search](depth-first-search.md)
- [Breadth-First Search](breadth-first-search.md)
- [Shortest Path Algorithms](shortest-path-algorithms.md)

## Programming Examples

- C: add the primary implementation example, with bounds, error handling, and memory behavior visible.
- Python: add a reference implementation when it helps explain the algorithm, generate test cases, or compare behavior.

## Embedded And Systems Angle

- choose adjacency matrices only when the vertex count is bounded and density justifies the memory
- use adjacency lists for sparse and dynamic relationships
- store predecessor information when diagnostics need the discovered path
- treat negative weights and overflow as correctness concerns

## Related Topics

- [Searching And Backtracking](../searching-and-backtracking/index.md)
- [Data Structures For Algorithms](../data-structures-for-algorithms/index.md)
- [Embedded Linux Algorithmic Constraints](../embedded-linux-algorithmic-constraints/index.md)

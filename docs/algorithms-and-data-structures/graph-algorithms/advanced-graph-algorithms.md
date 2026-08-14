---
status: draft
reviewed: false
domain: algorithms-data-models
difficulty: advanced
last_reviewed: null
---

# Advanced Graph Algorithms

Advanced graph algorithms build on representations and traversals to answer dependency, connectivity, optimization, and goal-directed search questions. The right choice depends on direction, edge weights, graph density, mutability, and whether the graph is guaranteed to be connected or acyclic.

## Algorithm Selection

| Objective | Conditions | Typical algorithm |
| --- | --- | --- |
| dependency order | directed acyclic graph | Kahn's algorithm or DFS finish order |
| detect dependency cycle | directed graph | topological sort failure or DFS colors |
| mutually reachable groups | directed graph | Kosaraju or Tarjan SCC |
| minimum total cost connecting vertices | undirected weighted graph | Kruskal or Prim |
| dynamic connectivity | undirected additions and unions | disjoint-set union |
| shortest route to one goal | non-negative weights and useful heuristic | A* |

State what happens when the precondition fails. A topological sort cannot produce a complete order for a cyclic graph; an MST does not exist for a disconnected graph unless the result is explicitly a minimum spanning forest.

## Topological Sorting

A topological order of a directed graph places every edge `u -> v` with `u` before `v`. It exists exactly when the graph is acyclic.

Kahn's algorithm computes indegrees, queues all zero-indegree vertices, and repeatedly removes one while decrementing its outgoing neighbors. The invariant is:

> Every emitted vertex has no remaining incoming edge from an un-emitted vertex.

If fewer than `V` vertices are emitted, at least one cycle remains. The result should report a cycle rather than returning a partial order as if it were complete.

Complexity is O(V + E), with O(V) indegree and queue storage.

### C: Bounded Kahn Topological Sort

```c
#include <stddef.h>

enum topo_status {
    TOPO_OK = 0,
    TOPO_ERR_NULL,
    TOPO_ERR_VERTEX,
    TOPO_ERR_CAPACITY,
    TOPO_ERR_CYCLE
};

struct directed_edge {
    size_t from;
    size_t to;
};

enum topo_status topological_order(size_t vertex_count,
                                   const struct directed_edge *edges,
                                   size_t edge_count,
                                   size_t *order,
                                   size_t *out_count)
{
    size_t indegree[32] = { 0 };
    size_t queue[32];
    size_t head = 0;
    size_t tail = 0;
    size_t written = 0;

    if (out_count == NULL || order == NULL)
        return TOPO_ERR_NULL;
    if (vertex_count > 32)
        return TOPO_ERR_CAPACITY;
    if (edges == NULL && edge_count > 0)
        return TOPO_ERR_NULL;

    for (size_t i = 0; i < edge_count; i++) {
        if (edges[i].from >= vertex_count || edges[i].to >= vertex_count)
            return TOPO_ERR_VERTEX;
        indegree[edges[i].to]++;
    }
    for (size_t vertex = 0; vertex < vertex_count; vertex++)
        if (indegree[vertex] == 0)
            queue[tail++] = vertex;

    while (head < tail) {
        size_t current = queue[head++];

        order[written++] = current;
        for (size_t i = 0; i < edge_count; i++) {
            if (edges[i].from != current)
                continue;
            if (--indegree[edges[i].to] == 0)
                queue[tail++] = edges[i].to;
        }
    }

    *out_count = written;
    return written == vertex_count ? TOPO_OK : TOPO_ERR_CYCLE;
}
```

The edge-list implementation scans all edges for every emitted vertex, so its literal cost is O(VE) rather than O(V + E). An adjacency list gives the usual O(V + E) behavior. For small bounded graphs, the simpler edge-list storage may still be easier to validate.

Use a deterministic zero-indegree queue if build order matters. A min-heap produces the lexicographically smallest available order; a FIFO queue produces a stable order based on discovery.

## DAG Dynamic Programming

Once a topological order exists, many graph problems become dynamic programs. For longest path in a DAG, initialize distances to impossible, process vertices in topological order, and relax outgoing edges using `max`. Unlike general graphs, a DAG can have negative edge weights without a cycle that makes the objective unbounded.

Dependency systems can use the same pattern for build levels, critical-path estimates, and resource readiness. Validate that each task's predecessor references are present before computing a schedule.

## Strongly Connected Components

A strongly connected component (SCC) is a maximal set of vertices where every vertex can reach every other vertex. Compressing each SCC into one node produces a condensation graph, which is always a DAG.

SCCs are useful for:

- finding mutually dependent modules
- detecting feedback loops in control or dataflow graphs
- reducing a directed graph before scheduling
- reasoning about recursion in a dependency graph

Tarjan's algorithm performs one DFS and tracks discovery indexes and low-link values. A vertex whose low-link equals its discovery index is the root of an SCC. Kosaraju's algorithm performs DFS on the graph and its transpose in two passes. Both are O(V + E); Tarjan avoids explicitly storing a second graph but needs a stack and per-vertex metadata.

The low-link invariant is subtle: a back edge to a vertex currently on the DFS stack can lower a component's low-link, while an edge to an already completed component must not merge components.

## Minimum Spanning Trees

For a connected undirected weighted graph, an MST connects all vertices with minimum total edge weight and no cycle. If equal weights exist, several MSTs may be valid; compare total cost and connectivity rather than requiring one exact edge set unless the tie policy is part of the contract.

### Kruskal

Kruskal sorts edges by weight and accepts an edge when it connects two different components. DSU makes the component test and union nearly constant amortized time. Complexity is O(E log E) for sorting.

The cut property explains correctness: the lightest edge crossing a cut can be included in some MST. Deterministic secondary keys make the selected tree reproducible.

### Prim

Prim grows one tree from a chosen start vertex, repeatedly selecting the cheapest edge from the current tree to an unvisited vertex. A heap and adjacency list give O(E log V); a dense-array implementation gives O(V²). Prim must be restarted for each unvisited component when a spanning forest is desired.

Choose Kruskal when the edge list is natural or the graph is sparse and disconnectedness should be detected. Choose Prim when adjacency traversal and a connected growing frontier are already available.

### Python: Kruskal Reference

```python
class DisjointSet:
    def __init__(self, count):
        self.parent = list(range(count))
        self.size = [1] * count

    def find(self, value):
        while self.parent[value] != value:
            self.parent[value] = self.parent[self.parent[value]]
            value = self.parent[value]
        return value

    def union(self, left, right):
        left = self.find(left)
        right = self.find(right)
        if left == right:
            return False
        if self.size[left] < self.size[right]:
            left, right = right, left
        self.parent[right] = left
        self.size[left] += self.size[right]
        return True


def minimum_spanning_tree(vertex_count, edges):
    dsu = DisjointSet(vertex_count)
    selected = []
    total = 0
    for weight, left, right in sorted(edges):
        if not (0 <= left < vertex_count and 0 <= right < vertex_count):
            raise ValueError("vertex out of range")
        if dsu.union(left, right):
            selected.append((weight, left, right))
            total += weight
    if len(selected) != max(0, vertex_count - 1):
        raise ValueError("graph is disconnected")
    return total, selected
```

The tuple sort gives deterministic ordering by weight, then endpoints. A production implementation should define whether negative weights and self-loops are accepted; Kruskal can ignore self-loops, but rejecting them may expose an upstream modeling error sooner.

## A* Search

A* combines the cost already paid `g(v)` with a heuristic estimate `h(v)` to the goal:

```text
f(v) = g(v) + h(v)
```

For an optimal result, the heuristic must be admissible: it never overestimates the remaining cost. A consistent heuristic also satisfies a triangle-like condition along every edge, which allows finalized distances to remain final and reduces reopenings.

Examples include Manhattan distance on a four-neighbor grid with uniform movement cost and straight-line distance when movement costs are proportional to physical distance. A heuristic based on an unconstrained relaxation can be admissible even when it ignores obstacles.

A* is not automatically faster than Dijkstra. A weak heuristic behaves like Dijkstra; an expensive heuristic may cost more than the search it saves. Bound the open set, node expansions, and memory when incomplete results are acceptable, and report an incomplete result separately from “no path.”

## Common Mistakes

- Returning a partial topological order as a valid schedule after detecting a cycle.
- Using DFS finish order without tracking cycles or validating the directed graph.
- Treating an MST as unique when equal weights allow alternatives.
- Forgetting that Prim on one start vertex covers only one component.
- Using DSU for directed reachability or path queries it cannot answer.
- Using an inadmissible A* heuristic while claiming optimality.
- Failing to check distance and total-weight overflow.
- Reporting “unreachable” and “search budget exhausted” as the same result.

## Embedded And Systems Angle

- bound vertices, edges, indegree arrays, DFS stacks, and heap occupancy
- use topological order for deterministic dependency execution
- reject cycles before starting a schedule that assumes a DAG
- choose edge-list, adjacency-list, or matrix storage from actual limits
- make disconnected, incomplete, and overflow statuses explicit
- use a bounded A* budget only when the caller accepts a best-effort route

## Review Checklist

- Are graph direction and edge-weight assumptions explicit?
- What result represents a cycle, disconnected component, or no path?
- Is the proof based on a valid invariant or cut property?
- Does storage match the graph representation and maximum degree?
- Are ties deterministic when output order affects scheduling or tests?
- Are heuristic admissibility, search limits, and incomplete results documented?

## Related Topics

- [Graph Algorithms](index.md)
- [Graph Representations](graph-representations.md)
- [Shortest Path Algorithms](shortest-path-algorithms.md)
- [Depth-First Search](depth-first-search.md)
- [Disjoint-Set Union](../data-structures-for-algorithms/disjoint-set-union.md)
- [Dynamic Programming](../algorithm-design-techniques/dynamic-programming.md)

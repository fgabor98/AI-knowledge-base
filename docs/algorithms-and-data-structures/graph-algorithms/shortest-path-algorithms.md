---
status: draft
reviewed: false
domain: algorithms-data-models
difficulty: intermediate
last_reviewed: null
---

# Shortest Path Algorithms

Shortest-path algorithms find the least-cost path from a source to one or more vertices. The correct algorithm depends on edge weights, graph direction, whether negative weights are possible, and whether the result must be one path or all distances.

Choosing the algorithm from the graph's constraints is more important than memorizing implementation details.

## Algorithm Selection

| Graph condition | Algorithm | Typical cost |
| --- | --- | --- |
| unweighted or all edges cost one | BFS | O(V + E) |
| non-negative weights | Dijkstra | O(V² + E) without a heap; O((V + E) log V) with one |
| negative edges, no negative cycle | Bellman-Ford | O(VE) |
| negative cycle reachable from source | no finite shortest path for affected vertices | report cycle |
| all-pairs distances, dense small graph | Floyd-Warshall | O(V³) |

Dijkstra's greedy choice is correct only when every edge weight is non-negative. A negative edge can make an already finalized distance smaller later.

## Distance Representation

Choose an explicit unreachable sentinel. Do not use zero unless zero cannot be a valid distance. Unsigned infinity is often `UINT32_MAX`, but addition must be checked before using it.

For a distance `d` and edge weight `w`, the safe relaxation test is:

```text
if d is finite and w <= MAX_DISTANCE - d:
    candidate = d + w
else:
    report overflow or skip according to the contract
```

Overflow is a correctness failure. Wrapping a large path to a small number can make the algorithm select an invalid route.

## Dijkstra's Invariant

At each iteration of Dijkstra's algorithm:

> Every finalized vertex has its true shortest distance from the source.

The proof relies on non-negative weights. The unfinalized vertex with the smallest tentative distance cannot be improved by a path that first visits another unfinalized vertex, because that other path already costs at least as much and later edges cannot reduce cost.

## Programming Examples

### C: Bounded Dijkstra Without Dynamic Allocation

This implementation uses an edge list and an O(V²) selection loop. It is suitable for small bounded graphs where avoiding a heap is more valuable than asymptotic improvement.

```c
#include <stddef.h>
#include <stdint.h>

enum {
    PATH_MAX_VERTICES = 16,
    PATH_MAX_EDGES = 64,
    PATH_INFINITY = UINT32_MAX,
    PATH_NO_VERTEX = (size_t)-1
};

enum path_status {
    PATH_OK = 0,
    PATH_ERR_NULL,
    PATH_ERR_VERTEX,
    PATH_ERR_FULL,
    PATH_ERR_WEIGHT,
    PATH_ERR_OVERFLOW
};

struct weighted_edge {
    size_t from;
    size_t to;
    uint32_t weight;
};

struct weighted_graph {
    size_t vertex_count;
    size_t edge_count;
    struct weighted_edge edges[PATH_MAX_EDGES];
};

enum path_status weighted_graph_add_edge(struct weighted_graph *graph,
                                         size_t from,
                                         size_t to,
                                         uint32_t weight)
{
    if (graph == NULL)
        return PATH_ERR_NULL;
    if (from >= graph->vertex_count || to >= graph->vertex_count)
        return PATH_ERR_VERTEX;
    if (graph->edge_count == PATH_MAX_EDGES)
        return PATH_ERR_FULL;
    if (weight == PATH_INFINITY)
        return PATH_ERR_WEIGHT;

    graph->edges[graph->edge_count++] = (struct weighted_edge){
        .from = from,
        .to = to,
        .weight = weight
    };
    return PATH_OK;
}

enum path_status dijkstra_distances(const struct weighted_graph *graph,
                                    size_t source,
                                    uint32_t *out_distance,
                                    size_t *out_predecessor)
{
    unsigned char finalized[PATH_MAX_VERTICES] = { 0 };

    if (graph == NULL || out_distance == NULL || out_predecessor == NULL)
        return PATH_ERR_NULL;
    if (source >= graph->vertex_count)
        return PATH_ERR_VERTEX;

    for (size_t vertex = 0; vertex < graph->vertex_count; vertex++) {
        out_distance[vertex] = PATH_INFINITY;
        out_predecessor[vertex] = PATH_NO_VERTEX;
    }
    out_distance[source] = 0;

    for (size_t iteration = 0;
         iteration < graph->vertex_count;
         iteration++) {
        size_t current = PATH_NO_VERTEX;

        for (size_t vertex = 0; vertex < graph->vertex_count; vertex++) {
            if (finalized[vertex] || out_distance[vertex] == PATH_INFINITY)
                continue;
            if (current == PATH_NO_VERTEX ||
                out_distance[vertex] < out_distance[current])
                current = vertex;
        }

        if (current == PATH_NO_VERTEX)
            break;
        finalized[current] = 1;

        for (size_t i = 0; i < graph->edge_count; i++) {
            const struct weighted_edge *edge = &graph->edges[i];
            uint32_t candidate;

            if (edge->from != current)
                continue;
            if (edge->weight > PATH_INFINITY - out_distance[current])
                return PATH_ERR_OVERFLOW;

            candidate = out_distance[current] + edge->weight;
            if (candidate < out_distance[edge->to]) {
                out_distance[edge->to] = candidate;
                out_predecessor[edge->to] = current;
            }
        }
    }

    return PATH_OK;
}
```

The implementation scans all edges during each selected-vertex iteration, so its time is O(V² + VE) with this literal edge-list loop. An adjacency list reduces relaxation scanning to O(V² + E), while a binary heap gives a different tradeoff. The fixed arrays make storage predictable.

### C: Reconstructing A Weighted Path

```c
enum path_status reconstruct_weighted_path(size_t source,
                                           size_t target,
                                           const size_t *predecessor,
                                           size_t vertex_count,
                                           size_t *path,
                                           size_t path_capacity,
                                           size_t *out_count)
{
    size_t count = 0;
    size_t current = target;

    if (predecessor == NULL || path == NULL || out_count == NULL)
        return PATH_ERR_NULL;
    if (source >= vertex_count || target >= vertex_count)
        return PATH_ERR_VERTEX;

    while (current != PATH_NO_VERTEX) {
        if (count == path_capacity)
            return PATH_ERR_FULL;
        path[count++] = current;
        if (current == source)
            break;
        current = predecessor[current];
    }

    if (count == 0 || path[count - 1] != source)
        return PATH_ERR_VERTEX;

    for (size_t left = 0, right = count - 1; left < right; left++, right--) {
        size_t temporary = path[left];
        path[left] = path[right];
        path[right] = temporary;
    }

    *out_count = count;
    return PATH_OK;
}
```

The predecessor chain should be acyclic when produced by a correct shortest-path algorithm. The capacity check still protects against malformed input or corrupted state.

### Python: Dijkstra Reference

```python
import heapq


def dijkstra(graph, source):
    distance = {vertex: float("inf") for vertex in graph}
    predecessor = {vertex: None for vertex in graph}
    distance[source] = 0
    pending = [(0, source)]

    while pending:
        current_distance, vertex = heapq.heappop(pending)
        if current_distance != distance[vertex]:
            continue

        for neighbor, weight in graph[vertex]:
            if weight < 0:
                raise ValueError("Dijkstra requires non-negative weights")
            candidate = current_distance + weight
            if candidate < distance[neighbor]:
                distance[neighbor] = candidate
                predecessor[neighbor] = vertex
                heapq.heappush(pending, (candidate, neighbor))

    return distance, predecessor
```

The heap may contain stale entries; comparing the popped distance with the current best distance discards them. The C version avoids that allocation pattern by using bounded arrays and a linear selection step.

## Bellman-Ford

Bellman-Ford relaxes every edge repeatedly for `V - 1` passes. After those passes, a further improving relaxation identifies a reachable negative cycle. It is slower than Dijkstra but supports negative edge weights.

A practical implementation should:

- use a signed distance type with explicit overflow checks
- distinguish unreachable vertices from finite distances
- stop early when a pass makes no changes
- report a reachable negative cycle separately from ordinary no-path results

Do not use Dijkstra as a fallback for negative weights. Select Bellman-Ford or reject the input according to the graph contract.

## Unreachable Vertices

Unreachable is not an error in every application. It may mean:

- no route exists in a directed topology
- the device is offline
- a dependency is not available
- a graph component was not connected to the source

Represent it explicitly and let the caller choose whether it is a normal result or an operational failure.

## Common Mistakes

- Applying Dijkstra to graphs with negative weights.
- Using a finite sentinel that can be mistaken for a real distance.
- Adding a weight to infinity or allowing unsigned wraparound.
- Forgetting to initialize predecessor entries for unreachable vertices.
- Reconstructing a path without a capacity or malformed-chain guard.
- Claiming an O(V² + E) implementation while scanning all edges for every vertex.

## Embedded And Systems Angle

- choose integer widths and overflow policy before implementing relaxation
- avoid a priority queue when the graph is small and a bounded O(V²) scan is easier to verify
- define unreachable-node behavior explicitly
- store predecessor information only when path diagnostics or output require it
- make negative weights an input-validation decision, not an accidental algorithm mode

## Related Topics

- [Graph Algorithms](index.md)
- [Graph Representations](graph-representations.md)
- [Breadth-First Search](breadth-first-search.md)
- [Heaps And Priority Queues](../data-structures-for-algorithms/heaps-and-priority-queues.md)
- [Priority And Partial Ordering](../sorting-and-ordering/priority-and-partial-ordering.md)

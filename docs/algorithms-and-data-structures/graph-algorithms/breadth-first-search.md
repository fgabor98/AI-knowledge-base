---
status: draft
reviewed: false
domain: algorithms-data-models
difficulty: intermediate
last_reviewed: null
---

# Breadth-First Search

Breadth-first search (BFS) explores a graph in layers. It visits the start vertex, then all vertices one edge away, then all vertices two edges away, and so on. In an unweighted graph, the first distance assigned to a vertex is the length of a shortest path from the start.

BFS is therefore both a traversal and a shortest-path algorithm for unit-cost edges.

## Queue And Distance Invariants

The central invariants are:

- a vertex is marked visited when it is enqueued
- every queued vertex has a known shortest distance from the start
- the queue contains discovered but not fully expanded vertices
- vertices are removed in nondecreasing distance order

Marking on enqueue is important. If marking waits until dequeue, multiple parents may enqueue the same vertex and a fixed queue can fill with redundant entries.

## BFS Layers

At the beginning of processing a layer at distance `d`:

- all vertices at distance less than `d` have been expanded
- queued vertices have distance `d` or `d + 1`
- any undiscovered neighbor of a distance-`d` vertex receives distance `d + 1`

This layer property proves shortest distance for unweighted edges. A weighted edge with cost greater than one breaks the proof; use Dijkstra or another weighted algorithm instead.

## Path Reconstruction

Store `predecessor[v]` when `v` is first discovered. To reconstruct a path to a target, follow predecessors backward to the start and reverse the resulting sequence.

Use a sentinel such as `GRAPH_NO_VERTEX` for the start predecessor and unreachable vertices. A predecessor array is unnecessary when only reachability or distance is required.

## Programming Examples

### C: Bounded BFS With Distances And Predecessors

This example uses a fixed adjacency table and a fixed queue. It traverses one connected component from `start`; unreachable vertices retain distance `BFS_UNREACHABLE`.

```c
#include <stddef.h>

enum {
    BFS_MAX_VERTICES = 16,
    BFS_MAX_NEIGHBORS = 16,
    BFS_UNREACHABLE = (size_t)-1,
    BFS_NO_VERTEX = (size_t)-1
};

enum bfs_status {
    BFS_OK = 0,
    BFS_ERR_NULL,
    BFS_ERR_VERTEX,
    BFS_ERR_QUEUE
};

struct bfs_graph {
    size_t vertex_count;
    size_t degree[BFS_MAX_VERTICES];
    size_t neighbors[BFS_MAX_VERTICES][BFS_MAX_NEIGHBORS];
};

static int bfs_vertex_valid(const struct bfs_graph *graph, size_t vertex)
{
    return graph != NULL && vertex < graph->vertex_count;
}

enum bfs_status bfs_add_edge(struct bfs_graph *graph,
                             size_t from,
                             size_t to)
{
    if (graph == NULL)
        return BFS_ERR_NULL;
    if (!bfs_vertex_valid(graph, from) || !bfs_vertex_valid(graph, to))
        return BFS_ERR_VERTEX;
    if (graph->degree[from] == BFS_MAX_NEIGHBORS)
        return BFS_ERR_QUEUE;

    graph->neighbors[from][graph->degree[from]++] = to;
    return BFS_OK;
}

enum bfs_status bfs_shortest_paths(const struct bfs_graph *graph,
                                   size_t start,
                                   size_t *out_distance,
                                   size_t *out_predecessor)
{
    size_t queue[BFS_MAX_VERTICES];
    size_t head = 0;
    size_t tail = 0;

    if (graph == NULL || out_distance == NULL || out_predecessor == NULL)
        return BFS_ERR_NULL;
    if (!bfs_vertex_valid(graph, start))
        return BFS_ERR_VERTEX;

    for (size_t vertex = 0; vertex < graph->vertex_count; vertex++) {
        out_distance[vertex] = BFS_UNREACHABLE;
        out_predecessor[vertex] = BFS_NO_VERTEX;
    }

    out_distance[start] = 0;
    if (tail == BFS_MAX_VERTICES)
        return BFS_ERR_QUEUE;
    queue[tail++] = start;

    while (head < tail) {
        size_t vertex = queue[head++];

        for (size_t i = 0; i < graph->degree[vertex]; i++) {
            size_t neighbor = graph->neighbors[vertex][i];

            if (out_distance[neighbor] != BFS_UNREACHABLE)
                continue;
            if (tail == BFS_MAX_VERTICES)
                return BFS_ERR_QUEUE;

            out_distance[neighbor] = out_distance[vertex] + 1;
            out_predecessor[neighbor] = vertex;
            queue[tail++] = neighbor;
        }
    }

    return BFS_OK;
}
```

Because each vertex is enqueued at most once, the queue needs capacity for at most `V` vertices. With adjacency lists, the time is O(V + E) and the extra storage is O(V).

### C: Reconstructing A Path

The output path is written from the target backward and then reversed in place. The caller supplies capacity, so an unexpectedly long or cyclic predecessor chain becomes an error.

```c
enum bfs_status bfs_reconstruct_path(size_t start,
                                    size_t target,
                                    const size_t *predecessor,
                                    size_t vertex_count,
                                    size_t *path,
                                    size_t path_capacity,
                                    size_t *out_path_count)
{
    size_t count = 0;
    size_t current = target;

    if (predecessor == NULL || path == NULL || out_path_count == NULL)
        return BFS_ERR_NULL;
    if (start >= vertex_count || target >= vertex_count)
        return BFS_ERR_VERTEX;

    while (current != BFS_NO_VERTEX) {
        if (count == path_capacity)
            return BFS_ERR_QUEUE;
        path[count++] = current;
        if (current == start)
            break;
        current = predecessor[current];
    }

    if (count == 0 || path[count - 1] != start)
        return BFS_ERR_VERTEX;

    for (size_t left = 0, right = count - 1; left < right; left++, right--) {
        size_t temporary = path[left];
        path[left] = path[right];
        path[right] = temporary;
    }

    *out_path_count = count;
    return BFS_OK;
}
```

### Python: BFS Reference

```python
from collections import deque


def shortest_paths(graph, start):
    distance = {start: 0}
    predecessor = {start: None}
    queue = deque([start])

    while queue:
        vertex = queue.popleft()
        for neighbor in graph[vertex]:
            if neighbor in distance:
                continue
            distance[neighbor] = distance[vertex] + 1
            predecessor[neighbor] = vertex
            queue.append(neighbor)

    return distance, predecessor


def reconstruct_path(predecessor, start, target):
    path = []
    current = target
    while current is not None:
        path.append(current)
        if current == start:
            return list(reversed(path))
        current = predecessor.get(current)
    return None
```

## Disconnected Graphs

BFS from one start visits only the start's connected or reachable component. To traverse all vertices, loop over vertices and start a new BFS for each undiscovered vertex. For an undirected graph this produces connected components; for a directed graph it produces reachability trees rooted at the selected starts, not necessarily strongly connected components.

## Queue Representation

A ring buffer can implement the BFS queue without shifting elements. A simple array with separate `head` and `tail` indexes is enough when each vertex is enqueued at most once and the capacity is V. If the queue may be reused or hold more general events, define full and empty behavior explicitly.

## Common Mistakes

- Marking vertices on dequeue and allowing duplicate queue entries.
- Using BFS for weighted edges without proving all edge costs are equal.
- Forgetting to initialize distance and predecessor arrays for unreachable vertices.
- Reconstructing a path without a capacity or cycle guard.
- Assuming one BFS start covers a disconnected graph.
- Using a queue that silently overwrites entries when full.

## Embedded And Systems Angle

- bound queue memory by the maximum vertex count or event count
- store predecessor information only when diagnostics or paths require it
- define behavior for disconnected graphs and unreachable targets
- use a ring buffer when queue operations cross producer/consumer boundaries
- expose queue high-water mark for sizing and field diagnostics

## Related Topics

- [Graph Algorithms](index.md)
- [Graph Representations](graph-representations.md)
- [Depth-First Search](depth-first-search.md)
- [Ring Buffers](../data-structures-for-algorithms/ring-buffers.md)
- [Shortest Path Algorithms](shortest-path-algorithms.md)

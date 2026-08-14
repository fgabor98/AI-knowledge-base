---
status: draft
reviewed: false
domain: algorithms-data-models
difficulty: intermediate
last_reviewed: null
---

# Depth-First Search

Depth-first search (DFS) follows one path as far as possible before returning to the most recent branch point. It is useful for reachability, connected components, cycle detection, dependency exploration, and postorder algorithms.

DFS is a traversal strategy, not a single result. The result depends on the graph representation, neighbor order, starting vertices, and whether the traversal is recursive or uses an explicit stack.

## DFS State

A basic DFS needs:

- the graph and a current vertex
- a visited set
- storage for the active path or explicit stack
- optional discovery order, finish order, or predecessor output

The key invariant is:

> A vertex is marked visited before its outgoing edges are explored, and every visited vertex is explored at most once.

Marking on discovery prevents cycles from causing infinite traversal. Marking only when a vertex is removed from the stack can enqueue or push duplicate work in graphs with converging paths.

## Recursive DFS

The recursive form maps directly to the definition:

```text
visit(v)
  mark v
  for each neighbor w of v
    if w is not marked
      visit(w)
```

Its active memory is O(V) in the worst case, mostly the call stack and visited state. Recursion is acceptable only when the maximum depth is known and the target stack has sufficient margin.

## Iterative DFS

An explicit stack replaces recursive calls. A frame can store only a vertex, or it can store a vertex plus the next neighbor index when the traversal needs to reproduce recursive entry/exit events exactly.

The simple vertex-stack form is enough for reachability and preorder. It pushes neighbors in reverse desired order when a deterministic order matching a forward adjacency array is required.

## Programming Examples

### C: Bounded Graph And Recursive DFS

This self-contained example uses a fixed adjacency table. It records preorder and traverses every connected component in ascending vertex order.

```c
#include <stddef.h>

enum {
    DFS_MAX_VERTICES = 16,
    DFS_MAX_NEIGHBORS = 16
};

enum dfs_status {
    DFS_OK = 0,
    DFS_ERR_NULL,
    DFS_ERR_VERTEX,
    DFS_ERR_OUTPUT
};

struct dfs_graph {
    size_t vertex_count;
    size_t degree[DFS_MAX_VERTICES];
    size_t neighbors[DFS_MAX_VERTICES][DFS_MAX_NEIGHBORS];
};

static int dfs_vertex_valid(const struct dfs_graph *graph, size_t vertex)
{
    return graph != NULL && vertex < graph->vertex_count;
}

enum dfs_status dfs_add_edge(struct dfs_graph *graph,
                             size_t from,
                             size_t to)
{
    if (graph == NULL)
        return DFS_ERR_NULL;
    if (!dfs_vertex_valid(graph, from) || !dfs_vertex_valid(graph, to))
        return DFS_ERR_VERTEX;
    if (graph->degree[from] == DFS_MAX_NEIGHBORS)
        return DFS_ERR_OUTPUT;

    graph->neighbors[from][graph->degree[from]++] = to;
    return DFS_OK;
}

static enum dfs_status dfs_visit(const struct dfs_graph *graph,
                                 size_t vertex,
                                 unsigned char *visited,
                                 size_t *order,
                                 size_t order_capacity,
                                 size_t *order_count)
{
    if (visited[vertex])
        return DFS_OK;
    if (*order_count == order_capacity)
        return DFS_ERR_OUTPUT;

    visited[vertex] = 1;
    order[(*order_count)++] = vertex;

    for (size_t i = 0; i < graph->degree[vertex]; i++) {
        enum dfs_status status = dfs_visit(graph,
                                           graph->neighbors[vertex][i],
                                           visited,
                                           order,
                                           order_capacity,
                                           order_count);
        if (status != DFS_OK)
            return status;
    }
    return DFS_OK;
}

enum dfs_status dfs_recursive_all(const struct dfs_graph *graph,
                                  size_t *order,
                                  size_t order_capacity,
                                  size_t *out_order_count)
{
    unsigned char visited[DFS_MAX_VERTICES] = { 0 };

    if (graph == NULL || order == NULL || out_order_count == NULL)
        return DFS_ERR_NULL;
    if (graph->vertex_count > DFS_MAX_VERTICES)
        return DFS_ERR_VERTEX;

    *out_order_count = 0;
    for (size_t vertex = 0; vertex < graph->vertex_count; vertex++) {
        enum dfs_status status = dfs_visit(graph,
                                           vertex,
                                           visited,
                                           order,
                                           order_capacity,
                                           out_order_count);
        if (status != DFS_OK)
            return status;
    }
    return DFS_OK;
}
```

The visited array guarantees each vertex enters `dfs_visit` at most once. The output capacity check makes failure explicit instead of silently writing beyond the caller's buffer.

### C: Iterative DFS

```c
enum dfs_status dfs_iterative_from(const struct dfs_graph *graph,
                                   size_t start,
                                   size_t *order,
                                   size_t order_capacity,
                                   size_t *out_order_count)
{
    unsigned char visited[DFS_MAX_VERTICES] = { 0 };
    size_t stack[DFS_MAX_VERTICES];
    size_t stack_count = 0;

    if (graph == NULL || order == NULL || out_order_count == NULL)
        return DFS_ERR_NULL;
    if (!dfs_vertex_valid(graph, start))
        return DFS_ERR_VERTEX;

    *out_order_count = 0;
    stack[stack_count++] = start;
    visited[start] = 1;

    while (stack_count > 0) {
        size_t vertex = stack[--stack_count];

        if (*out_order_count == order_capacity)
            return DFS_ERR_OUTPUT;

        order[(*out_order_count)++] = vertex;

        for (size_t i = graph->degree[vertex]; i > 0; i--) {
            size_t neighbor = graph->neighbors[vertex][i - 1];

            if (!visited[neighbor]) {
                visited[neighbor] = 1;
                if (stack_count == DFS_MAX_VERTICES)
                    return DFS_ERR_OUTPUT;
                stack[stack_count++] = neighbor;
            }
        }
    }

    return DFS_OK;
}
```

The explicit stack is bounded by the vertex limit, although duplicate pushes can occur before a vertex is marked. A production implementation can mark on push or use a separate queued flag if that matters for the exact capacity bound.

### Python: DFS Reference

```python
def depth_first_order(graph, start):
    visited = set()
    order = []
    stack = [start]

    while stack:
        vertex = stack.pop()
        if vertex in visited:
            continue
        visited.add(vertex)
        order.append(vertex)
        stack.extend(reversed(graph[vertex]))

    return order
```

The reverse push preserves the listed neighbor order in the resulting preorder. The graph is assumed to use valid vertex keys.

## Cycle Detection

For a directed graph, a three-state color model is useful:

- white: not discovered
- gray: discovered but not finished
- black: all outgoing edges finished

An edge from a gray vertex to another gray vertex is a back edge and proves a directed cycle in the current DFS forest. A simple visited set cannot distinguish a back edge from an edge to an already completed vertex.

For an undirected graph, a visited neighbor is not automatically a cycle because it may be the parent edge. Track the parent vertex or edge identity.

## DFS Orders

Discovery order is recorded when a vertex is first marked. Finish order is recorded after all neighbors are processed. Finish order supports topological sorting of a directed acyclic graph when the graph is validated as acyclic.

Traversal order is not a semantic property unless the graph contract defines neighbor order. Do not write tests that depend on incidental storage order without documenting it.

## Complexity

With an adjacency list, DFS is O(V + E) because each vertex and adjacency entry is inspected a bounded number of times. With an adjacency matrix, scanning neighbors costs O(V) per vertex, so the traversal is O(V²).

## Common Mistakes

- Marking vertices too late and allowing cycles or duplicate work.
- Using recursive DFS with attacker-controlled or unbounded depth.
- Treating a visited edge as a cycle in an undirected graph without excluding the parent.
- Forgetting disconnected components when the requirement is to traverse the whole graph.
- Returning traversal order without defining neighbor-order policy.
- Pushing neighbors beyond a fixed stack capacity.

## Embedded And Systems Angle

- avoid recursive DFS when graph depth is not tightly bounded
- store visited state in a bitmap when vertex IDs are dense
- make cycle detection explicit for dependency graphs and configuration validation
- use fixed output and stack capacities with distinct overflow statuses
- expose discovered count and deepest level for diagnostics

## Related Topics

- [Graph Algorithms](index.md)
- [Graph Representations](graph-representations.md)
- [Breadth-First Search](breadth-first-search.md)
- [Recursion Fundamentals](../control-flow-and-recursion/recursion-fundamentals.md)
- [Bitsets And Bitmaps](../data-structures-for-algorithms/bitsets-and-bitmaps.md)

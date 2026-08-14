---
status: draft
reviewed: false
domain: algorithms-data-models
difficulty: intermediate
last_reviewed: null
---

# Graph Representations

A graph models relationships between entities. The entities are vertices; the relationships are edges. Traversal and path algorithms are only as correct as the graph model they receive, so decide direction, weights, identity, ownership, and maximum size before choosing a representation.

## Graph Vocabulary

Vertex:
: A node or entity identified by a stable index or key.

Edge:
: A relationship between two vertices.

Directed edge:
: An edge from `u` to `v` that does not imply an edge from `v` to `u`.

Undirected edge:
: A relationship that can be traversed in both directions.

Weighted edge:
: An edge with a cost, distance, duration, or other numeric label.

Degree:
: The number of incident edges. Directed graphs distinguish in-degree and out-degree.

Path:
: A sequence of vertices connected by edges.

Simple path:
: A path that does not repeat a vertex.

## Representation Choices

| Representation | Storage | Edge lookup | Neighbor iteration | Good fit |
| --- | --- | --- | --- | --- |
| adjacency matrix | O(V²) | O(1) | O(V) | small dense graphs |
| adjacency list | O(V + E) | O(degree) | O(degree) | sparse graphs and traversal |
| edge list | O(E) | O(E) | O(E) | batch processing and Bellman-Ford |
| compressed sparse rows | O(V + E) | sequential by range | fast sequential scan | large static sparse graphs |

`V` is the vertex count and `E` is the edge count. A matrix can be simpler and faster for a small fixed topology, even though its asymptotic storage is larger. An adjacency list avoids reserving space for nonexistent relationships.

## Identity And Bounds

An embedded or systems graph should define:

- maximum vertex count
- maximum edge count
- whether vertex identifiers are dense indexes or external keys
- whether parallel edges are allowed
- whether self-loops are allowed
- whether edge weights are signed, bounded, or absent
- who owns graph storage and when it may be mutated

Dense integer vertex IDs make bitmaps and arrays practical. Sparse external IDs may need a mapping layer before graph algorithms run.

## Directed And Undirected Modeling

An undirected edge `(u, v)` is commonly stored as two directed adjacency entries. That makes neighbor traversal simple, but it consumes two edge slots and requires both entries to be updated consistently.

An undirected graph invariant can be stated as:

> For every stored edge from `u` to `v`, a matching edge from `v` to `u` exists with the same policy and weight.

If updates can fail because storage is full, add both directions transactionally or expose a partially updated state as an error that the caller must discard.

## Programming Examples

### C: Fixed-Capacity Adjacency List

This representation uses dense vertex indexes and caller-owned arrays. It stores directed edges; an undirected relationship is added by calling the directed operation twice.

```c
#include <stddef.h>

enum {
    GRAPH_MAX_VERTICES = 8,
    GRAPH_MAX_EDGES = 16,
    GRAPH_NO_EDGE = (size_t)-1
};

enum graph_status {
    GRAPH_OK = 0,
    GRAPH_ERR_NULL,
    GRAPH_ERR_VERTEX,
    GRAPH_ERR_FULL
};

struct graph_edge {
    size_t to;
    int weight;
    size_t next;
};

struct graph {
    size_t vertex_count;
    size_t edge_count;
    size_t heads[GRAPH_MAX_VERTICES];
    struct graph_edge edges[GRAPH_MAX_EDGES];
};

void graph_init(struct graph *graph, size_t vertex_count)
{
    if (graph == NULL)
        return;

    graph->vertex_count = vertex_count <= GRAPH_MAX_VERTICES
                        ? vertex_count
                        : 0;
    graph->edge_count = 0;
    for (size_t i = 0; i < GRAPH_MAX_VERTICES; i++)
        graph->heads[i] = GRAPH_NO_EDGE;
}

enum graph_status graph_add_directed(struct graph *graph,
                                     size_t from,
                                     size_t to,
                                     int weight)
{
    size_t edge_index;

    if (graph == NULL)
        return GRAPH_ERR_NULL;
    if (from >= graph->vertex_count || to >= graph->vertex_count)
        return GRAPH_ERR_VERTEX;
    if (graph->edge_count == GRAPH_MAX_EDGES)
        return GRAPH_ERR_FULL;

    edge_index = graph->edge_count++;
    graph->edges[edge_index] = (struct graph_edge){
        .to = to,
        .weight = weight,
        .next = graph->heads[from]
    };
    graph->heads[from] = edge_index;
    return GRAPH_OK;
}

enum graph_status graph_add_undirected(struct graph *graph,
                                       size_t first,
                                       size_t second,
                                       int weight)
{
    enum graph_status status;

    if (graph == NULL)
        return GRAPH_ERR_NULL;
    if (graph->edge_count > GRAPH_MAX_EDGES - 2)
        return GRAPH_ERR_FULL;

    status = graph_add_directed(graph, first, second, weight);
    if (status != GRAPH_OK)
        return status;

    status = graph_add_directed(graph, second, first, weight);
    if (status != GRAPH_OK) {
        /* The caller must discard the graph if this rollback is required. */
        return status;
    }
    return GRAPH_OK;
}
```

The linked-list representation stores each source vertex's neighbors through `heads` and `next`. Insertion is O(1), but neighbors are visited in reverse insertion order. If traversal order matters, define it explicitly or store and sort the adjacency entries.

### Python: Common Representations

```python
def adjacency_list(vertex_count, edges, directed=False):
    graph = [[] for _ in range(vertex_count)]
    for source, target, weight in edges:
        graph[source].append((target, weight))
        if not directed:
            graph[target].append((source, weight))
    return graph


def adjacency_matrix(vertex_count, edges, directed=False):
    matrix = [[None] * vertex_count for _ in range(vertex_count)]
    for source, target, weight in edges:
        matrix[source][target] = weight
        if not directed:
            matrix[target][source] = weight
    return matrix
```

The Python versions are useful for generating small test graphs. `None` represents no edge in the matrix; do not use zero as a sentinel when zero-weight edges are valid.

## Storage And Mutation Policy

Static graphs can use compact arrays and can be validated once before traversal. Dynamic graphs need a mutation policy:

- readers may observe only fully committed updates
- edge insertion either succeeds completely or reports a recoverable partial state
- traversal must not invalidate pointers or indexes while it is running
- deleting an edge must preserve the representation invariant

For a graph shared between a producer and a traversal thread, synchronization is part of the graph algorithm contract, not an implementation detail.

## Common Mistakes

- Treating an undirected edge as one adjacency entry and forgetting reverse traversal.
- Using zero to mean “no edge” when zero-weight edges are legal.
- Allowing invalid vertex indexes to reach array access.
- Assuming adjacency iteration order is stable after insertion changes.
- Exceeding edge capacity after reserving two entries for an undirected edge.
- Storing external pointers whose lifetime is shorter than the graph.
- Mutating topology during traversal without a snapshot or synchronization rule.

## Embedded And Systems Angle

- use matrices only when the vertex bound and density justify O(V²) memory
- prefer fixed adjacency lists for sparse bounded topologies
- validate weights and edge counts before running path algorithms
- use dense indexes when compact visited bitmaps are valuable
- make graph ownership and mutation boundaries explicit at driver or protocol interfaces

## Related Topics

- [Graph Algorithms](index.md)
- [Depth-First Search](depth-first-search.md)
- [Breadth-First Search](breadth-first-search.md)
- [Shortest Path Algorithms](shortest-path-algorithms.md)
- [Bitsets And Bitmaps](../data-structures-for-algorithms/bitsets-and-bitmaps.md)

---
status: draft
reviewed: false
domain: algorithms-data-models
difficulty: intermediate
last_reviewed: null
---

# Disjoint-Set Union

Disjoint-set union (DSU), also called union-find, maintains a partition of elements into non-overlapping sets. It supports finding the representative of an element's component and merging two components efficiently.

DSU is ideal when relationships are only added. It does not support arbitrary edge deletion, shortest paths, or directed reachability. Those require different structures or an offline rollback technique.

## Representation

Each element stores a parent index. A root is its own parent. The component representative is not inherently meaningful; it is an implementation choice unless the API promises a stable or deterministic representative.

The basic operations are:

- **make-set:** initialize each element as a one-element component
- **find(x):** return the root containing `x`
- **union(a, b):** merge the roots if they differ
- **connected(a, b):** compare their roots

With path compression and union by size or rank, a sequence of `m` operations on `n` elements has amortized cost O(m alpha(n)), where `alpha` grows so slowly that it is effectively constant for practical sizes.

## Path Compression

During `find`, every visited node can be linked directly to the root. The first lookup may walk a chain; later lookups become shorter. The transformation preserves the partition because all nodes on the path have the same representative.

## Union By Size Or Rank

When two roots merge, attach the smaller tree under the larger tree. This limits height before path compression. Rank is an upper-bound-like height measure; size tracks the number of elements. Use one consistently and update it only at roots.

### C: Bounded DSU

```c
#include <stddef.h>

enum {
    DSU_CAPACITY = 64,
    DSU_NONE = (size_t)-1
};

enum dsu_status {
    DSU_OK = 0,
    DSU_MERGED,
    DSU_ALREADY_CONNECTED,
    DSU_ERR_NULL,
    DSU_ERR_INDEX,
    DSU_ERR_CAPACITY
};

struct dsu {
    size_t parent[DSU_CAPACITY];
    size_t size[DSU_CAPACITY];
    size_t count;
    size_t component_count;
};

enum dsu_status dsu_init(struct dsu *set, size_t count)
{
    if (set == NULL)
        return DSU_ERR_NULL;
    if (count > DSU_CAPACITY)
        return DSU_ERR_CAPACITY;
    set->count = count;
    set->component_count = count;
    for (size_t i = 0; i < count; i++) {
        set->parent[i] = i;
        set->size[i] = 1;
    }
    return DSU_OK;
}

static enum dsu_status dsu_check(const struct dsu *set, size_t value)
{
    if (set == NULL)
        return DSU_ERR_NULL;
    if (value >= set->count)
        return DSU_ERR_INDEX;
    return DSU_OK;
}

enum dsu_status dsu_find(struct dsu *set, size_t value, size_t *out_root)
{
    size_t root;
    enum dsu_status status = dsu_check(set, value);

    if (status != DSU_OK || out_root == NULL)
        return status == DSU_OK ? DSU_ERR_NULL : status;
    root = value;
    while (set->parent[root] != root) {
        if (set->parent[root] >= set->count)
            return DSU_ERR_INDEX;
        root = set->parent[root];
    }

    while (set->parent[value] != value) {
        size_t next = set->parent[value];
        set->parent[value] = root;
        value = next;
    }
    *out_root = root;
    return DSU_OK;
}

enum dsu_status dsu_union(struct dsu *set, size_t left, size_t right)
{
    size_t left_root;
    size_t right_root;
    enum dsu_status status;

    status = dsu_find(set, left, &left_root);
    if (status != DSU_OK)
        return status;
    status = dsu_find(set, right, &right_root);
    if (status != DSU_OK)
        return status;
    if (left_root == right_root)
        return DSU_ALREADY_CONNECTED;
    if (set->size[left_root] < set->size[right_root]) {
        size_t temporary = left_root;
        left_root = right_root;
        right_root = temporary;
    }
    set->parent[right_root] = left_root;
    set->size[left_root] += set->size[right_root];
    set->component_count--;
    return DSU_MERGED;
}
```

The `find` implementation uses iteration rather than recursion, so its stack use is constant. The parent-index check protects against malformed or corrupted state; it is especially useful when the structure is restored from a wire format or shared memory.

## Component Counting And Cycle Detection

Initialize `component_count` to `n`. Every successful union decrements it. At the end, the value is the number of connected components among the elements that were initialized.

For an undirected edge list, an edge whose endpoints are already connected forms a cycle. This detects cycles while processing additions, but it is not a directed-cycle test and does not identify the cycle path without additional predecessor information.

## Kruskal's Minimum Spanning Tree

Kruskal sorts undirected weighted edges, then uses DSU to accept an edge only when its endpoints are in different components. The accepted edges form a minimum spanning forest; the forest is a single MST only if it contains `n - 1` edges for `n > 0`.

The DSU cost is nearly linear after the O(E log E) sort. If edge order is already a documented weight order, the sort can be omitted, but the caller then owns that precondition.

```python
def connected_components(vertex_count, edges):
    parent = list(range(vertex_count))
    size = [1] * vertex_count

    def find(value):
        while parent[value] != value:
            parent[value] = parent[parent[value]]
            value = parent[value]
        return value

    for left, right in edges:
        if not (0 <= left < vertex_count and 0 <= right < vertex_count):
            raise ValueError("vertex out of range")
        left_root, right_root = find(left), find(right)
        if left_root == right_root:
            continue
        if size[left_root] < size[right_root]:
            left_root, right_root = right_root, left_root
        parent[right_root] = left_root
        size[left_root] += size[right_root]

    groups = {}
    for vertex in range(vertex_count):
        groups.setdefault(find(vertex), []).append(vertex)
    return list(groups.values())
```

The Python model is useful for differential tests of the bounded C implementation. Its representative IDs depend on union order; compare partitions as sets of sets when representative identity is not part of the contract.

## Reset And Reuse

For repeated graph batches, reset cost is O(n). If batches touch only a small known subset, a timestamped lazy initialization can avoid clearing the whole array, but it adds generation overflow and stale-state rules. A rollback DSU can undo unions for offline divide-and-conquer processing, at the cost of storing a change log and avoiding path compression or recording every mutation.

## What DSU Cannot Answer

DSU cannot determine:

- the shortest path between two vertices
- whether a directed path exists
- which edges form a particular path
- whether an edge deletion disconnects a component in an online graph
- component geometry or minimum distance

Use BFS/DFS, shortest-path algorithms, dynamic connectivity structures, or an offline algorithm according to the actual operation mix.

## Common Mistakes

- Forgetting to initialize every parent to itself.
- Compressing a path and accidentally updating a non-root's size.
- Calling DSU connectivity a directed reachability result.
- Failing to validate element indexes before parent-array access.
- Treating the representative as stable when union order can change it.
- Using DSU for deletions without an explicit offline or rollback design.
- Reporting a minimum spanning tree for a disconnected graph.

## Embedded And Systems Angle

- size parent, rank/size, and optional generation arrays from the maximum ID
- use iterative find to bound call-stack usage
- validate parent indexes before pointer chasing
- define reset cost and reuse behavior for repeated graph batches
- make component count, cycle detection, and pool/capacity failures explicit
- choose a deterministic union policy when diagnostics or serialized output need stable roots

## Review Checklist

- Are IDs dense, bounded, and valid for direct indexing?
- Which operation mutates parent links, and is that mutation allowed during queries?
- Is representative identity observable or only component membership relevant?
- Are additions, deletions, rollback, and reset requirements separated?
- Does the MST caller distinguish a forest from a connected tree?
- Are malformed parent chains detected before they can loop indefinitely?

## Related Topics

- [Data Structures For Algorithms](index.md)
- [Advanced Graph Algorithms](../graph-algorithms/advanced-graph-algorithms.md)
- [Graph Representations](../graph-algorithms/graph-representations.md)
- [Heaps And Priority Queues](heaps-and-priority-queues.md)
- [Bounded Memory And Allocation Failure](../embedded-linux-algorithmic-constraints/bounded-memory-and-allocation-failure.md)

---
status: draft
reviewed: false
domain: algorithms-data-models
difficulty: intermediate
last_reviewed: null
---

# Deques

A deque, or double-ended queue, supports insertion and removal at both the front and the back. It is useful when an algorithm must process the oldest item, the newest item, or a priority represented by which end an item occupies.

## Endpoint Invariant

A ring-backed deque can be described by:

- `head`: index of the current front item
- `tail`: index one past the current back item
- `count`: number of stored items

The invariant is:

```text
0 <= count <= capacity
front is at head when count > 0
back is at (tail - 1) modulo capacity when count > 0
```

Using an explicit count allows all slots to be used and distinguishes full from empty. A one-slot-empty design avoids a count but reduces usable capacity and requires a clear wraparound rule.

## Operations And Costs

| Operation | Ring-backed deque | Linked-block deque |
| --- | --- | --- |
| push front/back | O(1) | O(1) amortized or worst case by block policy |
| pop front/back | O(1) | O(1) |
| peek front/back | O(1) | O(1) |
| random access | O(1) | O(number of blocks) or indexed |
| storage locality | contiguous | less predictable |

The ring is usually the best choice for bounded embedded work. Linked blocks can grow or shrink without moving all elements, but allocation failure, fragmentation, and lifetime ownership become part of every endpoint operation.

## C: Fixed-Capacity Ring Deque

```c
#include <stddef.h>

enum {
    DEQUE_CAPACITY = 16
};

enum deque_status {
    DEQUE_OK = 0,
    DEQUE_FULL,
    DEQUE_EMPTY,
    DEQUE_ERR_NULL
};

struct int_deque {
    int values[DEQUE_CAPACITY];
    size_t head;
    size_t tail;
    size_t count;
};

static size_t deque_previous(size_t index)
{
    return index == 0 ? DEQUE_CAPACITY - 1 : index - 1;
}

static size_t deque_next(size_t index)
{
    return index + 1 == DEQUE_CAPACITY ? 0 : index + 1;
}

enum deque_status deque_push_front(struct int_deque *deque, int value)
{
    if (deque == NULL)
        return DEQUE_ERR_NULL;
    if (deque->count == DEQUE_CAPACITY)
        return DEQUE_FULL;
    deque->head = deque_previous(deque->head);
    deque->values[deque->head] = value;
    deque->count++;
    return DEQUE_OK;
}

enum deque_status deque_push_back(struct int_deque *deque, int value)
{
    if (deque == NULL)
        return DEQUE_ERR_NULL;
    if (deque->count == DEQUE_CAPACITY)
        return DEQUE_FULL;
    deque->values[deque->tail] = value;
    deque->tail = deque_next(deque->tail);
    deque->count++;
    return DEQUE_OK;
}

enum deque_status deque_pop_front(struct int_deque *deque, int *out_value)
{
    if (deque == NULL || out_value == NULL)
        return DEQUE_ERR_NULL;
    if (deque->count == 0)
        return DEQUE_EMPTY;
    *out_value = deque->values[deque->head];
    deque->head = deque_next(deque->head);
    deque->count--;
    return DEQUE_OK;
}

enum deque_status deque_pop_back(struct int_deque *deque, int *out_value)
{
    if (deque == NULL || out_value == NULL)
        return DEQUE_ERR_NULL;
    if (deque->count == 0)
        return DEQUE_EMPTY;
    deque->tail = deque_previous(deque->tail);
    *out_value = deque->values[deque->tail];
    deque->count--;
    return DEQUE_OK;
}
```

When a deque is initialized with all indexes and count zero, `head` and `tail` may both be zero. After a sequence of operations they can differ even when the deque becomes empty; that is valid because `count` is the authoritative empty test. A validation helper should check `head` and `tail` are in range and `count <= capacity` before trusting deserialized state.

## Full-Queue Policy

When full, choose one policy explicitly:

- return `FULL` and preserve all existing items
- overwrite the oldest item
- overwrite the newest item
- drop the incoming item and increment a loss counter
- block or wait, if the execution context permits it

Overwriting is not a generic error recovery mechanism. It is appropriate only when the data model permits loss and the caller can observe which item was discarded.

## Monotonic Deques

A monotonic deque stores candidate indexes whose values are ordered. For a maximum window:

1. remove front indexes outside the current window
2. remove back indexes whose values are less than or equal to the new value
3. append the new index
4. read the front as the current maximum

Each index enters and leaves once, giving O(n) total work. Keep indexes instead of only values so expiration and duplicate values are handled correctly. Use `<` rather than `<=` when retaining older equal values is part of the tie policy.

## 0-1 BFS

For a graph whose edge weights are only zero or one, 0-1 BFS uses a deque instead of a heap. A zero-cost relaxation is pushed to the front; a one-cost relaxation is pushed to the back. The front of the deque contains the next smallest tentative distance under the algorithm's invariant.

This is a specialized shortest-path algorithm. If an edge weight outside `{0, 1}` appears, reject it or use Dijkstra. Do not silently treat a weight of two as one.

```python
from collections import deque


def zero_one_bfs(graph, source):
    distance = [float("inf")] * len(graph)
    distance[source] = 0
    pending = deque([source])
    while pending:
        vertex = pending.popleft()
        for neighbor, weight in graph[vertex]:
            if weight not in (0, 1):
                raise ValueError("edge weight must be zero or one")
            candidate = distance[vertex] + weight
            if candidate < distance[neighbor]:
                distance[neighbor] = candidate
                if weight == 0:
                    pending.appendleft(neighbor)
                else:
                    pending.append(neighbor)
    return distance
```

For production use, track stale entries or use a settled-state policy appropriate to the graph representation. The simple reference is intended to make the endpoint rule visible.

## Work Queues And Stealing

A deque is a natural local work queue: a worker takes its newest task from the back while another worker steals an older task from the front. The two-end policy preserves locality for the owner and exposes larger independent work units to thieves.

Endpoint operations are not automatically thread-safe. An SPSC deque may use a specialized acquire/release protocol, while MPMC work stealing requires careful atomic indexes, reservations, and memory reclamation. Start with a mutex or single-owner design if the concurrency contract is not proven.

## Common Mistakes

- Using `head == tail` as both full and empty without a count or reserved slot.
- Forgetting to move `tail` backward before reading the back item.
- Expiring a monotonic-queue value without storing its index.
- Treating equal values inconsistently and producing unstable window results.
- Silently overwriting data on full.
- Reusing a single-threaded deque in multiple producers or consumers.
- Assuming 0-1 BFS supports arbitrary non-negative weights.

## Embedded And Systems Angle

- prefer a bounded ring-backed deque when maximum work is known
- choose a full policy that preserves or reports data-loss semantics
- use contiguous storage when cache and DMA locality matter
- avoid pointer-heavy blocks unless growth or splicing justifies them
- keep monotonic auxiliary storage bounded by the window width
- make multi-producer and multi-consumer use explicit

## Review Checklist

- What do `head`, `tail`, and `count` mean after every operation?
- Can full and empty states be distinguished after wraparound?
- What happens to existing data when capacity is exhausted?
- Does the algorithm need values, indexes, or both?
- Are endpoint operations serialized or covered by a memory-ordering proof?
- Are partial, dropped, and stale items observable to the caller?

## Related Topics

- [Data Structures For Algorithms](index.md)
- [Ring Buffers](ring-buffers.md)
- [Practical Sequence Patterns](../basic-algorithm-schemes/practical-sequence-patterns.md)
- [Priority And Partial Ordering](../sorting-and-ordering/priority-and-partial-ordering.md)
- [Breadth-First Search](../graph-algorithms/breadth-first-search.md)
- [Atomic And Lock-Free Algorithm Patterns](../parallel-and-dataflow-algorithms/atomic-and-lock-free-algorithm-patterns.md)

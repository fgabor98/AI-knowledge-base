---
status: draft
reviewed: false
domain: algorithms-data-models
difficulty: intermediate
last_reviewed: null
---

# Priority And Partial Ordering

Many programs do not need a fully sorted sequence. They need the next most urgent item, the smallest pending deadline, the largest few values, or a result that satisfies a priority policy. A partial-order data structure can provide that result with less work than sorting every item.

The design question is:

> Which ordering facts must be maintained, and which ordering facts are unnecessary?

## Full Ordering Versus Priority

| Need | Suitable approach |
| --- | --- |
| iterate every item in order | full sort or sorted structure |
| repeatedly remove the smallest/largest item | heap or priority queue |
| keep only the best `k` items | bounded heap or selection algorithm |
| find a single minimum once | linear scan |
| preserve arrival order among equal priorities | priority plus sequence number |
| inspect arbitrary keys | sorted array, tree, or hash table |

A heap maintains the relationship between each parent and its children, not the total order of every pair. That is enough for efficient root selection.

## Priority Queue Contract

Define before implementing:

- whether lower or higher numbers have higher priority
- what happens when priorities tie
- whether insertion order is preserved for ties
- maximum pending item count
- behavior when the queue is full or empty
- whether an item may be updated or removed before reaching the root
- whether priorities can change while an item is queued

Without a tie policy, two correct implementations may produce different schedules and diagnostics.

## Heap Invariant

For a zero-based min-heap, the children of index `i` are:

```text
left  = 2 * i + 1
right = 2 * i + 2
```

The min-heap invariant is:

> Every parent is less than or equal to each of its children according to the priority comparator.

The root is therefore a minimum item, but the remaining array is not fully sorted.

Push appends an item and moves it upward until the invariant holds. Pop saves the root, moves the final item to the root, and moves it downward until the invariant holds.

## Top-K Selection

To retain the largest `k` values from a stream, use a min-heap of size `k`:

1. insert values until the heap is full
2. compare each later value with the heap minimum
3. discard it if it is no larger than the minimum
4. otherwise remove the minimum and insert the new value

The heap contains the current best `k` values, though not in sorted order. The time is O(n log k), and storage is O(k), which is preferable to sorting all `n` values when `k` is small.

## Programming Examples

### C: Fixed-Capacity Deterministic Priority Queue

This queue is a min-heap. Smaller `priority` values are served first; equal priorities use the insertion `order` as a deterministic tie-breaker.

```c
#include <stddef.h>
#include <stdint.h>

enum {
    PRIORITY_QUEUE_CAPACITY = 16
};

enum priority_queue_status {
    PRIORITY_QUEUE_OK = 0,
    PRIORITY_QUEUE_EMPTY,
    PRIORITY_QUEUE_FULL,
    PRIORITY_QUEUE_ERR_NULL,
    PRIORITY_QUEUE_ERR_ORDER
};

struct priority_item {
    int priority;
    uint64_t order;
    int value;
};

struct priority_queue {
    struct priority_item items[PRIORITY_QUEUE_CAPACITY];
    size_t count;
    uint64_t next_order;
};

static int priority_item_before(const struct priority_item *left,
                                const struct priority_item *right)
{
    if (left->priority != right->priority)
        return left->priority < right->priority;
    return left->order < right->order;
}

static void priority_swap(struct priority_item *left,
                          struct priority_item *right)
{
    struct priority_item temporary = *left;
    *left = *right;
    *right = temporary;
}

static void priority_sift_up(struct priority_queue *queue, size_t index)
{
    while (index > 0) {
        size_t parent = (index - 1) / 2;

        if (!priority_item_before(&queue->items[index],
                                  &queue->items[parent]))
            break;
        priority_swap(&queue->items[index], &queue->items[parent]);
        index = parent;
    }
}

static void priority_sift_down(struct priority_queue *queue, size_t index)
{
    for (;;) {
        size_t left = index * 2 + 1;
        size_t right = left + 1;
        size_t smallest = index;

        if (left < queue->count &&
            priority_item_before(&queue->items[left],
                                  &queue->items[smallest]))
            smallest = left;
        if (right < queue->count &&
            priority_item_before(&queue->items[right],
                                  &queue->items[smallest]))
            smallest = right;
        if (smallest == index)
            break;

        priority_swap(&queue->items[index], &queue->items[smallest]);
        index = smallest;
    }
}

enum priority_queue_status priority_queue_push(struct priority_queue *queue,
                                               int priority,
                                               int value)
{
    if (queue == NULL)
        return PRIORITY_QUEUE_ERR_NULL;
    if (queue->count == PRIORITY_QUEUE_CAPACITY)
        return PRIORITY_QUEUE_FULL;
    if (queue->next_order == UINT64_MAX)
        return PRIORITY_QUEUE_ERR_ORDER;

    queue->items[queue->count] = (struct priority_item){
        .priority = priority,
        .order = queue->next_order++,
        .value = value
    };
    queue->count++;
    priority_sift_up(queue, queue->count - 1);
    return PRIORITY_QUEUE_OK;
}

enum priority_queue_status priority_queue_pop(struct priority_queue *queue,
                                              struct priority_item *out_item)
{
    if (queue == NULL || out_item == NULL)
        return PRIORITY_QUEUE_ERR_NULL;
    if (queue->count == 0)
        return PRIORITY_QUEUE_EMPTY;

    *out_item = queue->items[0];
    queue->count--;
    if (queue->count > 0) {
        queue->items[0] = queue->items[queue->count];
        priority_sift_down(queue, 0);
    }
    return PRIORITY_QUEUE_OK;
}
```

Push and pop take O(log n), peek would take O(1), and the fixed queue uses O(n) storage with no allocation. The `uint64_t` sequence number makes ties deterministic until it reaches its documented limit.

### Python: Top-K Reference

```python
import heapq


def largest_k(values, k):
    if k < 0:
        raise ValueError("k must not be negative")
    if k == 0:
        return []

    heap = []
    for value in values:
        if len(heap) < k:
            heapq.heappush(heap, value)
        elif value > heap[0]:
            heapq.heapreplace(heap, value)
    return sorted(heap, reverse=True)
```

The Python heap contains only the current best `k` values. Sorting the final heap is optional and is done here only to make the returned result easy to inspect.

## Partial Ordering And Determinism

A scheduler-like algorithm often needs priority plus additional policy fields:

- deadline before best effort
- higher severity before lower severity
- older arrival before newer arrival
- lower identifier as a final reproducible tie-breaker

Put all required policy in the comparator. Do not depend on incidental array position or memory address for a result that must be reproducible.

## Common Mistakes

- Treating a heap array as if it were fully sorted.
- Using a comparator that does not define ties.
- Multiplying `2 * index + 1` without ensuring the index range is bounded.
- Forgetting to decrement the logical count before sifting down.
- Returning a pointer to an item that will move on the next operation.
- Using a heap for arbitrary-key lookup or range queries it does not support efficiently.
- Allowing priority updates without restoring the heap invariant.

## Embedded And Systems Angle

- use array-backed heaps for compact, predictable storage
- size the queue from maximum pending work and define full behavior
- make tie-breaking deterministic for reproducible scheduling and diagnostics
- avoid full sorting when only the next item or top `k` items matter
- account for comparator cost and bounded arithmetic in deadline-sensitive paths

## Related Topics

- [Sorting And Ordering](index.md)
- [Heaps And Priority Queues](../data-structures-for-algorithms/heaps-and-priority-queues.md)
- [Shortest Path Algorithms](../graph-algorithms/shortest-path-algorithms.md)
- [Deterministic Runtime And Real-Time Tradeoffs](../embedded-linux-algorithmic-constraints/deterministic-runtime-and-real-time-tradeoffs.md)

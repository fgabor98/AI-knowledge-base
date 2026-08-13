---
status: draft
reviewed: false
domain: algorithms-data-models
difficulty: intermediate
last_reviewed: null
---

# Heaps And Priority Queues

A heap is an array-backed tree that maintains a parent-child priority invariant. A priority queue uses that invariant to insert pending work and remove the highest- or lowest-priority item efficiently.

Unlike a sorted array, a heap does not fully order every item. It guarantees only enough ordering to identify the root.

## Binary Heap Layout

For a zero-based array:

```text
parent(i) = (i - 1) / 2       for i > 0
left(i)   = 2 * i + 1
right(i)  = 2 * i + 2
```

A min-heap satisfies:

> Every parent is less than or equal to its children.

A max-heap reverses the comparator. The root is the minimum or maximum, and the height is O(log n).

## Operations

Push appends at the next free slot and sifts upward. Pop saves the root, moves the last item into the root slot, decrements the count, and sifts downward. Peek reads the root without mutation.

| Operation | Cost |
| --- | --- |
| peek | O(1) |
| push | O(log n) |
| pop | O(log n) |
| build from n items | O(n) with bottom-up heapify |
| arbitrary search | O(n) |

If arbitrary removal or priority updates are required, store an item handle or position index and maintain it during every swap.

## Comparator And Ties

The comparator must be consistent. For deterministic scheduling, include a sequence number or stable identifier in the comparison after the primary priority. Otherwise equal-priority items may emerge in an order that depends on swaps and insertion history.

## Programming Examples

### C: Fixed-Capacity Min-Heap

```c
#include <stddef.h>
#include <stdint.h>

enum {
    HEAP_CAPACITY = 16
};

enum heap_status {
    HEAP_OK = 0,
    HEAP_EMPTY,
    HEAP_FULL,
    HEAP_ERR_NULL
};

struct heap_item {
    int priority;
    uint64_t sequence;
    int value;
};

struct min_heap {
    struct heap_item items[HEAP_CAPACITY];
    size_t count;
    uint64_t next_sequence;
};

static int heap_before(const struct heap_item *left,
                       const struct heap_item *right)
{
    if (left->priority != right->priority)
        return left->priority < right->priority;
    return left->sequence < right->sequence;
}

static void heap_swap(struct heap_item *left, struct heap_item *right)
{
    struct heap_item temporary = *left;
    *left = *right;
    *right = temporary;
}

static void heap_sift_up(struct min_heap *heap, size_t index)
{
    while (index > 0) {
        size_t parent = (index - 1) / 2;

        if (!heap_before(&heap->items[index], &heap->items[parent]))
            break;
        heap_swap(&heap->items[index], &heap->items[parent]);
        index = parent;
    }
}

static void heap_sift_down(struct min_heap *heap, size_t index)
{
    for (;;) {
        size_t left = index * 2 + 1;
        size_t right = left + 1;
        size_t best = index;

        if (left < heap->count &&
            heap_before(&heap->items[left], &heap->items[best]))
            best = left;
        if (right < heap->count &&
            heap_before(&heap->items[right], &heap->items[best]))
            best = right;
        if (best == index)
            return;

        heap_swap(&heap->items[index], &heap->items[best]);
        index = best;
    }
}

enum heap_status heap_push(struct min_heap *heap, int priority, int value)
{
    if (heap == NULL)
        return HEAP_ERR_NULL;
    if (heap->count == HEAP_CAPACITY)
        return HEAP_FULL;
    if (heap->next_sequence == UINT64_MAX)
        return HEAP_FULL;

    heap->items[heap->count] = (struct heap_item){
        .priority = priority,
        .sequence = heap->next_sequence++,
        .value = value
    };
    heap->count++;
    heap_sift_up(heap, heap->count - 1);
    return HEAP_OK;
}

enum heap_status heap_peek(const struct min_heap *heap,
                           struct heap_item *out_item)
{
    if (heap == NULL || out_item == NULL)
        return HEAP_ERR_NULL;
    if (heap->count == 0)
        return HEAP_EMPTY;
    *out_item = heap->items[0];
    return HEAP_OK;
}

enum heap_status heap_pop(struct min_heap *heap,
                          struct heap_item *out_item)
{
    if (heap == NULL || out_item == NULL)
        return HEAP_ERR_NULL;
    if (heap->count == 0)
        return HEAP_EMPTY;

    *out_item = heap->items[0];
    heap->count--;
    if (heap->count > 0) {
        heap->items[0] = heap->items[heap->count];
        heap_sift_down(heap, 0);
    }
    return HEAP_OK;
}
```

The fixed array bounds pending work. A public implementation should validate `count <= HEAP_CAPACITY` if the structure can be populated from external memory or deserialized state.

### C: Bottom-Up Heapify

```c
void heapify(struct heap_item *items, size_t count)
{
    struct min_heap view = { .count = count };

    if (items == NULL || count == 0 || count > HEAP_CAPACITY)
        return;
    for (size_t i = 0; i < count; i++)
        view.items[i] = items[i];
    for (size_t i = count / 2; i > 0; i--)
        heap_sift_down(&view, i - 1);
    for (size_t i = 0; i < count; i++)
        items[i] = view.items[i];
}
```

Bottom-up heapify takes O(n), unlike inserting every item one at a time, which takes O(n log n). The example copies through the fixed view for clarity; an in-place implementation can sift directly in the caller's array.

### Python: Heap Reference

```python
import heapq


def priority_order(items):
    pending = []
    for sequence, (priority, value) in enumerate(items):
        heapq.heappush(pending, (priority, sequence, value))
    result = []
    while pending:
        _, _, value = heapq.heappop(pending)
        result.append(value)
    return result
```

The sequence field provides deterministic tie-breaking just as in the C implementation.

## Heap Variants

A d-ary heap reduces height but increases the number of children examined during sift-down. A max-heap reverses priority. A bounded heap can discard low-value candidates while retaining the best `k` items. Choose the variant from the operation mix and capacity, not from asymptotic notation alone.

## Common Mistakes

- Treating the heap array as fully sorted.
- Forgetting to reduce logical count before sifting down.
- Using inconsistent comparators in push and pop.
- Allowing sequence-number overflow to reverse tie order.
- Updating an item's priority without restoring the invariant.
- Using a heap for exact-key lookup or range queries.
- Sizing a scheduler heap below its maximum pending-work count.

## Embedded And Systems Angle

- prefer array-backed heaps for compact predictable storage
- define tie-breaking for deterministic behavior
- size the heap from maximum pending work
- avoid allocation and resizing in deadline-sensitive paths
- expose full and empty status instead of silently dropping work

## Related Topics

- [Data Structures For Algorithms](index.md)
- [Priority And Partial Ordering](../sorting-and-ordering/priority-and-partial-ordering.md)
- [Shortest Path Algorithms](../graph-algorithms/shortest-path-algorithms.md)
- [Bounded Memory And Allocation Failure](../embedded-linux-algorithmic-constraints/bounded-memory-and-allocation-failure.md)

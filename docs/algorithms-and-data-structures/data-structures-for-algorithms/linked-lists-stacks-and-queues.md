---
status: draft
reviewed: false
domain: algorithms-data-models
difficulty: beginner
last_reviewed: null
---

# Linked Lists Stacks And Queues

Linked lists, stacks, and queues encode different access policies. A list links elements by position, a stack exposes last-in-first-out behavior, and a queue exposes first-in-first-out behavior.

These structures are simple to name but depend heavily on ownership and empty-state rules. Pointer links can make insertion cheap, but pointer chasing, allocation, and lifetime bugs may outweigh that benefit for small collections.

## Operation Policies

| Structure | Add | Remove | Access order | Typical use |
| --- | --- | --- | --- | --- |
| singly linked list | at head O(1) | known node O(1) | follow links | sparse ownership chains |
| stack | push O(1) | pop O(1) | newest first | parsing, DFS, undo |
| queue | enqueue O(1) | dequeue O(1) | oldest first | work handoff, BFS |
| array | append O(1) with capacity | end O(1) | index or insertion order | bounded dense data |

The operation contract matters more than the container label. A queue that can be full needs a full policy; a stack that can be empty needs an empty status; a list removal needs a lifetime rule for the removed node.

## Ownership Invariants

For a singly linked list:

- `head` is null or points to a valid owned node
- following `next` eventually reaches null
- no node appears twice in the chain
- each node is owned by at most one list

For a stack, `top` is either null or the newest node. For a queue, `head` is the oldest node and `tail` is the newest node; an empty queue has both null.

## Programming Examples

### C: Caller-Owned Fixed Node Pool Stack And Queue

This example avoids allocation by using indexes into a fixed node pool. A free-list index tracks available nodes; ownership transfers from the pool to the stack or queue.

```c
#include <stddef.h>

enum {
    COLLECTION_CAPACITY = 16,
    COLLECTION_NONE = (size_t)-1
};

enum collection_status {
    COLLECTION_OK = 0,
    COLLECTION_EMPTY,
    COLLECTION_FULL,
    COLLECTION_ERR_NULL,
    COLLECTION_ERR_INDEX
};

struct collection_node {
    int value;
    size_t next;
};

struct node_pool {
    struct collection_node nodes[COLLECTION_CAPACITY];
    size_t free_head;
};

struct index_stack {
    struct node_pool *pool;
    size_t top;
};

struct index_queue {
    struct node_pool *pool;
    size_t head;
    size_t tail;
};

void node_pool_init(struct node_pool *pool)
{
    if (pool == NULL)
        return;
    for (size_t i = 0; i < COLLECTION_CAPACITY; i++)
        pool->nodes[i].next = i + 1 < COLLECTION_CAPACITY
                            ? i + 1
                            : COLLECTION_NONE;
    pool->free_head = 0;
}

static enum collection_status pool_take(struct node_pool *pool,
                                        int value,
                                        size_t *out_index)
{
    size_t index;

    if (pool == NULL || out_index == NULL)
        return COLLECTION_ERR_NULL;
    if (pool->free_head == COLLECTION_NONE)
        return COLLECTION_FULL;

    index = pool->free_head;
    pool->free_head = pool->nodes[index].next;
    pool->nodes[index] = (struct collection_node){
        .value = value,
        .next = COLLECTION_NONE
    };
    *out_index = index;
    return COLLECTION_OK;
}

static void pool_release(struct node_pool *pool, size_t index)
{
    pool->nodes[index].next = pool->free_head;
    pool->free_head = index;
}

enum collection_status stack_push(struct index_stack *stack, int value)
{
    size_t index;
    enum collection_status status;

    if (stack == NULL || stack->pool == NULL)
        return COLLECTION_ERR_NULL;
    status = pool_take(stack->pool, value, &index);
    if (status != COLLECTION_OK)
        return status;
    stack->pool->nodes[index].next = stack->top;
    stack->top = index;
    return COLLECTION_OK;
}

enum collection_status stack_pop(struct index_stack *stack, int *out_value)
{
    size_t index;

    if (stack == NULL || stack->pool == NULL || out_value == NULL)
        return COLLECTION_ERR_NULL;
    if (stack->top == COLLECTION_NONE)
        return COLLECTION_EMPTY;

    index = stack->top;
    *out_value = stack->pool->nodes[index].value;
    stack->top = stack->pool->nodes[index].next;
    pool_release(stack->pool, index);
    return COLLECTION_OK;
}

enum collection_status queue_enqueue(struct index_queue *queue, int value)
{
    size_t index;
    enum collection_status status;

    if (queue == NULL || queue->pool == NULL)
        return COLLECTION_ERR_NULL;
    status = pool_take(queue->pool, value, &index);
    if (status != COLLECTION_OK)
        return status;

    if (queue->tail == COLLECTION_NONE)
        queue->head = index;
    else
        queue->pool->nodes[queue->tail].next = index;
    queue->tail = index;
    return COLLECTION_OK;
}

enum collection_status queue_dequeue(struct index_queue *queue,
                                     int *out_value)
{
    size_t index;

    if (queue == NULL || queue->pool == NULL || out_value == NULL)
        return COLLECTION_ERR_NULL;
    if (queue->head == COLLECTION_NONE)
        return COLLECTION_EMPTY;

    index = queue->head;
    *out_value = queue->pool->nodes[index].value;
    queue->head = queue->pool->nodes[index].next;
    if (queue->head == COLLECTION_NONE)
        queue->tail = COLLECTION_NONE;
    pool_release(queue->pool, index);
    return COLLECTION_OK;
}
```

The stack and queue must initialize their `top`, `head`, and `tail` fields to `COLLECTION_NONE` before use. The pool makes exhaustion a normal status instead of an implicit allocation failure.

### Python: Policy Reference

```python
from collections import deque


class Stack:
    def __init__(self):
        self.items = []

    def push(self, value):
        self.items.append(value)

    def pop(self):
        if not self.items:
            raise IndexError("stack is empty")
        return self.items.pop()


class Queue:
    def __init__(self):
        self.items = deque()

    def enqueue(self, value):
        self.items.append(value)

    def dequeue(self):
        if not self.items:
            raise IndexError("queue is empty")
        return self.items.popleft()
```

Python's standard containers provide the semantic reference; the C example makes capacity, ownership, and release paths visible.

## Linked Lists Versus Arrays

Lists are attractive when nodes are frequently inserted or removed from the middle and stable node identity matters. Arrays are usually better when the collection is small, indexed, scanned repeatedly, or moved in bulk.

List insertion is O(1) after a known predecessor, but finding that predecessor is O(n). Each node also carries link storage and may live in a separate cache line. A linked list does not automatically improve an algorithm just because insertion is constant time.

## Common Mistakes

- Leaving a queue tail pointing to a released node after removing the last item.
- Releasing a node while a caller still owns a pointer to it.
- Forgetting to initialize empty sentinels.
- Traversing a corrupted list without a cycle or step bound.
- Allocating for every item when a fixed pool would fit the maximum live count.
- Using a list where array indexing or locality is the dominant operation.

## Embedded And Systems Angle

- use lists only when pointer chasing and allocation costs are justified
- prefer bounded queues for producer-consumer handoff
- keep ownership transfer explicit at insertion and removal points
- use fixed pools or intrusive links when allocation failure must be bounded
- include empty, full, and corrupted-link behavior in tests

## Related Topics

- [Data Structures For Algorithms](index.md)
- [Ring Buffers](ring-buffers.md)
- [Intrusive Data Structures](intrusive-data-structures.md)
- [Memory Pools And Fixed-Size Allocators](memory-pools-and-fixed-size-allocators.md)
- [Breadth-First Search](../graph-algorithms/breadth-first-search.md)

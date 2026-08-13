---
status: draft
reviewed: false
domain: algorithms-data-models
difficulty: intermediate
last_reviewed: null
---

# Memory Pools And Fixed-Size Allocators

A memory pool reserves storage up front and allocates objects from that bounded region. A fixed-size allocator is a pool in which every slot has the same size, making allocation and release predictable and avoiding general-heap fragmentation for that object class.

The algorithmic benefit is not merely speed. A pool makes maximum live objects, exhaustion behavior, ownership, and cleanup paths visible.

## Pool Invariants

A valid fixed-size pool maintains:

- every slot is either free or owned by exactly one caller
- the free-list contains each free slot exactly once
- no allocated slot appears in the free-list
- allocation decreases free capacity by one
- release increases free capacity by one
- a released slot is not used through an old handle

If a pool carries a free count, check it against the free-list in debug builds. A double release can otherwise create a cycle or make one slot appear available twice.

## Sizing A Pool

Size from the maximum simultaneous live count, not the total number of objects processed over time. Include:

- normal peak occupancy
- error and retry paths
- objects held by asynchronous work
- temporary objects during replacement
- alignment and metadata overhead

If capacity is exhausted, choose whether the caller retries, degrades, drops work, uses a fallback, or fails the operation. Allocation failure is part of the algorithm contract.

## Programming Examples

### C: Index-Based Fixed-Size Pool

Using indexes as handles makes ownership and range checking explicit and avoids recovering a pool from an arbitrary pointer.

```c
#include <stddef.h>
#include <stdint.h>

enum {
    POOL_CAPACITY = 16,
    POOL_NONE = (size_t)-1
};

enum pool_status {
    POOL_OK = 0,
    POOL_EMPTY,
    POOL_ERR_NULL,
    POOL_ERR_INDEX,
    POOL_ERR_STATE
};

struct pool_object {
    uint32_t sequence;
    int value;
};

struct fixed_pool {
    struct pool_object objects[POOL_CAPACITY];
    size_t next_free[POOL_CAPACITY];
    unsigned char in_use[POOL_CAPACITY];
    size_t free_head;
    size_t free_count;
};

void fixed_pool_init(struct fixed_pool *pool)
{
    if (pool == NULL)
        return;
    for (size_t i = 0; i < POOL_CAPACITY; i++) {
        pool->next_free[i] = i + 1 < POOL_CAPACITY
                           ? i + 1
                           : POOL_NONE;
        pool->in_use[i] = 0;
    }
    pool->free_head = 0;
    pool->free_count = POOL_CAPACITY;
}

enum pool_status fixed_pool_alloc(struct fixed_pool *pool,
                                  size_t *out_index)
{
    size_t index;

    if (pool == NULL || out_index == NULL)
        return POOL_ERR_NULL;
    if (pool->free_head == POOL_NONE || pool->free_count == 0)
        return POOL_EMPTY;

    index = pool->free_head;
    if (index >= POOL_CAPACITY || pool->in_use[index] != 0)
        return POOL_ERR_STATE;
    pool->free_head = pool->next_free[index];
    pool->in_use[index] = 1;
    pool->free_count--;
    *out_index = index;
    return POOL_OK;
}

enum pool_status fixed_pool_get(struct fixed_pool *pool,
                                size_t index,
                                struct pool_object **out_object)
{
    if (pool == NULL || out_object == NULL)
        return POOL_ERR_NULL;
    if (index >= POOL_CAPACITY)
        return POOL_ERR_INDEX;
    if (pool->in_use[index] == 0)
        return POOL_ERR_STATE;
    *out_object = &pool->objects[index];
    return POOL_OK;
}

enum pool_status fixed_pool_release(struct fixed_pool *pool, size_t index)
{
    if (pool == NULL)
        return POOL_ERR_NULL;
    if (index >= POOL_CAPACITY)
        return POOL_ERR_INDEX;
    if (pool->in_use[index] == 0)
        return POOL_ERR_STATE;

    pool->in_use[index] = 0;
    pool->next_free[index] = pool->free_head;
    pool->free_head = index;
    pool->free_count++;
    return POOL_OK;
}
```

The pool returns `POOL_EMPTY` when no slot is available and `POOL_ERR_STATE` for an invalid double release or corrupted free-list state. A production implementation may omit `in_use` in a trusted hot path, but tests and debug builds benefit from the ownership check.

### C: Allocation Failure Policy

```c
enum allocation_policy_status {
    ALLOCATION_POLICY_OK = 0,
    ALLOCATION_POLICY_DROPPED,
    ALLOCATION_POLICY_FAILED
};

static enum allocation_policy_status make_optional_object(
    struct fixed_pool *pool,
    uint32_t sequence,
    int value,
    size_t *out_handle)
{
    size_t handle;
    struct pool_object *object;

    if (fixed_pool_alloc(pool, &handle) != POOL_OK)
        return ALLOCATION_POLICY_DROPPED;
    if (fixed_pool_get(pool, handle, &object) != POOL_OK)
        return ALLOCATION_POLICY_FAILED;

    object->sequence = sequence;
    object->value = value;
    *out_handle = handle;
    return ALLOCATION_POLICY_OK;
}
```

The caller must decide whether `DROPPED` is acceptable. If the object was allocated but a later initialization step fails, release the handle before returning failure.

### Python: Pool Reference

```python
class FixedPool:
    def __init__(self, capacity):
        if capacity < 0:
            raise ValueError("capacity must not be negative")
        self.objects = [None] * capacity
        self.free = list(range(capacity))

    def allocate(self, value):
        if not self.free:
            raise MemoryError("pool is exhausted")
        handle = self.free.pop()
        self.objects[handle] = value
        return handle

    def release(self, handle):
        if not 0 <= handle < len(self.objects):
            raise IndexError(handle)
        if self.objects[handle] is None:
            raise ValueError("double release")
        self.objects[handle] = None
        self.free.append(handle)
```

The Python model uses a list as a free stack and makes invalid handle behavior easy to test.

## Pool Classes And Fragmentation

Use separate pools for objects with different sizes or lifetimes. One mixed-size pool either wastes space or needs a more complex allocator. Fixed-size pools avoid external fragmentation, but they can still suffer internal waste when the slot is much larger than the common object.

For variable-size data, consider slabs, regions, or caller-owned buffers. Each alternative has a different release and failure model.

## Lifetime And Asynchronous Work

An object handed to a worker, timer, interrupt-deferred callback, or queue remains live until that consumer releases it. Pool sizing must include these in-flight objects. Reusing a slot too early can turn a valid handle into an alias for unrelated work.

## Common Mistakes

- Sizing only for the steady-state count and ignoring temporary peaks.
- Returning a raw pointer without a lifetime or release rule.
- Double-releasing a slot and corrupting the free-list.
- Treating pool exhaustion as impossible.
- Holding a pool object across an asynchronous boundary without accounting for it.
- Mixing object sizes or lifetimes in one fixed-size pool.
- Releasing an object before queued consumers have finished.

## Embedded And Systems Angle

- size pools from worst-case live object counts
- make exhaustion behavior part of the algorithm contract
- keep ownership and release paths auditable
- prefer index handles when bounds and stale-use checks matter
- avoid allocation in interrupt context and other non-failing paths

## Related Topics

- [Data Structures For Algorithms](index.md)
- [Bounded Memory And Allocation Failure](../embedded-linux-algorithmic-constraints/bounded-memory-and-allocation-failure.md)
- [Intrusive Data Structures](intrusive-data-structures.md)
- [Linked Lists Stacks And Queues](linked-lists-stacks-and-queues.md)

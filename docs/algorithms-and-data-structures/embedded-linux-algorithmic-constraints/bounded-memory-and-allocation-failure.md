---
status: draft
reviewed: false
domain: algorithms-data-models
difficulty: intermediate
last_reviewed: null
---

# Bounded Memory And Allocation Failure

An algorithm has bounded memory when its maximum live storage can be stated as a function of documented inputs and fixed system limits. On embedded Linux, allocation failure, fragmentation, reclaim behavior, and asynchronous ownership can affect correctness and timing, so memory policy belongs in the algorithm design.

## Memory Budget Model

Separate memory into:

- input and output buffers
- persistent state
- temporary working storage
- stack usage
- queued or in-flight objects
- allocator metadata and alignment overhead

Peak memory, not average memory, determines whether a path is safe. A pipeline may use little memory per item but exceed the budget when every stage holds an item and queues are full.

A useful budget statement is:

```text
peak = persistent + input + output + temporary + queue_high_water + stack
```

State the unit and maximum for each term. “Uses little memory” is not a testable contract.

## Allocation Policies

Choose deliberately among:

- static or compile-time storage
- caller-provided buffers
- preallocated pools
- bounded dynamic allocation before entering a critical path
- general heap allocation with recoverable failure

Preallocation moves failure earlier, where it may be easier to report. It does not remove the need to size for worst-case live objects.

## Failure Policies

When allocation fails, an algorithm can:

- return an error without changing existing state
- drop optional work and report the drop
- use a smaller or slower fallback
- retry after releasing temporary resources
- enter a degraded mode
- fail the owning operation or service

Do not retry blindly. Repeated allocation attempts can turn a local failure into an unbounded latency path.

## Programming Examples

### C: Transactional Growth Into Caller Storage

This vector never reallocates. Appending either writes one complete item or leaves the logical state unchanged.

```c
#include <stddef.h>

enum bounded_vector_status {
    BOUNDED_VECTOR_OK = 0,
    BOUNDED_VECTOR_FULL,
    BOUNDED_VECTOR_ERR_NULL,
    BOUNDED_VECTOR_ERR_STATE
};

struct bounded_vector {
    int *values;
    size_t count;
    size_t capacity;
};

enum bounded_vector_status bounded_vector_push(struct bounded_vector *vector,
                                               int value)
{
    if (vector == NULL)
        return BOUNDED_VECTOR_ERR_NULL;
    if (vector->count > vector->capacity ||
        (vector->values == NULL && vector->capacity > 0))
        return BOUNDED_VECTOR_ERR_STATE;
    if (vector->count == vector->capacity)
        return BOUNDED_VECTOR_FULL;

    vector->values[vector->count++] = value;
    return BOUNDED_VECTOR_OK;
}
```

The full status occurs before mutation. The caller can choose to drop the item, flush old entries, or fail a larger operation without recovering from a partial append.

### C: Pool Allocation With Failure Reporting

```c
enum object_status {
    OBJECT_OK = 0,
    OBJECT_NO_MEMORY,
    OBJECT_ERR_NULL
};

struct object_pool {
    struct object *objects;
    size_t capacity;
    size_t in_use;
};

struct object;

static enum object_status object_create(struct object_pool *pool,
                                        struct object **out_object)
{
    if (pool == NULL || out_object == NULL)
        return OBJECT_ERR_NULL;
    if (pool->in_use == pool->capacity)
        return OBJECT_NO_MEMORY;

    *out_object = &pool->objects[pool->in_use++];
    return OBJECT_OK;
}
```

This is a deliberately small allocation sketch. A real pool must also track release and free slots; the important contract is that exhaustion is an ordinary result and not an unchecked null dereference.

### Python: Capacity Reference

```python
class BoundedList:
    def __init__(self, capacity):
        if capacity < 0:
            raise ValueError("capacity must not be negative")
        self.capacity = capacity
        self.values = []

    def append(self, value):
        if len(self.values) == self.capacity:
            raise MemoryError("bounded storage is full")
        self.values.append(value)
```

Python's list may allocate internally, but the wrapper preserves the logical capacity policy for testing.

## Failure Atomicity

An operation is failure-atomic when failure leaves the data structure in its previous valid state. For multi-step operations:

1. validate capacity and input
2. reserve all required resources
3. perform mutations
4. publish the new state only after all steps succeed
5. release temporary resources on every failure path

If partial progress is useful, return a distinct partial status and a precise count or cursor. Never make callers guess which fields changed.

## Allocation Contexts

The same allocation may be acceptable in process context but invalid in an interrupt, atomic, or watchdog-sensitive path. Document the execution context and whether allocation may sleep, reclaim, or block.

On Linux, kernel and userspace allocation APIs have different rules. Algorithm pages should state the policy and direct implementation work to the relevant subsystem documentation rather than hiding context assumptions.

## Common Mistakes

- Planning for average occupancy instead of peak live objects.
- Dereferencing an allocation result before checking it.
- Retrying until memory becomes available in a deadline-sensitive path.
- Losing ownership when an operation fails halfway through.
- Returning a pointer to temporary storage after a successful call.
- Treating a full bounded queue as an impossible state.

## Embedded And Systems Angle

- prefer preallocation where failure cannot be handled locally
- include error paths in the algorithm design
- document what happens when capacity is reached
- separate optional work from mandatory state
- measure stack, queue, and allocator overhead in the actual build

## Related Topics

- [Embedded Linux Algorithmic Constraints](index.md)
- [Memory Pools And Fixed-Size Allocators](../data-structures-for-algorithms/memory-pools-and-fixed-size-allocators.md)
- [Time And Space Complexity](../complexity-and-efficiency/time-and-space-complexity.md)
- [Ring Buffers](../data-structures-for-algorithms/ring-buffers.md)

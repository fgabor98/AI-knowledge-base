---
status: draft
reviewed: false
domain: algorithms-data-models
difficulty: intermediate
last_reviewed: null
---

# Ring Buffers

A ring buffer is a bounded FIFO collection stored in a circular array. The head identifies the oldest item, the tail identifies the next insertion position, and indexes wrap at the buffer capacity.

Ring buffers are useful when data arrives as a stream and the maximum queued amount is known. They are common in logging, serial input, audio, DMA descriptors, and interrupt-to-thread handoff.

## Core Invariants

For a count-based ring buffer:

- `0 <= count <= capacity`
- `head < capacity` when capacity is nonzero
- `tail = (head + count) mod capacity`
- the logical sequence begins at `head`
- only the first `count` logical positions contain queued items

Using an explicit count removes the ambiguity between empty and full states that occurs when head and tail alone are equal.

## Full-Buffer Policies

When full, an enqueue can:

- reject the new item
- overwrite the oldest item
- drop the newest item and report a drop
- block or wait for a consumer

Reject is appropriate when every item matters. Overwrite is appropriate for latest-value telemetry or bounded history. Blocking is not appropriate in interrupt context unless the context and synchronization model explicitly permit it.

## Single-Producer Single-Consumer

An SPSC ring buffer can often avoid a lock when exactly one producer writes the tail, exactly one consumer writes the head, and the platform's memory-ordering requirements are satisfied. The indexes and data publication still need the correct atomic or barrier semantics for the target. A plain C struct is not automatically a lock-free queue.

## Programming Examples

### C: Fixed-Size Reject-on-Full Ring Buffer

```c
#include <stddef.h>

enum {
    RING_CAPACITY = 8
};

enum ring_status {
    RING_OK = 0,
    RING_EMPTY,
    RING_FULL,
    RING_ERR_NULL
};

struct int_ring {
    int values[RING_CAPACITY];
    size_t head;
    size_t count;
};

void int_ring_init(struct int_ring *ring)
{
    if (ring == NULL)
        return;
    ring->head = 0;
    ring->count = 0;
}

enum ring_status int_ring_push(struct int_ring *ring, int value)
{
    size_t tail;

    if (ring == NULL)
        return RING_ERR_NULL;
    if (ring->count == RING_CAPACITY)
        return RING_FULL;

    tail = (ring->head + ring->count) % RING_CAPACITY;
    ring->values[tail] = value;
    ring->count++;
    return RING_OK;
}

enum ring_status int_ring_pop(struct int_ring *ring, int *out_value)
{
    if (ring == NULL || out_value == NULL)
        return RING_ERR_NULL;
    if (ring->count == 0)
        return RING_EMPTY;

    *out_value = ring->values[ring->head];
    ring->head = (ring->head + 1) % RING_CAPACITY;
    ring->count--;
    return RING_OK;
}
```

Push and pop are O(1), and storage is exactly the fixed capacity. The capacity must be positive; if a generic ring accepts caller-provided capacity, validate it before using modulo.

### C: Overwrite-Oldest Policy

```c
enum ring_status int_ring_push_overwrite(struct int_ring *ring, int value)
{
    size_t tail;

    if (ring == NULL)
        return RING_ERR_NULL;

    tail = (ring->head + ring->count) % RING_CAPACITY;
    ring->values[tail] = value;
    if (ring->count < RING_CAPACITY) {
        ring->count++;
        return RING_OK;
    }

    ring->head = (ring->head + 1) % RING_CAPACITY;
    return RING_FULL;
}
```

Returning `RING_FULL` after overwriting makes data loss visible while still accepting the newest value. A separate `RING_OVERWROTE` status may be clearer in a public API.

### Python: Reference Ring

```python
class Ring:
    def __init__(self, capacity):
        if capacity <= 0:
            raise ValueError("capacity must be positive")
        self.values = [None] * capacity
        self.head = 0
        self.count = 0

    def push(self, value):
        tail = (self.head + self.count) % len(self.values)
        overwritten = self.count == len(self.values)
        self.values[tail] = value
        if overwritten:
            self.head = (self.head + 1) % len(self.values)
        else:
            self.count += 1
        return overwritten

    def pop(self):
        if self.count == 0:
            raise IndexError("ring is empty")
        value = self.values[self.head]
        self.head = (self.head + 1) % len(self.values)
        self.count -= 1
        return value
```

The returned boolean indicates whether the push discarded the oldest item.

## Batch Operations

Stream code often benefits from push-many and pop-many operations. A batch operation should return the number actually transferred and preserve the same full/empty policy as single-item operations. Do not expose a raw contiguous pointer across the wrap boundary without reporting how many elements are in each segment.

## Concurrency And Memory Ordering

For a concurrent ring, specify:

- producer and consumer count
- which fields each context writes
- whether an item is copied or ownership-transferred
- when a slot becomes visible
- whether the buffer is cache-coherent
- what synchronization primitive or memory order is required

The data structure algorithm and the memory model must agree. A logically correct index update can still publish partially written data if ordering is wrong.

## Common Mistakes

- Using head equals tail as both empty and full without another state bit or count.
- Advancing an index before the item is fully written.
- Modulo by zero in a generic zero-capacity buffer.
- Overwriting data without reporting the loss.
- Assuming a ring is thread-safe because each operation is short.
- Mixing signed and unsigned index arithmetic at wraparound.
- Returning a pointer to a slot that another context may immediately reuse.

## Embedded And Systems Angle

- use ring buffers for interrupt-to-thread and stream handoff
- define full-buffer behavior before implementation
- choose capacity and index types that avoid ambiguous states
- keep interrupt-side operations short and bounded
- account for cache-line ownership and memory barriers in concurrent use

## Related Topics

- [Data Structures For Algorithms](index.md)
- [Interrupt-Safe Queues And Buffers](../embedded-linux-algorithmic-constraints/interrupt-safe-queues-and-buffers.md)
- [Breadth-First Search](../graph-algorithms/breadth-first-search.md)
- [Pipeline And Dataflow Algorithms](../parallel-and-dataflow-algorithms/pipeline-and-dataflow-algorithms.md)

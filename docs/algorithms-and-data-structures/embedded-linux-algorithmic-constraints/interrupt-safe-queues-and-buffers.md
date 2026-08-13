---
status: draft
reviewed: false
domain: algorithms-data-models
difficulty: intermediate
last_reviewed: null
---

# Interrupt-Safe Queues And Buffers

Queues that cross interrupt, thread, worker, or deferred-work boundaries must define more than FIFO order. They need a context policy: which side may block, allocate, disable interrupts, take a lock, or access each field.

The interrupt-side operation should be short, bounded, and independent of resources that may sleep or fail unpredictably.

## Producer-Consumer Contract

Define:

- number and context of producers
- number and context of consumers
- item ownership before enqueue and after dequeue
- maximum queue capacity
- full policy: reject, drop, overwrite, or backpressure
- empty policy: return, block, or schedule work
- synchronization and memory-ordering requirements
- shutdown or reset behavior

An SPSC ring is a common solution for one producer and one consumer. Multiple producers or consumers need stronger coordination or separate queues with a merge step.

## Interrupt Constraints

An interrupt handler generally should not:

- allocate from a sleeping allocator
- block waiting for a consumer
- perform unbounded work
- call APIs that require process context unless explicitly allowed
- hold a lock across operations that can be preempted or block

The exact rules are kernel and architecture specific. The algorithm should expose a fast enqueue path and defer expensive processing to a thread, tasklet, workqueue, or other appropriate mechanism.

## Programming Examples

### C: SPSC Event Ring With Drop Reporting

This is a data-structure sketch. Real kernel code must use the synchronization and memory-order primitives required by its context and architecture.

```c
#include <stddef.h>
#include <stdint.h>

enum {
    EVENT_RING_CAPACITY = 16
};

enum event_ring_status {
    EVENT_RING_OK = 0,
    EVENT_RING_EMPTY,
    EVENT_RING_FULL,
    EVENT_RING_ERR_NULL
};

struct event {
    uint32_t type;
    uint32_t data;
};

struct event_ring {
    struct event events[EVENT_RING_CAPACITY];
    size_t head;
    size_t tail;
};

enum event_ring_status event_ring_push(struct event_ring *ring,
                                       struct event event)
{
    size_t next;

    if (ring == NULL)
        return EVENT_RING_ERR_NULL;
    next = (ring->tail + 1) % EVENT_RING_CAPACITY;
    if (next == ring->head)
        return EVENT_RING_FULL;

    ring->events[ring->tail] = event;
    ring->tail = next;
    return EVENT_RING_OK;
}

enum event_ring_status event_ring_pop(struct event_ring *ring,
                                      struct event *out_event)
{
    if (ring == NULL || out_event == NULL)
        return EVENT_RING_ERR_NULL;
    if (ring->head == ring->tail)
        return EVENT_RING_EMPTY;

    *out_event = ring->events[ring->head];
    ring->head = (ring->head + 1) % EVENT_RING_CAPACITY;
    return EVENT_RING_OK;
}
```

This head/tail design reserves one slot, so usable capacity is `EVENT_RING_CAPACITY - 1`. A count-based design can use every slot but needs count ownership and synchronization. The producer writes the event before publishing the new tail; the consumer reads the event before publishing the new head.

### C: Deferred-Work Policy

```c
enum irq_event_result {
    IRQ_EVENT_QUEUED = 0,
    IRQ_EVENT_DROPPED,
    IRQ_EVENT_ERR_NULL
};

struct irq_context {
    struct event_ring *ring;
    size_t dropped;
    int worker_pending;
};

enum irq_event_result irq_capture_event(struct irq_context *context,
                                        struct event event)
{
    enum event_ring_status status;

    if (context == NULL || context->ring == NULL)
        return IRQ_EVENT_ERR_NULL;
    status = event_ring_push(context->ring, event);
    if (status == EVENT_RING_FULL) {
        context->dropped++;
        return IRQ_EVENT_DROPPED;
    }
    if (status != EVENT_RING_OK)
        return IRQ_EVENT_ERR_NULL;

    context->worker_pending = 1;
    return IRQ_EVENT_QUEUED;
}
```

The handler records a bounded event and schedules deferred work. The worker can drain several events, perform parsing or I/O, and clear the pending state according to the synchronization design.

### Python: Policy Reference

```python
from collections import deque


class DroppingQueue:
    def __init__(self, capacity):
        if capacity <= 0:
            raise ValueError("capacity must be positive")
        self.queue = deque(maxlen=capacity)
        self.dropped = 0

    def push(self, item):
        if len(self.queue) == self.queue.maxlen:
            self.dropped += 1
            return False
        self.queue.append(item)
        return True

    def pop(self):
        return self.queue.popleft() if self.queue else None
```

Python makes the drop policy easy to test; it does not model interrupt context or memory ordering.

## Visibility And Synchronization

The producer must publish the item before publishing the index that tells the consumer it is available. The consumer must finish reading an item before publishing the slot as free. On coherent systems this still requires the appropriate atomic operations or barriers; on non-coherent DMA paths it also requires cache maintenance.

Do not infer a memory-ordering guarantee from the fact that an integer write is usually atomic on the target.

## Full-Buffer Policies

Reject and count drops when each event is independent and loss is observable. Overwrite only when the newest state supersedes old state. Backpressure may be valid for a thread producer but is usually impossible for an interrupt producer. If loss is unacceptable, increase capacity, reduce producer work, or change the handoff design.

## Common Mistakes

- Calling a blocking or allocating API from an interrupt context.
- Publishing the tail before the event payload is complete.
- Using a queue designed for one producer with multiple producers.
- Silently dropping events when diagnostics depend on complete history.
- Treating a scheduled-worker flag as synchronization by itself.
- Resetting head and tail concurrently without a quiescence rule.
- Reusing an event buffer before the consumer has finished.

## Embedded And Systems Angle

- keep interrupt-side operations short and bounded
- avoid blocking allocation in interrupt context
- define synchronization requirements for each producer and consumer pair
- report drops, high-water marks, and deferred-work failures
- separate capture from parsing, logging, and device I/O

## Related Topics

- [Embedded Linux Algorithmic Constraints](index.md)
- [Ring Buffers](../data-structures-for-algorithms/ring-buffers.md)
- [Pipeline And Dataflow Algorithms](../parallel-and-dataflow-algorithms/pipeline-and-dataflow-algorithms.md)
- [Cache-Aware And DMA-Friendly Layouts](cache-aware-and-dma-friendly-layouts.md)

---
status: draft
reviewed: false
domain: algorithms-data-models
difficulty: advanced
last_reviewed: null
---

# Atomic And Lock-Free Algorithm Patterns

Atomic algorithms coordinate shared state without placing every operation inside a conventional mutex-protected critical section. They can reduce blocking and improve throughput, but they require a precise ownership model, memory-ordering argument, and object-lifetime policy.

“Lock-free” is a progress guarantee, not a synonym for “uses atomics.” A compare-and-swap loop can still starve one thread, spin for an unbounded time, or access an object whose lifetime ended.

## Atomic Operations

The basic C11 operations are:

- load and store
- exchange
- fetch-add, fetch-sub, and bitwise read-modify-write
- compare-and-exchange

Use the weakest ordering that supports the proof, but only after stating the proof. `memory_order_relaxed` gives atomicity and modification order for one object without publishing unrelated data. Acquire and release create a synchronizes-with relationship when an acquire observes a release. Sequential consistency is easier to reason about but can be more expensive and is not a replacement for ownership design.

## Publication Pattern

A producer can initialize an object with ordinary stores, then publish a pointer or index using a release store. A consumer that reads it with an acquire load sees the initialization before using the object.

The ordering only works when the consumer actually observes the release sequence and the object remains alive. An atomic pointer does not make the pointed-to object safe to mutate or reclaim concurrently.

## Compare-And-Swap Loops

Compare-and-exchange reads a value, compares it with an expected value, and conditionally replaces it. On failure it writes the observed value back to `expected`; a loop must use that updated value.

### C: Bounded Atomic Counter

```c
#include <stdatomic.h>

enum counter_status {
    COUNTER_OK = 0,
    COUNTER_LIMIT,
    COUNTER_ERR_NULL
};

struct bounded_counter {
    atomic_uint value;
    unsigned maximum;
};

enum counter_status counter_try_increment(struct bounded_counter *counter)
{
    unsigned expected;

    if (counter == NULL)
        return COUNTER_ERR_NULL;
    expected = atomic_load_explicit(&counter->value,
                                    memory_order_relaxed);
    for (;;) {
        if (expected >= counter->maximum)
            return COUNTER_LIMIT;
        if (atomic_compare_exchange_weak_explicit(
                &counter->value,
                &expected,
                expected + 1,
                memory_order_relaxed,
                memory_order_relaxed))
            return COUNTER_OK;
        /* Failure updates expected with the current counter value. */
    }
}
```

The increment is atomic, but the `maximum` field must be immutable while the counter is in use. The loop may retry repeatedly under contention. If a caller cannot tolerate that, add a retry budget and return a contention status; that changes the API from “eventually try” to “best effort within this budget.”

## Single-Producer Single-Consumer Ring

An SPSC ring can avoid a mutex when exactly one producer writes `head` and exactly one consumer writes `tail`. The producer checks the consumer's tail with acquire, writes the element, then publishes the new head with release. The consumer checks head with acquire, reads the element, then publishes the new tail with release.

### C: Acquire/Release SPSC Queue

```c
#include <stdatomic.h>
#include <stddef.h>

enum {
    SPSC_CAPACITY = 16
};

enum spsc_status {
    SPSC_OK = 0,
    SPSC_FULL,
    SPSC_EMPTY,
    SPSC_ERR_NULL
};

struct spsc_queue {
    int values[SPSC_CAPACITY];
    atomic_size_t head;
    atomic_size_t tail;
};

void spsc_init(struct spsc_queue *queue)
{
    if (queue == NULL)
        return;
    atomic_init(&queue->head, 0);
    atomic_init(&queue->tail, 0);
}

enum spsc_status spsc_push(struct spsc_queue *queue, int value)
{
    size_t head;
    size_t next;
    size_t tail;

    if (queue == NULL)
        return SPSC_ERR_NULL;
    head = atomic_load_explicit(&queue->head, memory_order_relaxed);
    next = (head + 1) % SPSC_CAPACITY;
    tail = atomic_load_explicit(&queue->tail, memory_order_acquire);
    if (next == tail)
        return SPSC_FULL;

    queue->values[head] = value;
    atomic_store_explicit(&queue->head, next, memory_order_release);
    return SPSC_OK;
}

enum spsc_status spsc_pop(struct spsc_queue *queue, int *out_value)
{
    size_t tail;
    size_t head;
    size_t next;

    if (queue == NULL || out_value == NULL)
        return SPSC_ERR_NULL;
    tail = atomic_load_explicit(&queue->tail, memory_order_relaxed);
    head = atomic_load_explicit(&queue->head, memory_order_acquire);
    if (tail == head)
        return SPSC_EMPTY;

    *out_value = queue->values[tail];
    next = (tail + 1) % SPSC_CAPACITY;
    atomic_store_explicit(&queue->tail, next, memory_order_release);
    return SPSC_OK;
}
```

This representation uses one slot as the full/empty discriminator, so the usable capacity is `SPSC_CAPACITY - 1`. It is not safe for multiple producers or multiple consumers without a different reservation protocol. Cache-line padding may be needed to reduce false sharing, but padding does not change the memory-ordering proof.

## Progress Guarantees

- **Wait-free:** every operation completes in a bounded number of its own steps.
- **Lock-free:** system-wide progress occurs; some operation completes after a finite number of steps, but one thread may starve.
- **Obstruction-free:** an operation completes if it runs alone for long enough.
- **Blocking:** progress can depend on a lock owner or scheduler.

CAS loops are often lock-free in an abstract model, but bounded retry loops are wait-free only if the retry count is truly bounded and the failure path is valid. Interrupts, cache coherence, page faults, and preemption still affect wall-clock latency.

## ABA And Object Lifetime

The ABA problem occurs when a value changes from A to B and back to A. A CAS that sees A may incorrectly assume nothing relevant changed. Generation tags, hazard pointers, epochs, or a reclamation scheme can address this, but each has storage and progress costs.

Never reclaim an object merely because an atomic pointer no longer references it. A reader may have loaded the pointer before the removal. Memory reclamation is a separate algorithm involving ownership, quiescence, or reader registration.

## Counters And Flags

Use relaxed atomics for independent statistics when no publication relationship is needed. Use release/acquire for a ready flag that publishes a payload. A flag does not make multiple fields independently safe if writers can update them concurrently; protect the complete state with a protocol or a lock.

## Python: Sequential Reference Model

```python
from collections import deque


class QueueModel:
    def __init__(self, capacity):
        if capacity <= 0:
            raise ValueError("capacity must be positive")
        self.capacity = capacity
        self.values = deque()

    def push(self, value):
        if len(self.values) == self.capacity:
            return False
        self.values.append(value)
        return True

    def pop(self):
        if not self.values:
            return None
        return self.values.popleft()
```

The model is deliberately sequential. It checks FIFO and full/empty policy but says nothing about the C memory model. Concurrency tests still need thread sanitizers, stress schedules, and a review of the happens-before argument.

## Common Mistakes

- Using relaxed ordering when the operation also publishes ordinary data.
- Assuming an atomic pointer makes the pointed-to object immutable or immortal.
- Reusing an SPSC queue as an MPSC or MPMC queue without a new proof.
- Treating a failed weak CAS as an error instead of retrying with updated expected state.
- Claiming wait-free behavior for an unbounded CAS loop.
- Ignoring false sharing, cache-line ownership, or target-specific atomic support.
- Reclaiming nodes without addressing ABA and readers that hold old pointers.

## Embedded And Systems Angle

- state producer and consumer counts before choosing an atomic queue pattern
- check whether the target supports the required atomic width without hidden locks
- bound retries or expose contention and queue-full results
- keep object lifetime and memory reclamation separate from publication
- place hot atomic fields to reduce false sharing where measurement justifies it
- use a mutex or interrupt masking when it gives a simpler, safer bounded contract

## Review Checklist

- Which fields are atomic, and which ordinary fields do they publish?
- What event establishes the happens-before relationship?
- Who owns each index, pointer, and object lifetime?
- What happens under contention, wraparound, full, empty, and cancellation states?
- Is the claimed progress class proven for this implementation and target?
- Are atomic width, alignment, and cache effects part of the deployment assumptions?

## Related Topics

- [Parallel And Dataflow Algorithms](index.md)
- [Synchronization Costs And Result Merging](synchronization-costs-and-result-merging.md)
- [Ring Buffers](../data-structures-for-algorithms/ring-buffers.md)
- [Deques](../data-structures-for-algorithms/deques.md)
- [Interrupt-Safe Queues And Buffers](../embedded-linux-algorithmic-constraints/interrupt-safe-queues-and-buffers.md)
- [Algorithm Testing Fuzzing And Reference Models](../algorithm-design-techniques/algorithm-testing-fuzzing-and-reference-models.md)

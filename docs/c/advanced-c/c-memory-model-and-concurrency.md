---
status: draft
reviewed: false
domain: c
difficulty: advanced
last_reviewed: null
---

# C Memory Model And Concurrency

The C memory model defines how threads can access objects and how atomic operations
establish ordering. It is a language contract between the program and the compiler; it
does not describe every interrupt controller, DMA engine, cache, or peripheral. A
correct embedded design therefore has two proofs: a C memory-model proof for C
execution agents, and a platform visibility/ownership proof for hardware agents.

## Learning Objectives

- Distinguish an ordinary data race from an intentional atomic access.
- Explain atomicity, visibility, synchronization, happens-before, and memory order.
- Choose relaxed, acquire/release, or sequentially consistent operations from a
  protocol rather than from folklore.
- Distinguish lock-free and wait-free progress and recognize reclamation hazards.
- Build and review a bounded publication or ring-buffer protocol.
- Understand why volatile, interrupt masking, and compiler barriers do not replace a
  complete synchronization design.

## The C Concurrency Contract

Two evaluations conflict when they access the same memory location, at least one is a
write, and neither is atomic. If conflicting evaluations are not ordered by the C
memory model, the program has a data race and its behavior is undefined. This is
stronger than “the result might be stale”: the optimizer may assume that the race does
not exist and transform surrounding code accordingly.

An object is not made safe by making only one access atomic. All accesses participating
in a shared protocol need a defined relationship. A non-atomic payload can be safe if
it is written before a release operation and read after a matching acquire operation,
and if no other access overlaps that ownership interval. It is not safe to let another
thread modify the payload concurrently merely because a separate flag is atomic.

## Atomic Objects And Lock-Free Properties

Use `<stdatomic.h>` types and operations for C-level atomic objects. Do not cast an
ordinary object to an atomic pointer and assume the representation or alignment is
compatible. Atomic objects can require stronger alignment or can use an internal lock.

The implementation exposes lock-free information through `ATOMIC_*_LOCK_FREE` macros
and `atomic_is_lock_free`. A lock-free operation does not mean that a larger algorithm
is lock-free, and a lock-free implementation does not mean wait-free progress. A
product that cannot tolerate an internal lock must verify the exact type and target.

```c
#include <stdatomic.h>
#include <stdbool.h>

struct status_word {
    atomic_uint flags;
};

static void set_flag(struct status_word *status, unsigned int mask)
{
    (void)atomic_fetch_or_explicit(&status->flags, mask, memory_order_relaxed);
}

static bool has_flag(const struct status_word *status, unsigned int mask)
{
    unsigned int flags = atomic_load_explicit(&status->flags,
                                              memory_order_relaxed);
    return (flags & mask) != 0u;
}
```

Relaxed access is sufficient here only because the flags are independent state and no
other object is being published by them. If setting a flag means that a buffer is ready,
the flag must participate in an ownership protocol with release/acquire ordering.

## Memory Orders

The standard memory orders express increasingly strong constraints:

- **Relaxed:** atomicity and modification-order participation for that atomic object,
  but no ordering of other memory operations.
- **Acquire:** a load or read-modify-write prevents later operations from moving before
  it and can consume a release publication.
- **Release:** a store or read-modify-write prevents earlier operations from moving after
  it and can publish preceding writes.
- **Acquire-release:** combines both directions for a read-modify-write operation.
- **Sequentially consistent:** acquire/release behavior plus a single global order for
  sequentially consistent operations.

Acquire and release are directional. An acquire load that reads a value not published
by the relevant release does not magically order a payload. A release store that no
consumer acquires is only a one-sided intention. Always identify the operation that
transfers ownership and the value that links the two operations.

Sequential consistency is easier to reason about but can be more expensive and does
not fix a wrong algorithm, object lifetime, or missing reset state. Use it as a baseline
while developing a protocol, then weaken orders only with a written argument and tests.

## Happens-Before And Synchronizes-With

Within one thread, sequenced-before orders evaluations. Certain atomic operations form
a synchronizes-with relationship—for example, an acquire load that reads a value
published by a release store. The transitive closure of these relationships produces
happens-before. If a write to a non-atomic payload happens-before a read, the read can
be well-defined; if another write can occur without such ordering, the design is still
wrong.

Draw a small event graph for each protocol:

```text
producer: write payload -> release ready
consumer: acquire ready -> read payload
```

Then add all realistic transitions: queue full, retry, reset, cancellation, timeout,
multiple producers, and object reclamation. A diagram with only the success path is
not a concurrency proof.

## Publication And Consumption

A common one-way publication pattern is:

```c
#include <stdatomic.h>
#include <stdbool.h>

struct publication {
    unsigned int value;
    atomic_bool valid;
};

static void publish(struct publication *publication, unsigned int value)
{
    publication->value = value;
    atomic_store_explicit(&publication->valid, true, memory_order_release);
}

static bool consume(const struct publication *publication, unsigned int *value)
{
    if (!atomic_load_explicit(&publication->valid, memory_order_acquire)) {
        return false;
    }
    *value = publication->value;
    return true;
}
```

This is a one-shot or externally resettable example. A reusable mailbox needs an
acknowledgement or sequence number so the producer does not overwrite the payload while
the consumer still uses it. If the consumer clears `valid`, that transition must also
be atomic and the producer must define what happens when the consumer disappears.

For multi-field messages, publish a single state or sequence value after writing all
fields. Avoid publishing several independent flags unless the consumer has a defined
snapshot algorithm.

## A Bounded SPSC Ring Buffer

A single-producer/single-consumer ring can avoid a lock when each side owns one index.
The following sketch uses monotonically increasing indices and acquire/release only at
the ownership boundary:

```c
#include <stdbool.h>
#include <stddef.h>
#include <stdatomic.h>

enum { QUEUE_CAPACITY = 8u };

struct spsc_queue {
    int values[QUEUE_CAPACITY];
    atomic_size_t head;
    atomic_size_t tail;
};

static bool queue_push(struct spsc_queue *queue, int value)
{
    size_t head = atomic_load_explicit(&queue->head, memory_order_relaxed);
    size_t tail = atomic_load_explicit(&queue->tail, memory_order_acquire);
    if (head - tail == QUEUE_CAPACITY) {
        return false;
    }
    queue->values[head % QUEUE_CAPACITY] = value;
    atomic_store_explicit(&queue->head, head + 1u, memory_order_release);
    return true;
}

static bool queue_pop(struct spsc_queue *queue, int *value)
{
    size_t tail = atomic_load_explicit(&queue->tail, memory_order_relaxed);
    size_t head = atomic_load_explicit(&queue->head, memory_order_acquire);
    if (tail == head || value == NULL) {
        return false;
    }
    *value = queue->values[tail % QUEUE_CAPACITY];
    atomic_store_explicit(&queue->tail, tail + 1u, memory_order_release);
    return true;
}
```

The proof depends on exactly one producer and one consumer, a capacity that fits the
index arithmetic, and no access to a slot after ownership is transferred. For a
long-running system, analyze unsigned counter wraparound; use a sufficiently wide
counter or a proven modular algorithm. For multiple producers or consumers, this code
is not enough—use a designed MPMC algorithm or a lock.

The queue is also a platform problem when shared with DMA or another processor. Add
cache maintenance, explicit layout/endian rules, and a hardware barrier where the
interconnect requires them. C atomics alone do not make a peripheral see the values.

## Fences And Compiler Barriers

`atomic_thread_fence` can participate in a C atomic synchronization pattern without
being attached to the payload object, but it is easy to misuse. Prefer atomic operations
on a state variable because they make the communication edge visible in the code.
`atomic_signal_fence` only constrains compiler reordering with respect to signal
handlers in the C model; it is not a hardware memory barrier.

A compiler-specific barrier such as an empty inline assembly with a memory clobber can
prevent compiler motion, but it does not necessarily emit a CPU fence. A CPU/device
barrier may order hardware accesses while leaving C-level data races unresolved. Name
the layer being synchronized and use the platform primitive that covers it.

## Locks And Progress Guarantees

Classify an operation's progress:

- **Obstruction-free:** a thread completes if it eventually runs alone.
- **Lock-free:** some thread completes after a finite number of steps; an individual
  thread may starve.
- **Wait-free:** every operation completes within a bounded number of its own steps.
- **Blocking:** progress depends on a lock holder, scheduler, or resource becoming
  available.

Embedded systems often prefer a bounded blocking design with a timeout over a complex
lock-free design with unbounded retries. A short critical section can be easier to
verify, especially in an RTOS with priority inheritance. If a lock is used in interrupt
context, verify whether the primitive can be taken there and whether the owner can be
preempted by the interrupt.

## ABA, Reclamation, And Lifetime

Compare-and-swap can be fooled when a location changes from A to B and back to A. A
thread that only compares the value sees no change even though the object may have been
removed and freed in between. Countermeasures include tagged pointers, hazard
pointers, epoch/RCU reclamation, reference counts with a safe acquisition protocol, or
a design that never reuses addresses while an observation is outstanding.

Memory reclamation is usually harder than the atomic update. A lock-free stack with a
correct CAS but an unsafe `free` is still incorrect. Define when a node becomes
unreachable, when readers stop using it, how reclamation is announced, and what memory
allocator/thread-safety guarantees apply.

## False Sharing And Cache Effects

Two independent atomic counters can contend when they share a cache line. Aligning or
padding them may help, but the target's cache-line size, allocator behavior, and
deployment topology must be known. Measure before adding layout constraints, and do
not alter a shared ABI without versioning it.

Per-core counters, batching, read-mostly snapshots, and message passing can reduce
contention more effectively than stronger memory orders. A faster atomic operation is
not useful if the algorithm spends most of its time retrying due to contention.

## Interrupts, Signals, And DMA

An interrupt handler is not automatically a C thread. The implementation and RTOS may
allow only lock-free atomics or a subset of operations. A signal handler has an even
smaller set of safe operations in hosted C/POSIX environments. DMA does not execute C
and does not participate in the C happens-before relation; the driver must define cache
and ownership transitions.

Treat these as separate boundaries:

- task/thread to task/thread: C atomics, mutexes, condition variables, or RTOS APIs;
- thread to ISR: interrupt-safe state and deferred work;
- CPU to DMA: buffer ownership, cache maintenance, descriptors, and device barriers;
- CPU to another processor: shared-memory ABI, interconnect ordering, and reset policy.

## Testing Concurrent Code

Test more than the final value:

- stress with randomized delays, CPU affinity, and scheduler preemption;
- run ThreadSanitizer where the target and code allow it;
- use model checking or a small reference model for bounded protocols;
- inject queue full/empty, cancellation, reset, allocation failure, and wraparound;
- inspect generated atomics for every supported architecture;
- use trace or hardware watchpoints to verify ownership transitions;
- run long enough to exercise counter and generation wraparound.

Avoid tests that accidentally serialize the threads or use a mutex in the test path
while claiming to test a lock-free algorithm. Keep a simple reference implementation
and compare observable behavior under the same operation stream.

## Exercises And Diagnostics

1. Write the happens-before graph for the publication example, then add a reusable
   acknowledgement state and prove that the producer cannot overwrite live data.
2. Stress the SPSC queue with randomized producer/consumer delays and verify every
   value exactly once; test index wraparound with a reduced-width model.
3. Implement a bounded MPMC queue using a mutex, then compare its latency and proof
   burden with a published lock-free algorithm.
4. Create a reclamation test that detects use-after-free under hazard pointers or
   epochs; include a deliberate ABA scenario.
5. Compare relaxed, acquire/release, and sequentially consistent assembly on two
   architectures and explain the observable protocol difference.

## Common Mistakes

- Assuming a volatile flag makes a non-atomic payload safe.
- Using relaxed atomics when the operation is intended to publish data.
- Treating acquire/release as a global ordering guarantee for unrelated atomics.
- Calling an algorithm lock-free because it uses compare-and-swap.
- Freeing an object while another thread can still hold an unprotected pointer.
- Forgetting counter wraparound, queue reset, or generation reuse.
- Using interrupt masking as an SMP lock or a CPU fence as a DMA protocol.
- Testing only on x86 and assuming its ordering or cache behavior proves portability.

## Related Topics

- [Advanced C overview](./index.md)
- [Atomics, Threads, And Signals](../standard-library-and-ecosystem/atomics-threads-and-signals.md)
- [Memory Safety And Lifetime](../semantics-and-memory/memory-safety-and-lifetime.md)
- [Const, Volatile, And Restrict](../semantics-and-memory/qualifiers-const-volatile-restrict.md)
- [Multicore And Heterogeneous Systems](../platform-specific-c/multicore-and-heterogeneous-systems.md)
- [Embedded C And Hardware](../embedded-c-and-hardware/index.md)

## References

- [C11 draft N1570](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n1570.pdf)
- [cppreference: C memory order](https://en.cppreference.com/w/c/atomic/memory_order)
- [Linux memory barriers](https://docs.kernel.org/core-api/wrappers/memory-barriers.html)
- [LLVM atomics and memory model reference](https://llvm.org/docs/LangRef.html#atomic-memory-ordering-constraints)
- [ThreadSanitizer documentation](https://clang.llvm.org/docs/ThreadSanitizer.html)
- The exact C standard edition, compiler atomic implementation, RTOS synchronization
  rules, cache/coherency manual, and DMA programming guide

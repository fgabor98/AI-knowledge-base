---
status: draft
reviewed: false
domain: c
difficulty: advanced
last_reviewed: null
---

# Advanced Data Structures

In embedded C, a data structure is a resource and timing decision as much as a
collection of fields. The right choice depends on maximum element count, allocation
failure, cache locality, interrupt access, ownership, persistence, fragmentation,
reset behavior, and the cost of proving every operation. A linked list that is elegant
on a workstation may be a poor choice in a bounded real-time task; a fixed pool that is
predictable may waste memory when a workload is bursty.

## Learning Objectives

- Select a structure from capacity, locality, ownership, and progress requirements.
- Implement and review intrusive lists, rings, pools, arenas, bitmaps, hash tables,
  heaps, state machines, and zero-copy views.
- State invariants for each operation and test boundary transitions.
- Separate storage ownership from links, indexes, handles, and protocol state.
- Recognize when a lock-free design, custom allocator, or zero-copy path increases
  proof burden more than it improves the product.

## Start With Invariants

Before writing a structure, write down:

- maximum and typical element count;
- whether capacity is fixed, growable, or reclaimable;
- ownership of each element and storage region;
- allowed operations in thread, ISR, DMA, and reset contexts;
- ordering, stability, and lookup requirements;
- memory alignment, cache-line, DMA, and ABI constraints;
- behavior when full, empty, malformed, or out of memory;
- whether handles remain valid after deletion or reset.

Then state an invariant. Examples:

- A ring has `0 <= count <= capacity` and each slot is owned by exactly one side.
- A pool free-list contains every free block exactly once and no allocated block.
- A heap's parent key is no greater than either child for a min-heap.
- A state machine has no transition without a defined event, guard, and error action.
- A zero-copy view is valid only while its backing buffer remains owned and unchanged.

## Choosing A Representation

| Structure | Strength | Cost/risk | Good embedded use |
| --- | --- | --- | --- |
| Array/vector | Locality, simple bounds, predictable iteration | Resize or deletion cost | Samples, tables, fixed registries |
| Intrusive list | No per-node allocation, cheap splicing | Pointer bugs, poor locality | Ready queues, object membership |
| Ring buffer | Bounded FIFO, excellent locality | Capacity/ownership rules | UART, logging, ISR-to-task transfer |
| Bitmap | Compact set/free tracking, fast scans | Limited to indexed universe | Resource allocation, flags |
| Pool/slab | Bounded allocation and stable addresses | Fixed classes, internal waste | Packets, messages, descriptors |
| Arena/region | Very cheap bulk allocation | Individual free unavailable | Parsing, transactions, frame lifetimes |
| Hash table | Expected constant-time lookup | Capacity/load/rehash policy | Registries, caches, routing |
| Heap/priority queue | Ordered extraction | Logarithmic operations, ties | Timers, deadlines, schedulers |
| State machine | Explicit behavior and recovery | More states to document | Drivers, protocols, power modes |

## Intrusive Lists

An intrusive list stores link fields inside the object. It avoids a separate node
allocation and permits one object to be in several lists when it has several link
members. It also couples the object layout to the list implementation and makes a
removed object's links part of its lifetime contract.

Define whether a node is initialized, linked, or detached. Poisoning links after
removal can expose double insertion in debug builds, but do not dereference poison
values in production. A list operation must be atomic with respect to its execution
context; a correct pointer algorithm is not an ISR-safe or SMP-safe algorithm by
itself.

For a doubly linked list, the core invariants are:

- empty means head and tail are both null;
- a non-empty head has no previous node and a non-empty tail has no next node;
- every `next`/`previous` pair agrees;
- the forward and reverse traversals contain the same nodes;
- a node is linked at most once in that list.

Use a sentinel node to remove special cases when the project can tolerate one extra
object. If list order is not required, a singly linked free-list is smaller and faster.

## Ring Buffers

A ring buffer uses a contiguous array and indexes that advance modulo capacity. Decide
whether the capacity is a power of two, whether one slot is reserved to distinguish
full from empty, or whether a separate count/sequence is used. Never let an index
wrap in a way that makes a stale value look current.

For a single producer and consumer, each side can own one index and publish it with
acquire/release ordering. Multiple producers or consumers require a lock, per-slot
sequence numbers, or a carefully reviewed MPMC algorithm. DMA rings add cache and
descriptor ownership; do not apply a thread-only queue recipe to a peripheral.

Document overflow policy: reject newest, drop oldest, overwrite in place, block,
signal backpressure, or reset. A logging buffer may drop data; a command queue usually
must not.

## Bitmaps And Resource Allocation

Bitmaps compactly represent a set of indexed resources. Use a fixed-width word type,
define bit numbering, and guard shifts by the word width. A first-set-bit builtin may
be undefined for zero or may be target-specific; check zero before using it. For a
concurrent bitmap, a read-modify-write needs an atomic bit operation or a lock.

Useful operations include set, clear, test-and-set, find-first-zero, range reservation,
and population count. If resources have different capabilities, maintain separate
bitmaps or a descriptor table rather than encoding an unreviewable flag grammar into
one word.

## Object Pools And Slabs

A fixed object pool gives bounded allocation time and stable addresses. A pool needs:

- storage aligned for the object type;
- a free-state representation or free-list;
- initialization and destruction rules;
- behavior when exhausted;
- double-free and foreign-pointer diagnostics in debug builds;
- concurrency/ISR ownership policy;
- a reset rule for allocated objects.

Slab allocators generalize this into size classes or caches of preinitialized objects.
They can reduce fragmentation and construction cost but need a policy for class choice,
per-core caches, memory coloring, cache locality, and delayed reclamation. Never return a
pool object to the free-list while a DMA descriptor, callback, or external handle can
still reference it.

```c
#include <stdbool.h>
#include <stddef.h>

enum { POOL_CAPACITY = 16u };

struct packet {
    unsigned char data[32];
    size_t length;
};

struct packet_pool {
    struct packet objects[POOL_CAPACITY];
    unsigned char used[POOL_CAPACITY];
};

static struct packet *pool_acquire(struct packet_pool *pool)
{
    for (size_t index = 0u; index < POOL_CAPACITY; ++index) {
        if (pool->used[index] == 0u) {
            pool->used[index] = 1u;
            pool->objects[index].length = 0u;
            return &pool->objects[index];
        }
    }
    return NULL;
}

static bool pool_release(struct packet_pool *pool, struct packet *packet)
{
    for (size_t index = 0u; index < POOL_CAPACITY; ++index) {
        if (&pool->objects[index] == packet && pool->used[index] != 0u) {
            pool->used[index] = 0u;
            return true;
        }
    }
    return false;
}
```

This deliberately simple pool is linear-time and not concurrent. A production pool
can use a free-list or bitmap, but should preserve the same explicit ownership and
exhaustion contract.

## Region And Arena Allocators

An arena allocates monotonically from a region and frees everything at once. It is
excellent for parsing one packet, constructing one configuration snapshot, or building
a transaction. It is not suitable when individual objects live for unrelated lengths
of time.

An arena must check alignment and overflow for every allocation. It must define whether
zero-size allocations return null or a unique aligned pointer, whether allocations are
zeroed, and whether a failed allocation leaves the cursor unchanged. A checkpoint API
can support nested lifetimes:

```text
checkpoint = arena_mark()
temporary = arena_alloc(...)
...
arena_rewind(checkpoint)
```

Never rewind while an object, pointer, DMA descriptor, or callback can still refer to
the region. In safety-critical code, an arena's maximum consumption and reset points
should be visible in the design review.

## Hash Tables

Hash tables need a hash function, collision policy, capacity policy, key lifetime
contract, and deletion strategy. Open addressing has good locality but requires a
load-factor limit and tombstones or backward-shift deletion. Chaining tolerates more
load patterns but adds pointers and allocation/locality costs.

Define whether keys are copied, interned, or borrowed. Hash exactly the bytes and
length specified by the key type; never hash a structure's padding. Avoid unbounded
rehashing in a real-time path. A fixed-capacity table can reject insertion or use a
secondary policy when full. For adversarial inputs, use a keyed hash or a bounded
collision strategy; a fast non-cryptographic hash is not automatically resistant to
chosen inputs.

## Priority Queues And Heaps

A binary heap stores a partial order in an array. For zero-based indexing, parent and
child relationships must be written without overflow-prone arithmetic for the maximum
index. Define tie behavior: stable FIFO among equal priorities, arbitrary order, or an
explicit sequence number. Timer queues need a wraparound-aware time comparison and a
policy for overdue entries.

For hard latency limits, a bucketed timer wheel or fixed-priority bitmap can be more
predictable than a heap. For a small number of elements, a sorted array may be faster
and simpler despite its asymptotic cost. Measure the actual workload and include
allocation, cache, and wake-up cost.

## State Machines

A state machine makes valid transitions explicit. Represent state as an enum or tagged
object, events as a separate enum, and actions as functions with explicit error
returns. Avoid a giant switch with hidden side effects; table-driven transitions can
make coverage and review easier but must not hide guards or ownership changes.

For each transition document:

- current state and accepted event;
- guard conditions and consumed resources;
- outputs and side effects;
- next state;
- timeout, cancellation, and retry behavior;
- action when the event is invalid or duplicated.

Drivers often need a state machine for reset, initialization, active, suspended, fault,
and recovery. A state enum alone does not serialize concurrent events; the event queue
or lock must provide that guarantee.

## Zero-Copy Buffers

Zero-copy means avoiding a data copy, not avoiding all ownership work. A view needs a
pointer/offset, length, and backing-buffer lifetime. The producer must not recycle or
modify the backing storage while a consumer holds the view. If a view crosses a CPU,
DMA, process, or language boundary, add address-space, cache, endian, and alignment
rules.

Use scatter/gather descriptors when data is fragmented, but bound the number of
fragments and validate every segment. A small copy can be cheaper and safer than a
complex zero-copy path when cache lines, lifetime, or cleanup dominate.

## Lock-Free Structures

Lock-free data structures combine atomics, ownership, and reclamation. Before using one,
ask whether a mutex plus a bounded pool meets the latency and scheduling requirement.
If not, specify the algorithm's progress guarantee, memory ordering, ABA defense,
reclamation method, capacity, counter wraparound, and behavior after reset.

Do not derive a lock-free queue by removing a lock from a locked queue. The algorithm's
invariants change, and memory reclamation often becomes the dominant problem. Refer to
[C Memory Model And Concurrency](./c-memory-model-and-concurrency.md) for publication
and ownership proofs.

## Measurement And Failure Testing

For each structure, measure:

- operation latency distribution, not only the mean;
- peak and steady-state memory consumption;
- fragmentation or wasted capacity;
- cache misses and contention where available;
- behavior at empty, full, maximum, wraparound, and allocation failure;
- reset, cancellation, and peer-crash recovery.

Use invariant checks in debug builds, property-based tests for sequences of operations,
and a reference model for queues, heaps, maps, and state machines. Test invalid handles,
double release, stale views, malformed keys, and ownership violations deliberately.

## Exercises And Diagnostics

1. Implement a bounded queue three ways: array FIFO, ring buffer, and linked list. Compare
   memory, locality, full behavior, and worst-case latency.
2. Add invariant assertions to a pool and fuzz acquire/release sequences, including
   foreign pointers and double release.
3. Implement open-addressing lookup with tombstones; measure load-factor and deletion
   behavior under adversarial keys.
4. Build a timer queue using a heap and a bucketed wheel; compare deadline jitter and
   memory use for the target workload.
5. Design a zero-copy packet view, then add a reset/recycle event and prove no stale
   pointer can be used after ownership returns to the pool.

## Common Mistakes

- Choosing by asymptotic complexity while ignoring locality, bounds, and allocator cost.
- Using a linked list in a cache-sensitive path without measuring it.
- Returning pooled storage while a callback, DMA engine, or view still owns it.
- Allowing an arena to rewind while any pointer into it remains live.
- Hashing padding bytes or borrowing keys without a lifetime contract.
- Forgetting full/empty, wraparound, tie, and reset behavior.
- Calling a queue lock-free without proving reclamation and progress.
- Treating zero-copy as a license to skip validation or ownership transfer.

## Related Topics

- [Advanced C overview](./index.md)
- [Data Structures For Algorithms](../../algorithms-and-data-structures/data-structures-for-algorithms/index.md)
- [Memory Layout And Allocation](../semantics-and-memory/memory-layout-and-allocation.md)
- [Memory Safety And Lifetime](../semantics-and-memory/memory-safety-and-lifetime.md)
- [C Memory Model And Concurrency](./c-memory-model-and-concurrency.md)
- [Protocols And Serialization](./protocols-and-serialization.md)

## References

- [C11 draft N1570](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n1570.pdf)
- [Linux kernel linked-list documentation](https://docs.kernel.org/core-api/kernel-api.html)
- [Linux memory allocation documentation](https://docs.kernel.org/core-api/memory-allocation.html)
- [LLVM BumpPtrAllocator reference](https://llvm.org/doxygen/classllvm_1_1BumpPtrAllocator.html)
- The target RTOS/container library, allocator, cache, DMA, and timing documentation

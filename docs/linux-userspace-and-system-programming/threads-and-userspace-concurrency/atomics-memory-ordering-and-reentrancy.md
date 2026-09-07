---
status: draft
reviewed: false
domain: linux-userspace
difficulty: advanced
last_reviewed: null
---

# Atomics, Memory Ordering, And Reentrancy

## What problem does this solve?

An atomic object prevents a data race on that object; it does not automatically make
an adjacent payload, pointer, or lifetime safe. Correct lock-free code needs an
ownership protocol, a happens-before argument, and a reclamation strategy.

## Data races are undefined behavior

Conflicting non-atomic accesses, with at least one write and no ordering, form a data
race. “The CPU usually reads the latest value” is not a C memory-model proof. The
compiler can reorder or eliminate accesses when the program has a race.

Use `<stdatomic.h>` atomic types and operations. Do not cast ordinary storage to an
atomic pointer and assume alignment/representation compatibility.

## Memory orders

| Order | Guarantee |
| --- | --- |
| Relaxed | Atomic access and modification order for that atomic object; no payload ordering |
| Acquire | Later operations cannot move before the load; can consume a release publication |
| Release | Earlier operations cannot move after the store; can publish prior writes |
| Acq-rel | Both directions for a read-modify-write |
| Seq-cst | Acq-rel behavior plus one global order for seq-cst operations |

Classic publication:

```text
producer: write payload -> atomic_store(ready, release)
consumer: atomic_load(ready, acquire) -> read payload
```

This is safe only while the producer does not modify or reclaim the payload and the
consumer does not read before the acquire. Clearing `ready` needs its own ownership
transition. A relaxed flag is enough for independent counters, not for publishing a
buffer.

## Lock-free is not wait-free

An atomic operation may be implemented with a hidden lock. A lock-free algorithm means
some thread makes progress; wait-free means each operation completes in a bounded
number of steps. Neither property solves memory reclamation, ABA, starvation, or
unbounded retries. Check `atomic_is_lock_free` on the target when it matters.

## Reentrancy

A function is reentrant when concurrent or nested calls do not corrupt shared state.
Avoid hidden mutable static buffers, return storage with explicit ownership, protect
shared state, and document callbacks that can re-enter the library. Thread-safe
does not mean signal-safe; signal handlers have a much narrower safe-call set.

`errno` is normally thread-local, but a library’s other global state may not be. Read
the libc attributes and protect higher-level sequences even when individual calls are
MT-Safe.

## Atomics and lifetime

An atomic pointer can publish a pointer safely while the pointed-to object is still
alive, but it cannot prevent another thread from freeing the object. Use a mutex,
reference counting with safe acquisition, epochs/hazards, RCU-like discipline, or a
single-owner queue. Never solve a use-after-free by making the pointer atomic.

## Common mistakes

- Making one flag atomic while accessing the payload non-atomically without ordering.
- Choosing relaxed order because it is faster without a publication proof.
- Calling an atomic pointer protocol lock-free while reclamation is undefined.
- Assuming volatile provides inter-thread synchronization.
- Hiding mutable state in a function-static buffer.
- Calling non-reentrant code from a signal handler or callback.
- Ignoring alignment and target lock-free properties.

## Debugging checklist

- Draw producer/consumer events and identify the synchronizes-with edge.
- Identify who owns and reclaims every published object.
- Check atomic type, alignment, lock-free property, and memory order.
- Run ThreadSanitizer/Helgrind-style tooling in host tests where available.
- Test reset, multiple producers, peer death, cancellation, and reclamation races.
- Separate C memory-model issues from hardware/DMA visibility issues.

## Related topics

- [Stage 6: Threads And Userspace Concurrency](index.md)
- [Mutexes, Condition Variables, And Semaphores](mutexes-condition-variables-and-semaphores.md)
- [Shared Memory And Zero-Copy IPC](../ipc-and-event-driven-design/shared-memory-and-zero-copy-ipc.md)
- [C Memory Model And Concurrency](../../c/advanced-c/c-memory-model-and-concurrency.md)

## References

- [`stdatomic.h(7)`](https://man7.org/linux/man-pages/man7/stdatomic.h.7.html)
- [`atomic_is_lock_free(3)`](https://en.cppreference.com/w/c/atomic/atomic_is_lock_free)
- [C11 atomics overview](https://en.cppreference.com/w/c/atomic)
- [`signal-safety(7)`](https://man7.org/linux/man-pages/man7/signal-safety.7.html)

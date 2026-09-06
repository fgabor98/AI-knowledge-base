---
status: draft
reviewed: false
domain: linux-userspace
difficulty: intermediate
last_reviewed: null
---

# Stage 6: Threads And Userspace Concurrency

Threads share one process address space, which makes communication cheap and
lifetime mistakes easy. This stage develops a disciplined model for thread creation,
synchronization, ownership, cancellation, bounded work, and scheduling. The C memory
model and POSIX APIs must agree: an atomic flag cannot repair an object lifetime bug,
and a mutex cannot make an unbounded queue safe for a constrained product.

## The concurrency model

```text
process address space
  +-- thread A: stack, registers, signal mask, TLS
  +-- thread B: stack, registers, signal mask, TLS
  +-- shared heap/globals, FDs, mappings, credentials
```

Every shared object needs an ownership or synchronization rule. “It is only one
integer” is not a rule; “only the producer writes it and the consumer acquires the
release publication” is a rule.

## Learning materials

1. [pthread Lifecycle And Thread Attributes](pthread-lifecycle-and-thread-attributes.md)
2. [Mutexes, Condition Variables, And Semaphores](mutexes-condition-variables-and-semaphores.md)
3. [Atomics, Memory Ordering, And Reentrancy](atomics-memory-ordering-and-reentrancy.md)
4. [Worker Pools, Bounded Queues, And Backpressure](worker-pools-bounded-queues-and-backpressure.md)
5. [Cancellation, Priority, And Real-Time Scheduling](cancellation-priority-and-realtime-scheduling.md)

## Contract questions

| Question | Required answer |
| --- | --- |
| Ownership | Which thread may mutate or destroy the object? |
| Lifetime | Which joins, references, or barriers keep it alive? |
| Synchronization | Which mutex, atomic, condition predicate, or queue orders access? |
| Progress | Can a thread block, starve, deadlock, or spin? |
| Shutdown | How do waiters wake, stop, drain, and join? |
| Cancellation | Where are cancellation points and cleanup handlers? |
| Scheduling | What latency/priority/affinity assumption is required? |
| Observability | How are thread IDs, queue depth, waits, and stalls diagnosed? |

## Stage lab

```sh
cc -std=c11 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -pthread -g \
    examples/c/linux-userspace-worker-pool.c -o /tmp/worker-pool
/tmp/worker-pool
```

Run it repeatedly and interrupt it during work. Verify that the queue closes, workers
wake, each worker joins, and no task remains owned by a destroyed queue.

## Completion criteria

You can complete this stage when you can:

- create, name, join, detach, and shut down threads with explicit ownership;
- use mutexes and condition variables with predicate loops and lock ordering;
- distinguish mutex synchronization from atomics and memory-order publication;
- design a bounded worker queue with backpressure and shutdown markers;
- handle cancellation without leaking locks, FDs, memory, or partially updated state;
- explain priority inversion, affinity, and real-time policy limits;
- diagnose deadlock, starvation, race, lost wakeup, and thread-lifetime failures.

## Related topics

- [Stage 5: Time, Clocks, And Signals](../time-clocks-and-signals/index.md)
- [Stage 7: IPC And Event-Driven Design](../ipc-and-event-driven-design/index.md)
- [C Memory Model And Concurrency](../../c/advanced-c/c-memory-model-and-concurrency.md)
- [Memory Pressure, OOM, And Real-Time Constraints](../process-memory-and-mapping/memory-pressure-oom-and-realtime.md)

## References

- [`pthreads(7)`](https://man7.org/linux/man-pages/man7/pthreads.7.html)
- [POSIX Threads](https://pubs.opengroup.org/onlinepubs/9799919799/basedefs/pthread.h.html)
- [`pthread_mutex_lock(3)`](https://man7.org/linux/man-pages/man3/pthread_mutex_lock.3p.html)

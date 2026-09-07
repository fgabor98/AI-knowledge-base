---
status: draft
reviewed: false
domain: linux-userspace
difficulty: intermediate
last_reviewed: null
---

# Mutexes, Condition Variables, And Semaphores

## What problem does this solve?

Threads need both mutual exclusion and a way to wait for state changes. A mutex
protects an invariant; a condition variable waits for a predicate; a semaphore counts
available units. Choosing one without defining the invariant leads to lost wakeups,
deadlocks, or code that wakes without knowing what changed.

## Mutex ownership

```text
lock mutex
  inspect predicate and shared state
  modify state while invariant is protected
unlock mutex
```

A mutex is not a memory buffer or a general event. Keep critical sections short, do
not call unknown blocking operations while holding a global lock, and define lock
ordering for nested acquisition. Recursive mutexes can hide a design error; robust
mutexes add owner-death recovery obligations rather than making data consistent.

## Condition-variable predicate loop

```c
pthread_mutex_lock(&queue->mutex);
while (!queue->stopping && queue->count == 0U) {
    pthread_cond_wait(&queue->not_empty, &queue->mutex);
}
if (queue->count != 0U) {
    item = queue_pop(queue);
}
pthread_mutex_unlock(&queue->mutex);
```

The condition variable has no memory of a notification. The predicate is the state;
the mutex makes checking and waiting atomic with respect to the producer. Always use
a `while` loop for spurious wakeups, notifications for another state transition, and
multiple consumers racing for one item. Signal after changing the predicate while
holding the mutex, then release it promptly.

Timed waits use an explicitly selected clock and an absolute deadline where supported.
After timeout, recheck the predicate and distinguish “still false” from a state change
that happened just before the timeout.

## Semaphores

A semaphore counts permits. It is useful for bounded resource slots or simple
producer/consumer counts, but it does not protect a multi-field invariant by itself.
Pair it with a mutex or atomics when queue data and count must change consistently.
Define cancellation and shutdown behavior for a thread blocked in `sem_wait`.

## Lock ordering and deadlocks

For locks A and B, choose one order and obey it everywhere:

```text
lock A -> lock B -> change both -> unlock B -> unlock A
```

Do not call back into code that can acquire a lock in the opposite order. Lock
contention can become priority inversion when a low-priority owner blocks a high-
priority waiter. Keep ownership graphs and lock order in design documentation.

## Process-shared synchronization

Pthread mutexes and condition variables are process-private by default. A process-
shared attribute requires shared backing, correct initialization exactly once, and a
peer-death/recovery protocol. Never place a process-private pointer in a shared queue.

## Robust owner-death recovery

For a robust mutex, `pthread_mutex_lock` returning `EOWNERDEAD` means the
caller **acquired** the lock but the protected state may be inconsistent.
Repair the invariant before calling `pthread_mutex_consistent`, then unlock.
Unlocking without marking it consistent makes subsequent acquisition report
`ENOTRECOVERABLE`. These are direct error values, not `errno`.

Suppose a shared queue owner died after advancing its tail but before publishing
the item length. Recovery needs enough metadata to identify complete items and
discard the partial update. Merely calling `pthread_mutex_consistent` declares
repair; it does not perform repair. If reconstruction cannot be proven, retire
the entire shared generation through a coordinated protocol.

See [POSIX mutex acquisition](https://man7.org/linux/man-pages/man3/pthread_mutex_lock.3p.html).
Test owner death at each mutation boundary in a separate process. Destroying and
reinitializing a lock while old waiters exist is not a recovery protocol.

## Common mistakes

- Using `if` instead of `while` around `pthread_cond_wait`.
- Signaling without changing the predicate.
- Checking a predicate outside the mutex and then waiting.
- Holding a mutex across I/O, callbacks, allocation, or unbounded work.
- Acquiring locks in inconsistent order.
- Treating a semaphore as protection for a complex data structure.
- Destroying a synchronization object while waiters may still reference it.

## Debugging checklist

- Write the invariant protected by each mutex.
- Record owner, acquisition order, wait predicate, and wakeup source.
- Capture thread stacks when deadlocked and inspect mutex wait relationships.
- Test spurious wakeups, timeout races, shutdown, cancellation, and worker failure.
- Check process-shared attributes and initialization/lifetime.
- Measure contention and critical-section duration under target load.

## Related topics

- [Stage 6: Threads And Userspace Concurrency](index.md)
- [pthread Lifecycle And Thread Attributes](pthread-lifecycle-and-thread-attributes.md)
- [Atomics, Memory Ordering, And Reentrancy](atomics-memory-ordering-and-reentrancy.md)
- [Worker Pools, Bounded Queues, And Backpressure](worker-pools-bounded-queues-and-backpressure.md)

## References

- [`pthread_mutex(3)`](https://man7.org/linux/man-pages/man3/pthread_mutex_lock.3p.html)
- [`pthread_cond_wait(3)`](https://man7.org/linux/man-pages/man3/pthread_cond_wait.3.html)
- [`sem_wait(3)`](https://man7.org/linux/man-pages/man3/sem_wait.3.html)
- [POSIX condition variables](https://pubs.opengroup.org/onlinepubs/9799919799/functions/pthread_cond_wait.html)

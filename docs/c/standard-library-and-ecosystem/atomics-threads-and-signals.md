---
status: draft
reviewed: false
domain: c
difficulty: intermediate
last_reviewed: null
---

# Atomics, Threads, And Signals

C11 added a language and library model for atomic objects and threads, but implementation support varies, especially in freestanding and embedded environments. RTOSes and operating systems often provide stronger scheduling and synchronization APIs than threads.h, while interrupt handlers and signals have their own restrictions.

Do not choose a facility only because it exists in a header. Match the mechanism to the execution contexts, memory-ordering proof, latency, and target runtime.

## Learning Objectives

- Use atomic objects and operations with deliberate memory orders.
- Distinguish atomicity, visibility, ordering, and lock-freedom.
- Understand the scope and limitations of threads.h.
- Use mutexes and condition variables without lifetime or shutdown races.
- Handle signals and setjmp/longjmp within their narrow contracts.
- Choose between C atomics, RTOS primitives, interrupt masking, and platform APIs.

## Atomic Objects

An atomic object is accessed through atomic operations defined by stdatomic.h:

~~~c
#include <stddef.h>
#include <stdatomic.h>

static _Atomic unsigned int ready;
static unsigned int payload;

void publish(unsigned int value)
{
    payload = value;
    atomic_store_explicit(&ready, 1u, memory_order_release);
}

int consume(unsigned int *value)
{
    if (value == NULL) {
        return -1;
    }

    if (atomic_load_explicit(&ready, memory_order_acquire) == 0u) {
        return 1;
    }

    *value = payload;
    return 0;
}
~~~

The release store and acquire load establish a happens-before relationship for payload when the load observes the published value. The payload itself need not be atomic only because access is ordered by the atomic protocol; every access must obey the protocol.

Do not read or write an atomic object through an incompatible non-atomic access path. Use atomic_init for objects that require explicit initialization, especially dynamically allocated or embedded atomic objects.

## Memory Orders

The common memory orders are:

| Order | Purpose |
| --- | --- |
| Relaxed | Atomicity and modification order only; no cross-object ordering |
| Acquire | Prevent later operations from moving before a successful load |
| Release | Prevent earlier operations from moving after a store |
| Acq_rel | Acquire and release for a read-modify-write operation |
| Sequentially consistent | Strong single-order model, often easiest to reason about |
| Consume | Specialized dependency ordering; commonly treated conservatively as acquire |

Start with sequential consistency when proving a design. Weaken memory orders only when the synchronization graph is understood and measurement or architecture requires it.

An atomic counter does not publish associated data automatically:

~~~c
#include <stdatomic.h>

static _Atomic unsigned int count;

void record_event(void)
{
    atomic_fetch_add_explicit(&count, 1u, memory_order_relaxed);
}
~~~

This counts events safely, but another thread cannot infer that unrelated data is visible from the counter unless the protocol provides the required ordering.

## Lock-Free Properties

The implementation may use hardware instructions or hidden locks:

~~~c
#include <stdatomic.h>

_Static_assert(ATOMIC_INT_LOCK_FREE >= 0,
               "implementation must define atomic lock-free property");

int atomic_counter_is_lock_free(void)
{
    static _Atomic unsigned int counter;
    return atomic_is_lock_free(&counter);
}
~~~

ATOMIC_*_LOCK_FREE macros describe implementation properties. An operation that is not lock-free may call a runtime helper or take a library lock, making it inappropriate for an ISR or hard real-time path.

Check the target compiler, ABI, and library. Do not infer lock-freedom from the width of the type.

## C Threads

threads.h provides optional C11 thread interfaces on implementations that support them:

~~~c
#include <threads.h>

int worker(void *context)
{
    (void)context;
    return 0;
}

int start_worker(void)
{
    thrd_t thread;
    if (thrd_create(&thread, worker, NULL) != thrd_success) {
        return -1;
    }

    return thrd_join(thread, NULL) == thrd_success ? 0 : -2;
}
~~~

Thread creation has a lifetime contract for the context, stack, scheduler, and shutdown. The C threads API does not describe all target scheduling, priority, affinity, interrupt, or real-time behavior.

On a microcontroller, an RTOS task API may be the supported mechanism. Do not mix C threads and RTOS threads without understanding TLS, libc locks, startup, and scheduler integration.

## Mutexes And Condition Variables

C mutexes protect invariants, not individual lines:

~~~c
#include <threads.h>

struct queue_state {
    mtx_t lock;
    cnd_t changed;
    unsigned int count;
};

int queue_wait(struct queue_state *queue)
{
    if (mtx_lock(&queue->lock) != thrd_success) {
        return -1;
    }

    while (queue->count == 0u) {
        if (cnd_wait(&queue->changed, &queue->lock) != thrd_success) {
            mtx_unlock(&queue->lock);
            return -2;
        }
    }

    --queue->count;
    mtx_unlock(&queue->lock);
    return 0;
}
~~~

Always wait in a loop because wakeups can occur without the predicate becoming true. Define destruction order: stop producers, wake waiters, join threads, then destroy condition variables and mutexes.

A mutex can be correct for tasks and still illegal in an ISR. A condition variable is generally not a real-time interrupt primitive.

## Signals

ISO signal handlers have a narrow portable contract. A handler should usually set a volatile sig_atomic_t flag and return:

~~~c
#include <signal.h>

static volatile sig_atomic_t stop_requested;

static void handle_signal(int signal_number)
{
    (void)signal_number;
    stop_requested = 1;
}
~~~

The main execution context can observe the flag and perform cleanup. Do not call printf, malloc, most string functions, mutex operations, or arbitrary project APIs from a signal handler unless the platform explicitly guarantees safety.

POSIX defines an async-signal-safe function set, but that is a POSIX contract, not a general ISO C rule. Hardware interrupts and RTOS ISRs are different mechanisms and require their platform rules.

## setjmp And longjmp

Non-local jumps can bypass ordinary cleanup:

~~~c
#include <setjmp.h>

static jmp_buf recovery;

int protected_operation(void)
{
    if (setjmp(recovery) != 0) {
        return -1;
    }

    risky_operation();
    return 0;
}

void fault_recovery(void)
{
    longjmp(recovery, 1);
}
~~~

This pattern is safe only with a tightly controlled lifetime and execution context. Do not longjmp into a function that has returned. Automatic variables changed after setjmp may have indeterminate values unless declared volatile according to the standard rules. Locks, allocations, hardware transactions, and callbacks are not automatically unwound.

Prefer explicit status propagation for ordinary error handling. Reserve non-local jumps for a documented recovery boundary or parser context with no skipped resources.

## C Atomics Versus RTOS Primitives

| Requirement | Suitable mechanism |
| --- | --- |
| Atomic counter or flag | C atomic |
| Publish data between tasks | C atomic release/acquire or RTOS queue |
| Queue ownership and blocking | RTOS queue or message API |
| Priority inheritance | RTOS mutex |
| ISR-to-task notification | RTOS ISR-safe primitive |
| Disable interrupts for a tiny critical section | Target/RTOS primitive |
| Device ordering and cache visibility | Platform barrier and DMA API |
| Cross-process synchronization | OS/POSIX mechanism |

The mechanism must match the scope. C atomics do not replace a scheduler, priority protocol, interrupt barrier, or cache maintenance operation.

## Exercises

1. Implement a release/acquire publication and test that the payload is never observed before publication.
2. Compare relaxed, acquire/release, and sequentially consistent counter designs.
3. Query lock-free properties on host and target.
4. Implement a mutex-protected queue with a predicate loop and explicit shutdown.
5. Replace a signal handler’s unsafe logging call with a flag and main-context reporting.
6. Identify every resource skipped by a sample longjmp path and rewrite it with explicit cleanup.
7. Map one RTOS synchronization requirement to C atomics, a queue, a mutex, or an ISR primitive.

## Common Mistakes

- Treating volatile as atomic synchronization.
- Using relaxed atomics when a publication relationship is required.
- Assuming atomic operations are lock-free.
- Waiting on a condition variable without a predicate loop.
- Destroying synchronization objects while threads or waiters still use them.
- Calling non-signal-safe functions from a signal handler.
- Using C threads in a freestanding target without runtime support.
- Calling mutexes, malloc, or C library functions from an ISR.
- Assuming longjmp releases locks or allocations.
- Ignoring cache and device-ordering requirements.

## Debugging Checklist

1. Draw the happens-before or ownership graph.
2. Identify the atomic object and associated non-atomic data.
3. Verify memory order against the proof, not intuition.
4. Check lock-free properties and hidden runtime helpers.
5. Trace thread, task, ISR, signal, and callback lifetimes.
6. Test shutdown, cancellation, wakeups, and missed events.
7. Run ThreadSanitizer on host-representable code.
8. Compare C synchronization with RTOS and hardware barrier requirements.

## Related Topics

- [Standard Library And Ecosystem overview](./index.md)
- [Semantics And Memory](../semantics-and-memory/index.md)
- [Const, Volatile, And Restrict](../semantics-and-memory/qualifiers-const-volatile-restrict.md)
- [C Memory Model And Concurrency](../advanced-c/c-memory-model-and-concurrency.md)
- [Embedded C And Hardware](../embedded-c-and-hardware/index.md)
- [C Programming](../index.md)

## References

- [ISO/IEC 9899 standards and drafts](https://open-std.org/jtc1/sc22/wg14/www/standards.html)
- [C11 public draft N1570](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n1570.pdf)
- [POSIX signal concepts](https://pubs.opengroup.org/onlinepubs/9799919799/basedefs/signal.h.html)
- [Clang ThreadSanitizer documentation](https://clang.llvm.org/docs/ThreadSanitizer.html)

---
status: draft
reviewed: false
domain: linux-userspace
difficulty: intermediate
last_reviewed: null
---

# pthread Lifecycle And Thread Attributes

## What problem does this solve?

A thread has a creation owner, a start argument, a termination result, a stack, and a
join/detach lifecycle. Losing any of those obligations creates leaks, use-after-free,
shutdown hangs, or a worker that continues using destroyed state.

## Create, run, join

```c
static void *worker_main(void *argument)
{
    struct worker_context *context = argument;
    /* establish ownership rules before accessing context */
    return result;
}

pthread_t thread;
int error = pthread_create(&thread, NULL, worker_main, context);
if (error != 0) {
    /* pthread functions return an error number directly, not through errno */
}
error = pthread_join(thread, &result);
```

The creator owns the `pthread_t` lifecycle until it joins or deliberately detaches.
Joining waits for termination and releases joinable-thread resources. A detached
thread releases those resources automatically but cannot return a result to a joiner;
detachment is not a substitute for shutdown coordination.

## Argument and result lifetime

The argument must remain valid until the start routine no longer uses it. Do not pass
the address of a loop variable or stack object that ends before the thread reads it.
For results, return a stable allocation with an explicit owner, store into a shared
result protected by synchronization, or communicate through a queue.

```text
create context -> create thread -> thread signals ready
main requests stop -> thread leaves loop -> thread releases borrowed resources
main joins -> main destroys context
```

Destroying context before `pthread_join` is a lifetime error even if the thread
“usually finishes quickly.”

## Attributes

Thread attributes can set detach state, stack address/size, guard size, scheduling
inheritance/policy, priority, and scope where supported. Initialize and destroy the
`pthread_attr_t` object itself, and check every return value.

Use explicit stack sizes for many workers or deep call paths. A stack that is too small
causes faults; a stack that is too large wastes virtual address space and committed
resources. Guard pages help detect overflow but do not make recursion bounded.

## Thread-local state

Thread-local storage gives each thread an instance of a variable. It is useful for
scratch buffers and per-thread diagnostics, but it does not make referenced objects
thread-safe. Destructors and cleanup order need a documented lifetime, especially when
TLS points into a subsystem being torn down.

## Naming and identification

Give worker threads stable names for diagnostics, within platform length limits. Log
the kernel TID when correlating with `/proc/<pid>/task`, scheduler traces, or `strace`.
Do not use a TID as a permanent ownership handle after thread termination.

## Error and shutdown paths

If creating worker 4 fails after workers 1–3 exist, stop and join the already-created
workers before freeing shared state. If a thread is detached, provide another way to
know that all uses of shared state have ended. A process exit terminates all threads,
but it is not a graceful resource protocol.

## Common mistakes

- Passing a short-lived stack address as thread context.
- Forgetting to join or detach every successfully created thread.
- Detaching a thread while another component still needs its result.
- Destroying mutexes, queues, or context before workers have stopped.
- Ignoring direct pthread error returns and checking stale `errno`.
- Choosing a tiny stack because the worker’s initial function is small.
- Assuming TLS protects pointed-to shared data.

## Debugging checklist

- Record thread name, TID, creator, context owner, stack/guard attributes, and state.
- Check join/detach ownership on success, partial creation, cancellation, and crash.
- Inspect `/proc/<pid>/task`, stack limits, and scheduler state.
- Test context lifetime with delayed worker start and repeated shutdown.
- Verify all pthread return values and cleanup ordering.
- Use thread sanitizers in host tests where supported.

## Related topics

- [Stage 6: Threads And Userspace Concurrency](index.md)
- [Mutexes, Condition Variables, And Semaphores](mutexes-condition-variables-and-semaphores.md)
- [Cancellation, Priority, And Real-Time Scheduling](cancellation-priority-and-realtime-scheduling.md)
- [Process Model And Identifiers](../processes-and-program-lifetime/process-model-and-identifiers.md)

## References

- [`pthread_create(3)`](https://man7.org/linux/man-pages/man3/pthread_create.3.html)
- [`pthread_join(3)`](https://man7.org/linux/man-pages/man3/pthread_join.3.html)
- [`pthread_attr_setstacksize(3)`](https://man7.org/linux/man-pages/man3/pthread_attr_setstacksize.3.html)
- [`pthread_setname_np(3)`](https://man7.org/linux/man-pages/man3/pthread_setname_np.3.html)

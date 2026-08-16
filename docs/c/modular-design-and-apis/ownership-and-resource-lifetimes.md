---
status: draft
reviewed: false
domain: c
difficulty: intermediate
last_reviewed: null
---

# Ownership And Resource Lifetimes

Ownership answers who is responsible for a resource, who may use it, who may transfer it, and who must release or quiesce it. In C, these rules are usually expressed through API design, naming, documentation, and review rather than enforced by the type system.

Resources include memory, file descriptors, mutexes, interrupt registrations, DMA mappings, clocks, power references, device handles, and callback registrations.

## Learning Objectives

- Classify owned, borrowed, shared, and transferred resources.
- Design acquisition and cleanup paths.
- Handle partial initialization and failure rollback.
- Use forward cleanup patterns correctly.
- Model asynchronous resource lifetimes.
- Apply ownership rules to host, RTOS, and hardware resources.

## Ownership Vocabulary

| Term | Meaning |
| --- | --- |
| Owner | Responsible for validity, release, and lifecycle |
| Borrower | May use temporarily but must not release |
| Transfer | Ownership moves at a defined API boundary |
| Shared | Multiple users coordinate access and final release |
| View | A pointer and bounds into storage owned elsewhere |
| Lease | Temporary use allowed until an explicit expiry or release |
| Quiesce | Stop new users and wait for in-flight users to finish |

Use names that carry the contract: create/destroy, acquire/release, lock/unlock, map/unmap, submit/complete, retain/release.

## Acquisition And Rollback

Acquire resources in a known order and release them in reverse order:

~~~c
#include <stdbool.h>
#include <stddef.h>

struct component {
    bool clock_enabled;
    bool irq_registered;
    bool worker_started;
};

int component_start(struct component *component)
{
    if (component == NULL) {
        return -1;
    }

    if (enable_clock() != 0) {
        return -2;
    }
    component->clock_enabled = true;

    if (register_interrupt() != 0) {
        goto rollback;
    }
    component->irq_registered = true;

    if (start_worker() != 0) {
        goto rollback;
    }
    component->worker_started = true;
    return 0;

rollback:
    component_stop(component);
    return -3;
}
~~~

The rollback function must tolerate partial state. Make cleanup idempotent so both normal shutdown and failed startup can call it safely.

## Forward Cleanup

A single cleanup block makes error paths auditable:

~~~c
int open_device(struct device **result)
{
    int status = -1;
    struct device *device = NULL;

    if (result == NULL) {
        return -2;
    }
    *result = NULL;

    device = device_alloc();
    if (device == NULL) {
        goto out;
    }

    if (device_configure(device) != 0) {
        goto out;
    }

    if (device_start(device) != 0) {
        goto out;
    }

    *result = device;
    device = NULL;
    status = 0;

out:
    if (device != NULL) {
        device_destroy(device);
    }
    return status;
}
~~~

The local owner retains responsibility until publication succeeds. After the result is published, the local pointer is cleared so cleanup does not release the transferred resource.

## Partial Initialization

Prefer explicit state flags or a state enum over guessing from pointer values:

~~~c
enum device_phase {
    DEVICE_NEW,
    DEVICE_CONFIGURED,
    DEVICE_RUNNING,
    DEVICE_STOPPED
};

struct device {
    enum device_phase phase;
    void *context;
};
~~~

A non-null context does not prove that clocks, interrupts, queues, and hardware are initialized. Cleanup should inspect each resource or a precisely defined phase.

## Memory Ownership

For an allocated object:

~~~c
#include <stdlib.h>

int buffer_create(unsigned char **result, size_t length)
{
    unsigned char *buffer;

    if (result == NULL) {
        return -1;
    }

    buffer = malloc(length);
    if (buffer == NULL) {
        *result = NULL;
        return -2;
    }

    *result = buffer;
    return 0;
}

void buffer_destroy(unsigned char **buffer)
{
    if (buffer != NULL) {
        free(*buffer);
        *buffer = NULL;
    }
}
~~~

The API makes creation, destruction, and nulling visible. It still needs a length contract and a rule for who may borrow the buffer.

Do not use free for memory from a static array, pool, arena, linker section, or DMA-specific allocator.

## File Descriptors And Locks

Hosted and embedded Linux modules often own file descriptors:

~~~c
#include <unistd.h>

int close_fd(int *fd)
{
    int result;

    if (fd == NULL || *fd < 0) {
        return 0;
    }

    result = close(*fd);
    *fd = -1;
    return result;
}
~~~

For locks, the owner must release on every path, including early returns and error handling. A lock is also a context resource: never hold it across a blocking operation unless the design permits priority inversion and reentrancy consequences.

## DMA And Hardware Handles

A DMA mapping or hardware handle has a multi-party lifetime:

1. Software allocates and prepares storage.
2. Software submits it to hardware.
3. Hardware owns the buffer while active.
4. Completion transfers ownership back.
5. Software validates status and releases or reuses it.

Do not free, overwrite, unmap, or power down a device while hardware may still access its descriptors or buffers. Shutdown must first prevent new submissions, then wait for in-flight work, then release mappings and clocks.

## Reference Counting

Reference counting can manage shared lifetimes:

~~~c
#include <stddef.h>

struct shared_buffer {
    unsigned int references;
    unsigned char *data;
};

void buffer_retain(struct shared_buffer *buffer)
{
    if (buffer != NULL) {
        ++buffer->references;
    }
}

void buffer_release(struct shared_buffer **buffer)
{
    if (buffer != NULL && *buffer != NULL) {
        if (--(*buffer)->references == 0u) {
            free((*buffer)->data);
            free(*buffer);
        }
        *buffer = NULL;
    }
}
~~~

This sketch needs overflow protection, synchronization, a defined initial reference, and a matching allocator. Reference counting does not handle cycles and does not replace quiescing asynchronous users.

## Shutdown Order

A robust shutdown usually follows this order:

1. Reject new work.
2. Signal or stop producers.
3. Disable or detach interrupts.
4. Stop workers and wait for callbacks.
5. Wait for DMA or device ownership to return.
6. Flush or cancel queued work.
7. Release locks, mappings, clocks, and memory.
8. Invalidate handles and publish the stopped state.

The exact order is platform-specific, but releasing storage before all users stop is always a lifetime defect.

## Exercises

1. Draw an acquisition and rollback graph for a driver with clock, IRQ, DMA, queue, and worker resources.
2. Make cleanup idempotent and test it after every startup failure.
3. Implement an ownership-transfer API that never publishes a partially initialized object.
4. Model DMA ownership with explicit states and reject illegal transitions.
5. Review a reference-counted object for overflow, cycles, and concurrent retain/release.
6. Write a shutdown test with callbacks and interrupts arriving at each phase.

## Common Mistakes

- Releasing resources in acquisition order rather than reverse order.
- Assuming a non-null pointer means complete initialization.
- Publishing an object before all required fields and dependencies are ready.
- Calling the wrong allocator’s release function.
- Freeing or reusing DMA storage before completion.
- Holding locks across blocking or callback operations.
- Treating reference counting as synchronization or cycle collection.
- Forgetting to invalidate handles after destruction.
- Allowing new work during shutdown.
- Making cleanup depend on the exact failure path.

## Debugging Checklist

1. List every resource and its owner in a table.
2. Log acquisition, transfer, quiescence, and release events.
3. Inject failure after each acquisition step.
4. Assert legal lifecycle states at public entry points.
5. Check allocator and release pairing.
6. Trace callbacks, IRQs, tasks, and DMA before freeing shared state.
7. Add generation counters or poison patterns for stale handles.
8. Test repeated shutdown and partial-start cleanup.

## Related Topics

- [Modular Design And APIs overview](./index.md)
- [APIs And Opaque Types](./api-and-opaque-types.md)
- [Error Handling](./error-handling.md)
- [Semantics And Memory](../semantics-and-memory/index.md)
- [Embedded C And Hardware](../embedded-c-and-hardware/index.md)
- [C Programming](../index.md)

## References

- [ISO/IEC 9899 standards and drafts](https://open-std.org/jtc1/sc22/wg14/www/standards.html)
- [C11 public draft N1570](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n1570.pdf)
- [POSIX close specification](https://pubs.opengroup.org/onlinepubs/9799919799/functions/close.html)
- [CERT C memory-management rules](https://wiki.sei.cmu.edu/confluence/display/c)

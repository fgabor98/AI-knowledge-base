---
status: draft
reviewed: false
domain: c
difficulty: intermediate
last_reviewed: null
---

# Callbacks And Function Tables

Callbacks and function tables let a module call policy supplied by another module. They are the C equivalent of many interface, strategy, driver, and event-dispatch patterns. Their safety depends on function-pointer compatibility, context lifetime, invocation context, reentrancy, and shutdown ordering.

## Learning Objectives

- Design callbacks with explicit context pointers.
- Build function tables for ports and driver operations.
- Document invocation, blocking, and reentrancy rules.
- Keep callback context alive through asynchronous use.
- Use callbacks and tables to create testable dependencies.
- Prevent stale function pointers and teardown races.

## Callback Contracts

A callback type should include all required context:

~~~c
#include <stddef.h>
#include <stdint.h>

typedef void (*event_callback)(void *context, uint32_t event);

struct event_sink {
    event_callback callback;
    void *context;
};

static void emit_event(const struct event_sink *sink, uint32_t event)
{
    if (sink != NULL && sink->callback != NULL) {
        sink->callback(sink->context, event);
    }
}
~~~

The contract should state which thread, task, interrupt, or worker invokes the callback; whether it may block; whether it may call back into the source; and when registration becomes active.

Never cast an incompatible function pointer to fit a callback type. Add an adapter with the correct signature instead.

## Context Lifetime

The context pointer usually points to an object owned by the registering caller:

~~~c
struct application {
    unsigned int event_count;
};

static void on_event(void *context, uint32_t event)
{
    struct application *application = context;
    if (application != NULL && event != 0u) {
        ++application->event_count;
    }
}
~~~

The application object must remain alive until all callbacks have stopped. If callbacks can be queued, deregistration must wait for queued invocations or use a generation and cancellation protocol.

Do not store a pointer to a stack-local context in a long-lived driver.

## Registration And Teardown

A registration API needs a clear replacement rule:

~~~c
enum callback_result {
    CALLBACK_OK,
    CALLBACK_BUSY,
    CALLBACK_INVALID
};

struct callback_slot {
    event_callback callback;
    void *context;
    unsigned int generation;
};

enum callback_result callback_register(struct callback_slot *slot,
                                       event_callback callback,
                                       void *context)
{
    if (slot == NULL || callback == NULL) {
        return CALLBACK_INVALID;
    }
    if (slot->callback != NULL) {
        return CALLBACK_BUSY;
    }

    slot->callback = callback;
    slot->context = context;
    ++slot->generation;
    return CALLBACK_OK;
}

void callback_unregister(struct callback_slot *slot)
{
    if (slot != NULL) {
        slot->callback = NULL;
        slot->context = NULL;
        ++slot->generation;
    }
}
~~~

This simple version is safe only when registration and invocation are serialized. An asynchronous implementation must prevent a callback from reading context after unregistration and must define whether unregister waits for in-flight calls.

Clear the callback before releasing its context, but also synchronize with a callback that already copied the function and context values.

## Function Tables

A function table groups a port’s operations:

~~~c
#include <stddef.h>
#include <stdint.h>

struct clock_ops {
    uint32_t (*now_ticks)(void *context);
    void (*sleep_ticks)(void *context, uint32_t ticks);
};

struct clock_port {
    const struct clock_ops *ops;
    void *context;
};

static uint32_t clock_now(const struct clock_port *port)
{
    if (port == NULL || port->ops == NULL || port->ops->now_ticks == NULL) {
        return 0u;
    }
    return port->ops->now_ticks(port->context);
}
~~~

The table can be const when operations do not change. Validate the table at initialization and document whether a null operation is allowed.

A table is an ABI: member order, parameter types, calling convention, and lifetime matter. Version or size the table when implementations may evolve independently.

## Driver Interfaces

A driver table can separate portable policy from hardware operations:

~~~c
struct uart_driver {
    int (*configure)(void *context, unsigned int baud);
    int (*write)(void *context, const uint8_t *data, size_t length);
    int (*read)(void *context, uint8_t *data, size_t capacity,
                size_t *received);
    void *context;
};

int uart_send(const struct uart_driver *driver,
              const uint8_t *data, size_t length)
{
    if (driver == NULL || driver->write == NULL) {
        return -1;
    }
    return driver->write(driver->context, data, length);
}
~~~

The portable layer can be tested with a fake table. The hardware adapter owns registers, interrupts, DMA, and platform-specific timing.

## State Callbacks

A state machine can use a table instead of a large conditional:

~~~c
struct controller;

typedef int (*state_enter_fn)(struct controller *controller);
typedef int (*state_event_fn)(struct controller *controller, unsigned int event);

struct controller_state {
    state_enter_fn enter;
    state_event_fn event;
};

struct controller {
    const struct controller_state *state;
};
~~~

Keep state tables static and immutable when possible. Verify every function pointer before invocation and define behavior for an unknown state.

Table-driven code is not automatically safer. It moves control flow into data, so review must cover table completeness, index bounds, and function-pointer lifetime.

## Reentrancy And Invocation Context

A callback can re-enter its source directly or indirectly:

- a driver emits an event while holding an internal lock;
- the callback calls a stop or unregister function;
- an ISR schedules a task that invokes the same object;
- a logger callback triggers another event;
- a completion callback runs during shutdown.

Define whether callbacks run inside or outside locks, whether they may call public APIs, and whether events are queued or delivered synchronously. Prefer copying the callback and context under synchronization, releasing locks, then invoking outside the lock when the design permits.

## Mockable Dependencies

A function table makes a hardware dependency replaceable:

~~~c
struct storage_ops {
    int (*read)(void *context, uint32_t address,
                uint8_t *data, size_t length);
    int (*write)(void *context, uint32_t address,
                 const uint8_t *data, size_t length);
    void *context;
};

int load_record(const struct storage_ops *storage,
                uint8_t *data, size_t capacity)
{
    if (storage == NULL || storage->read == NULL
        || data == NULL || capacity < 4u) {
        return -1;
    }

    return storage->read(storage->context, 0u, data, 4u);
}
~~~

Tests can provide short reads, corrupt data, timeouts, and power-loss behavior without touching flash hardware.

## Exercises

1. Define a callback with context and test registration, replacement, and unregistration.
2. Add a generation counter to reject stale asynchronous completions.
3. Build a fake UART operation table and test short writes, timeouts, and failures.
4. Review whether callbacks execute under locks and document allowed reentrancy.
5. Version a function table with size and feature flags.
6. Create a state table and test missing, invalid, and out-of-range entries.

## Common Mistakes

- Casting incompatible callback types.
- Storing a context pointer beyond its lifetime.
- Releasing context before queued callbacks finish.
- Calling callbacks while holding locks without a reentrancy policy.
- Treating a function table as immutable without enforcing its lifetime.
- Failing to validate a null operation pointer.
- Using table indexes without bounds checks.
- Hiding blocking and execution-context behavior.
- Letting callbacks unregister or destroy their source unexpectedly.

## Debugging Checklist

1. Log callback registration, generation, context, invocation, and removal.
2. Check function-pointer compatibility at compile time.
3. Reproduce teardown with callbacks queued and in flight.
4. Inspect lock ownership when callbacks run.
5. Validate table size, version, and every required operation.
6. Use fake tables to inject short operations and failures.
7. Add a lifetime assertion or generation check to asynchronous completions.
8. Verify ISR and task stack, timing, and blocking restrictions.

## Related Topics

- [Modular Design And APIs overview](./index.md)
- [APIs And Opaque Types](./api-and-opaque-types.md)
- [Ownership And Resource Lifetimes](./ownership-and-resource-lifetimes.md)
- [Architecture Patterns](./architecture-patterns.md)
- [Semantics And Memory](../semantics-and-memory/index.md)
- [Embedded C And Hardware](../embedded-c-and-hardware/index.md)
- [C Programming](../index.md)

## References

- [ISO/IEC 9899 standards and drafts](https://open-std.org/jtc1/sc22/wg14/www/standards.html)
- [C11 public draft N1570](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n1570.pdf)
- [Clang function type and attribute documentation](https://clang.llvm.org/docs/AttributeReference.html)
- [CERT C callback and concurrency rules](https://wiki.sei.cmu.edu/confluence/display/c)

---
status: draft
reviewed: false
domain: c
difficulty: intermediate
last_reviewed: null
---

# Architecture Patterns

Architecture is the arrangement of responsibilities and dependencies across modules. C does not force an object model or framework, so embedded teams can choose a style that fits timing, memory, hardware, and verification needs. The important property is that the chosen structure makes state, ownership, and change boundaries visible.

## Learning Objectives

- Separate policy, mechanism, platform, and application layers.
- Use ports and adapters to isolate hardware.
- Choose state-driven, event-driven, or table-driven structures deliberately.
- Apply dependency injection without unnecessary framework complexity.
- Recognize object-oriented patterns that fit C and those that obscure it.
- Keep compile-time configuration bounded and testable.

## Layered Architecture

A layered system often separates:

| Layer | Responsibility | Should know about |
| --- | --- | --- |
| Application policy | Product behavior and user-visible decisions | Domain services and stable interfaces |
| Domain/service | Reusable operations and state | Ports, data types, policies |
| Port/interface | Required operations and contracts | Abstract types and callbacks |
| Adapter/driver | Hardware or OS implementation | Platform APIs and device details |
| Board support | Startup, clocks, pins, interrupts, memory | Target hardware |
| Toolchain/runtime | ABI, libc, linker, startup | Compiler and platform |

Dependencies should normally point downward through interfaces. A service should not include a board register header merely to obtain a clock or GPIO operation.

A layer is useful only if it reduces change propagation. Artificial layers that merely forward every call add complexity without isolation.

## Ports And Adapters

A port defines what a module needs:

~~~c
#ifndef SENSOR_PORT_H
#define SENSOR_PORT_H

#include <stdint.h>

struct sensor_port {
    int (*read_raw)(void *context, uint16_t *value);
    uint32_t (*now_ticks)(void *context);
    void *context;
};

#endif
~~~

A target adapter supplies the operations:

~~~c
static int board_read_raw(void *context, uint16_t *value)
{
    (void)context;
    *value = BOARD_ADC->DATA;
    return 0;
}
~~~

A host test supplies deterministic values and failures. The service layer should not care which adapter is installed.

Keep ports narrow. A table with dozens of unrelated operations is a disguised dependency on an entire platform.

## Driver And Hardware Abstraction

A driver should own hardware sequencing while exposing stable service behavior:

- validate arguments and state;
- configure registers and clocks;
- handle interrupts and DMA;
- translate hardware statuses;
- enforce access widths and barriers;
- expose bounded operations to callers;
- stop and quiesce hardware before release.

A hardware abstraction should preserve important semantics. Hiding a write-one-to-clear register behind a generic read-modify-write setter is a broken abstraction, not a clean one.

Keep register definitions at the hardware layer. Pass typed domain values upward rather than exposing volatile register structures to application policy.

## State Machines

State machines make legal transitions explicit:

~~~c
enum connection_state {
    CONNECTION_DOWN,
    CONNECTION_CONNECTING,
    CONNECTION_UP,
    CONNECTION_FAULT
};

enum connection_event {
    EVENT_START,
    EVENT_CONNECTED,
    EVENT_TIMEOUT,
    EVENT_STOP,
    EVENT_ERROR
};

enum connection_state connection_step(enum connection_state state,
                                      enum connection_event event)
{
    switch (state) {
    case CONNECTION_DOWN:
        return event == EVENT_START ? CONNECTION_CONNECTING : state;
    case CONNECTION_CONNECTING:
        if (event == EVENT_CONNECTED) {
            return CONNECTION_UP;
        }
        if (event == EVENT_TIMEOUT || event == EVENT_ERROR) {
            return CONNECTION_FAULT;
        }
        return state;
    case CONNECTION_UP:
        return event == EVENT_STOP ? CONNECTION_DOWN : state;
    case CONNECTION_FAULT:
        return event == EVENT_STOP ? CONNECTION_DOWN : state;
    }

    return CONNECTION_FAULT;
}
~~~

Separate the pure transition function from side effects where possible. That makes exhaustive transition tests easy and keeps hardware actions in an adapter or controller layer.

## Event-Driven Design

An event-driven module receives messages rather than being called for every detail:

~~~c
enum event_type {
    EVENT_SAMPLE_READY,
    EVENT_TRANSFER_DONE,
    EVENT_TIMEOUT,
    EVENT_SHUTDOWN
};

struct event {
    enum event_type type;
    uint32_t value;
};

void service_event(const struct event *event)
{
    if (event == NULL) {
        return;
    }

    switch (event->type) {
    case EVENT_SAMPLE_READY:
        process_sample(event->value);
        break;
    case EVENT_TRANSFER_DONE:
        complete_transfer(event->value);
        break;
    case EVENT_TIMEOUT:
        handle_timeout();
        break;
    case EVENT_SHUTDOWN:
        begin_shutdown();
        break;
    }
}
~~~

Define queue capacity, event ownership, ordering, coalescing, overflow behavior, and execution context. An event queue is a resource and a backpressure boundary, not merely an array of structs.

## Table-Driven Design

A table can replace repeated conditionals:

~~~c
struct command_handler {
    unsigned int command;
    int (*handle)(void *context, const uint8_t *data, size_t length);
};

static const struct command_handler handlers[] = {
    {1u, handle_start},
    {2u, handle_stop},
    {3u, handle_status}
};

static int dispatch_command(unsigned int command, void *context,
                            const uint8_t *data, size_t length)
{
    for (size_t i = 0u; i < sizeof handlers / sizeof handlers[0]; ++i) {
        if (handlers[i].command == command) {
            return handlers[i].handle(context, data, length);
        }
    }
    return -1;
}
~~~

Validate tables at compile time where possible. Handle duplicate keys, null functions, unknown commands, and table versioning. A table can centralize policy, but it can also hide control flow from reviewers.

## Dependency Injection

Dependency injection means providing dependencies rather than constructing them inside the module:

~~~c
struct logger {
    void (*write)(void *context, const char *message);
    void *context;
};

struct controller {
    const struct logger *logger;
};

void controller_report(const struct controller *controller,
                       const char *message)
{
    if (controller != NULL && controller->logger != NULL
        && controller->logger->write != NULL) {
        controller->logger->write(controller->logger->context, message);
    }
}
~~~

In C, injection can be a function table, context pointer, allocator parameter, port structure, or compile-time selected implementation. Keep injected interfaces small and make the lifetime of the dependency at least as long as the consumer.

## Object-Oriented Patterns In C

Useful patterns include:

- opaque structs for encapsulation;
- function tables for polymorphic operations;
- context pointers for instance state;
- constructors and destructors for lifecycle;
- explicit interfaces for dependencies;
- composition rather than inheritance.

A common interface shape is:

~~~c
struct stream_ops {
    int (*read)(void *context, uint8_t *data, size_t capacity);
    int (*write)(void *context, const uint8_t *data, size_t length);
    void (*close)(void *context);
};

struct stream {
    const struct stream_ops *ops;
    void *context;
};
~~~

Do not imitate C++ mechanically. Function tables add indirection, expose ABI and lifetime concerns, and may be inappropriate in a tiny interrupt path. Use direct calls when the implementation is fixed and abstraction has no testing or replacement benefit.

## Compile-Time Configuration

Use compile-time configuration for structural differences:

- target memory map;
- available peripherals;
- buffer counts and sizes;
- optional protocol features;
- diagnostic level;
- safety profile.

Validate combinations:

~~~c
#if defined(BOARD_SMALL) && defined(BOARD_LARGE)
#error "Select one board"
#endif

#if !defined(BOARD_SMALL) && !defined(BOARD_LARGE)
#error "No board selected"
#endif

#if defined(FEATURE_DMA) && !defined(HAS_DMA)
#error "DMA feature requires DMA hardware"
#endif
~~~

Use runtime configuration for values that can change without rebuilding. Do not multiply conditional branches across every module; centralize feature decisions and provide one tested configuration per supported product.

## Architecture Tradeoffs

Evaluate an abstraction by:

- binary size and call overhead;
- stack and static memory cost;
- worst-case latency;
- test isolation;
- dependency direction;
- fault containment;
- ABI and upgrade stability;
- debugging visibility;
- team comprehension;
- platform portability.

A senior design is not the most abstract design. It is the smallest structure that keeps important change, failure, timing, and ownership boundaries explicit.

## Exercises

1. Draw layers for a sensor product from application policy to register access.
2. Extract a hardware dependency into a port and implement a host fake.
3. Separate a state machine’s pure transitions from hardware side effects.
4. Design an event queue with explicit overflow and shutdown behavior.
5. Convert a conditional dispatch chain into a table and test duplicate or unknown keys.
6. Compare direct calls and a function table for code size and worst-case latency on the target.
7. Define compile-time configuration checks for two boards and one unsupported feature combination.

## Common Mistakes

- Creating layers that only forward calls and add no boundary.
- Letting application policy depend on register definitions.
- Hiding hardware side effects behind generic operations with the wrong semantics.
- Using global state instead of explicit context and ownership.
- Building event queues without overflow or backpressure policy.
- Using table indexes without bounds and completeness checks.
- Injecting a huge platform interface instead of a narrow port.
- Adding function-pointer indirection to hard real-time paths without measuring it.
- Spreading conditional compilation across every module.
- Assuming an abstraction is good because it is reusable in theory.

## Debugging Checklist

1. Draw the dependency graph and identify cycles.
2. Trace one operation from policy to hardware and back through errors.
3. Check ownership and lifetime across each layer.
4. Measure calls, indirection, stack, and latency on the target.
5. Test every state transition and event-queue overflow path.
6. Compile every supported configuration and reject invalid combinations.
7. Replace hardware adapters with fakes for deterministic host tests.
8. Inspect whether the abstraction preserves register, DMA, cache, and timing semantics.

## Related Topics

- [Modular Design And APIs overview](./index.md)
- [APIs And Opaque Types](./api-and-opaque-types.md)
- [Callbacks And Function Tables](./callbacks-and-function-tables.md)
- [Error Handling](./error-handling.md)
- [Embedded C And Hardware](../embedded-c-and-hardware/index.md)
- [Professional Practice And Capstones](../professional-and-capstone/index.md)
- [C Programming](../index.md)

## References

- [ISO/IEC 9899 standards and drafts](https://open-std.org/jtc1/sc22/wg14/www/standards.html)
- [C11 public draft N1570](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n1570.pdf)
- [GCC function attributes and optimization](https://gcc.gnu.org/onlinedocs/gcc/Common-Function-Attributes.html)
- [GCC preprocessor options](https://gcc.gnu.org/onlinedocs/gcc/Preprocessor-Options.html)

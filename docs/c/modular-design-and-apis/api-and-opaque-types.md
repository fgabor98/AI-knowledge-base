---
status: draft
reviewed: false
domain: c
difficulty: intermediate
last_reviewed: null
---

# APIs And Opaque Types

An API is a contract between a caller and an implementation. The function signature is only one part of that contract. A robust C API also defines representation visibility, ownership, valid states, error behavior, timing, reentrancy, execution context, and versioning.

Opaque types are one of C’s strongest techniques for keeping representation private while allowing efficient, type-checked access.

## Learning Objectives

- Separate public declarations from private representations.
- Design opaque handles and lifecycle functions.
- Document pointer, length, ownership, and context rules.
- Build initialization and shutdown contracts.
- Use versioned structures without unsafe size assumptions.
- Evaluate API stability across compilers, ABIs, boards, and product releases.

## Public And Private Boundaries

A public header should expose only what callers need:

~~~c
#ifndef SENSOR_H
#define SENSOR_H

#include <stddef.h>
#include <stdint.h>

struct sensor;

enum sensor_result {
    SENSOR_OK,
    SENSOR_BAD_ARGUMENT,
    SENSOR_NOT_READY,
    SENSOR_IO_ERROR
};

struct sensor *sensor_create(void);
void sensor_destroy(struct sensor *sensor);
enum sensor_result sensor_read(struct sensor *sensor,
                               int16_t *temperature);
void sensor_close(struct sensor *sensor);

#endif
~~~

The representation of struct sensor is private. Callers can pass a pointer but cannot allocate it by value, access members, or apply sizeof. This prevents callers from depending on layout.

The source file completes the type:

~~~c
#include "sensor.h"

struct sensor {
    uint32_t state;
    uint8_t channel;
};
~~~

Private representation allows the implementation to add fields, change layout, or move from a structure to a handle table without recompiling callers against a leaked layout, although ABI and allocation policy still matter.

## Opaque Handles

A handle can be an opaque pointer, an integer token, or a structure containing validation information:

~~~c
typedef struct uart uart_t;

uart_t *uart_open(unsigned int instance);
void uart_close(uart_t *uart);
~~~

Opaque pointers are convenient when the implementation allocates or owns a context. Integer handles can make stale-handle detection and table ownership explicit, but they require a defined namespace, width, and validation policy.

Do not expose a void pointer as an opaque handle unless losing type checking is intentional. A named incomplete structure gives callers useful diagnostics.

## Lifecycle Contracts

Define the lifecycle as a state machine:

| State | Allowed operations | Ownership |
| --- | --- | --- |
| Uncreated | create/open only | No handle exists |
| Created | configure, query, start | Module owns internal state |
| Active | read/write/service/stop | Hardware or task may be active |
| Quiescing | stop, flush, close | No new asynchronous work |
| Destroyed | None | All pointers and callbacks are invalid |

Initialization should be explicit about repeated calls:

~~~c
enum sensor_result sensor_init(struct sensor *sensor);
enum sensor_result sensor_start(struct sensor *sensor);
enum sensor_result sensor_stop(struct sensor *sensor);
void sensor_deinit(struct sensor *sensor);
~~~

State checks should reject invalid transitions rather than relying on callers to remember hidden order. Deinitialization must stop interrupts, DMA, callbacks, and worker tasks before releasing the context.

## Pointer And Length Contracts

Make bounds and mutability visible:

~~~c
enum packet_result packet_parse(const uint8_t *data,
                                size_t length,
                                struct packet *result);

enum packet_result packet_encode(uint8_t *destination,
                                 size_t capacity,
                                 const struct packet *packet,
                                 size_t *written);
~~~

Document whether null is permitted for an empty input, whether result is unchanged on failure, whether written is required, and whether packet contains borrowed pointers.

Avoid APIs that accept an unbounded pointer and rely on a convention known only to the implementation.

## Versioned Structures

When a structure crosses a module, plugin, bootloader, or firmware boundary, include a size or version field:

~~~c
#include <stdint.h>

struct driver_config {
    uint32_t size;
    uint32_t version;
    uint32_t flags;
    uint32_t baud_rate;
};

#define DRIVER_CONFIG_INIT \
    { sizeof(struct driver_config), 1u, 0u, 115200u }
~~~

The callee can accept older sizes and ignore fields introduced later, or reject unsupported versions. It must validate size before reading optional members; a pointer to a smaller object is not made safe by the structure declaration.

For a public ABI, also define packing, alignment, byte order, calling convention, error width, and ownership. A size field does not solve all ABI compatibility problems.

## Thread And Interrupt Contracts

Document context restrictions next to declarations:

~~~c
/* May block; task context only; not safe from an ISR. */
enum sensor_result sensor_read_blocking(struct sensor *sensor,
                                        int16_t *temperature);

/* Bounded; ISR-safe if the caller provides a valid preallocated result. */
enum sensor_result sensor_try_read(struct sensor *sensor,
                                   int16_t *temperature);
~~~

Comments are not a substitute for implementation, but they make review and static checks possible. Use separate functions when context restrictions materially differ.

## Error And Output Rules

A function should define whether outputs are written on failure. This convention is easy to test:

~~~c
enum sensor_result sensor_read(struct sensor *sensor, int16_t *temperature)
{
    int16_t temporary;

    if (sensor == NULL || temperature == NULL) {
        return SENSOR_BAD_ARGUMENT;
    }

    if (!hardware_read(sensor, &temporary)) {
        return SENSOR_IO_ERROR;
    }

    *temperature = temporary;
    return SENSOR_OK;
}
~~~

The output is committed only after the operation succeeds. If partial results are useful, define which fields are valid and how the caller detects them.

## ABI And Representation

An API intended to cross a binary boundary must specify:

- language linkage and name mangling expectations;
- calling convention and symbol visibility;
- integer widths and signedness;
- structure alignment and packing;
- allocation and release functions;
- error representation;
- thread and callback rules;
- version negotiation;
- endianness and serialization.

Do not expose standard-library types, compiler-specific structures, or ownership-sensitive implementation types across an ABI without a deliberate compatibility policy.

## Testability

An API is easier to test when it separates policy from mechanisms:

~~~c
struct sensor_io {
    int (*read_raw)(void *context, uint16_t *value);
    uint32_t (*now_ticks)(void *context);
    void *context;
};

enum sensor_result sensor_sample(const struct sensor_io *io,
                                 int16_t *temperature);
~~~

A host test can provide deterministic read and time functions. A target adapter supplies registers and timer ticks. The public contract stays the same while hardware effects are isolated.

## Exercises

1. Design an opaque handle for a UART with create, configure, start, stop, and destroy operations.
2. Add a size and version field to a configuration structure and test older and newer callers.
3. Write an API contract for an asynchronous read including callback, buffer ownership, and shutdown.
4. Split a blocking function into task-context and bounded non-blocking variants.
5. Replace a concrete hardware dependency with a small injected port and a host fake.
6. Review an existing header and list every undocumented pointer, error, timing, and context assumption.

## Common Mistakes

- Exposing private structures in public headers without need.
- Returning a handle that can be used before initialization completes.
- Reading optional versioned fields before checking size.
- Allowing callers to retain borrowed pointers beyond their lifetime.
- Leaving output state undefined on failure.
- Using one API for incompatible blocking and interrupt-context contracts.
- Exposing compiler or libc-specific layout as a stable ABI accidentally.
- Destroying a context while callbacks, DMA, or tasks still reference it.
- Treating a version field as a complete ABI strategy.
- Using void pointers where an opaque typed handle would preserve safety.

## Debugging Checklist

1. Write the lifecycle states and allowed transitions.
2. Check every pointer, length, capacity, and output rule at the boundary.
3. Log handle creation, state changes, and destruction.
4. Validate version and size before reading optional data.
5. Test repeated initialization, partial initialization, shutdown, and stale handles.
6. Run host tests with fake ports and injected failures.
7. Inspect ABI layout and symbol visibility for binary interfaces.
8. Exercise callbacks and asynchronous operations during teardown.

## Related Topics

- [Modular Design And APIs overview](./index.md)
- [Translation Units And Headers](./translation-units-and-headers.md)
- [Ownership And Resource Lifetimes](./ownership-and-resource-lifetimes.md)
- [Error Handling](./error-handling.md)
- [Semantics And Memory](../semantics-and-memory/index.md)
- [Compilation, Linking, And ABI](../compilation-linking-and-abi/index.md)
- [C Programming](../index.md)

## References

- [ISO/IEC 9899 standards and drafts](https://open-std.org/jtc1/sc22/wg14/www/standards.html)
- [C11 public draft N1570](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n1570.pdf)
- [Clang attribute reference](https://clang.llvm.org/docs/AttributeReference.html)
- [GCC visibility options](https://gcc.gnu.org/onlinedocs/gcc/Code-Gen-Options.html)

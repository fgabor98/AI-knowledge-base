---
status: draft
reviewed: false
domain: c
difficulty: beginner
last_reviewed: null
---

# Initialization

Initialization gives an object its first value. Assignment changes an existing object. The distinction controls whether an object is valid to read, whether a value can be computed before startup, and how embedded startup code moves data into RAM.

Make initialization explicit at interfaces and state boundaries. “It happens to be zero on this board” is not a sufficient contract unless the language, startup code, and hardware all guarantee it.

## Learning Objectives

- Distinguish initialization from assignment.
- Explain static, automatic, allocated, and thread storage initialization.
- Use scalar, array, structure, union, designated, and compound initializers.
- Reason about omitted members and partial initialization.
- Understand the data and bss startup model.
- Identify why memset is not a universal initializer.
- Avoid initialization-order assumptions across modules.

## Initialization Versus Assignment

An initializer is part of a declaration:

~~~c
#include <stdint.h>

void update_retry_limit(void)
{
    uint32_t retry_limit = 3u;
    retry_limit = 4u;
}
~~~

Initialization establishes the first value. A const object can be initialized but cannot later be assigned:

~~~c
#include <stdint.h>

const uint32_t protocol_version = 2u;
~~~

An assignment requires a modifiable lvalue and a live object.

## Storage Duration And Default Initialization

| Storage duration | Typical declaration | If no initializer is supplied |
| --- | --- | --- |
| Automatic | local non-static variable | Indeterminate value |
| Static | file-scope object or local static | Zero-initialized before startup |
| Allocated | storage returned by an allocator | Bytes are not automatically initialized |
| Thread | thread-local object | Declaration and runtime rules apply |

A local automatic object without an initializer cannot be read safely:

~~~c
#include <stdint.h>

uint32_t safe_counter(void)
{
    uint32_t counter;
    counter = 0u;
    return counter;
}
~~~

The example is safe because counter is assigned before it is read. Debug builds must not be used as evidence that accidental stack contents are valid.

Static-storage objects are initialized before program startup. Without an initializer, arithmetic types become zero, pointers become a null pointer value, and aggregates are initialized recursively. A null pointer’s representation is not required to be all-bits-zero.

## Constant Initialization And Startup

Static-storage initializers use permitted constant expressions and aggregate forms:

~~~c
#include <stdint.h>

static const uint8_t calibration_table[] = {0u, 10u, 20u, 30u};
static uint32_t boot_flags;
static uint32_t retry_limit = 3u;
~~~

A typical embedded image contains:

- text and read-only data in nonvolatile memory;
- data initial values in the image, copied to RAM by startup;
- bss objects represented by a size and zeroed in RAM by startup.

Exact section names and startup sequence are toolchain and linker-script details. A freestanding startup may configure clocks, stack, memory protection, caches, floating point, and the runtime environment before C entry.

If startup fails to copy data or clear bss, correct C source can still observe invalid state. Treat startup and linker scripts as part of the language/runtime boundary.

## Scalar Initialization

Use an initializer appropriate to the type:

~~~c
#include <stdbool.h>
#include <stdint.h>

static uint32_t timeout_ticks = 1000u;
static bool enabled = true;
static float gain = 1.5f;
static const char terminator = '\0';
~~~

The compiler converts the initializer to the destination type. Narrowing can lose information:

~~~c
#include <stdint.h>

static uint8_t truncated = 300u;
~~~

For safety-critical code, make range checks and conversions explicit.

## Array Initialization

An array initializer can infer its bound:

~~~c
#include <stdint.h>

static const uint8_t preamble[] = {0xA5u, 0x5Au, 0x01u};
static uint16_t samples[4] = {100u, 200u};
static char label[8] = "run";
~~~

Remaining sample elements are zero. Designated initializers make sparse tables readable:

~~~c
#include <stdint.h>

static const uint8_t lookup[16] = {
    [0] = 0x10u,
    [3] = 0x30u,
    [15] = 0xF0u
};
~~~

Unspecified elements are initialized to zero.

A character array can have exactly enough elements for visible characters but no terminator:

~~~c
char exact[3] = "abc";
~~~

This is a valid array but not a C string.

## Structure Initialization

Designated initialization makes configuration intent visible:

~~~c
#include <stdbool.h>
#include <stdint.h>

struct uart_config {
    uint32_t baud;
    uint8_t data_bits;
    uint8_t stop_bits;
    bool parity;
};

static const struct uart_config default_uart = {
    .baud = 115200u,
    .data_bits = 8u,
    .stop_bits = 1u,
    .parity = false
};
~~~

Unspecified members are initialized recursively to zero. Later structure assignment copies member values; pointer members are copied as addresses and do not duplicate pointed-to storage.

## Union Initialization

A union initializer initializes one member, normally using a designator:

~~~c
#include <stdint.h>

union register_value {
    uint32_t word;
    uint16_t halfwords[2];
};

static const union register_value reset_value = {
    .word = 0u
};
~~~

Use a tag alongside a union when the active interpretation is program state. Do not assume initializing one member creates a portable representation for reading another.

## Compound Literals

A compound literal creates an unnamed object of a specified type:

~~~c
#include <stdint.h>

struct point {
    int16_t x;
    int16_t y;
};

static void draw_point(struct point point);

void draw_origin(void)
{
    draw_point((struct point){.x = 0, .y = 0});
}
~~~

At block scope it has automatic storage duration until the end of the enclosing block; at file scope it has static storage duration. Do not return a pointer to a block-scope compound literal.

## Variable-Length Arrays And Flexible Members

A variable-length array has a runtime bound and normally automatic storage duration. Its stack budget and maximum bound must be reviewed, and it cannot be treated like a fixed-size initialized array.

A flexible array member contributes no storage to its enclosing structure. Allocate enough trailing storage and initialize the fixed members explicitly; trailing bytes need their own copy or fill operation.

## Why memset Is Not Universal Initialization

This is byte-oriented initialization:

~~~c
#include <stddef.h>
#include <stdint.h>
#include <string.h>

void clear_bytes(uint8_t *bytes, size_t length)
{
    if (bytes != NULL) {
        memset(bytes, 0, length);
    }
}
~~~

It is not a universal typed initializer:

- all-bits-zero need not be a null pointer representation;
- a floating zero and a zero byte pattern are not universally identical;
- padding bytes may remain unspecified;
- structures may contain members needing nonzero defaults;
- memory-mapped registers may have write side effects;
- memset establishes neither synchronization nor ownership.

Use typed initializers for typed objects and memset only for an explicitly byte-oriented contract.

## Initialization Order

C has no language-level constructor system that runs arbitrary code for all global objects. Static objects receive language-defined initial values before startup; runtime initialization order is chosen by the program or framework.

Do not rely on link order between modules. Use an explicit sequence:

~~~c
int system_init(void)
{
    if (clock_init() != 0) {
        return -1;
    }
    if (gpio_init() != 0) {
        return -2;
    }
    if (sensor_init() != 0) {
        return -3;
    }
    return 0;
}
~~~

If initialization may run more than once, define whether it is idempotent, rejects repetition, or restarts the subsystem.

## Reset And Reinitialization

Power-on reset, warm reset, watchdog reset, and bootloader handoff may leave different RAM and peripheral state. Retained memory must be validated rather than treated as ordinary initialized configuration.

Make reset behavior explicit:

- store a version and integrity check with retained data;
- do not trust arbitrary RAM contents;
- reconfigure peripherals whose reset state is not guaranteed;
- clear security-sensitive state on every appropriate reset;
- test every reset class supported by the product.

## Exercises

1. Distinguish an automatic object assigned before use from one read before assignment.
2. Define a sparse lookup table and verify unspecified elements.
3. Create a configuration structure with a safe default and validation.
4. Show why an exactly sized character array is not null-terminated.
5. Replace a memset of a typed structure with a designated initializer and document the difference.
6. Trace target startup and linker map to identify data copying and bss clearing.
7. Define explicit initialization order for clock, GPIO, communication, and sensor subsystems.

## Common Mistakes

- Reading an uninitialized automatic or allocated object.
- Confusing zero initialization with all-bits-zero memory.
- Assuming const data is nonvolatile without checking the linker.
- Forgetting that omitted aggregate members are zeroed only during initialization.
- Passing a non-terminated character array to string APIs.
- Returning a pointer to an automatic array or compound literal.
- Using memset to initialize pointers, floats, structures, or registers.
- Relying on unspecified cross-module initialization order.
- Assuming all reset types clear RAM and restore peripherals identically.
- Forgetting to validate retained state after warm reset.

## Debugging Checklist

1. Identify storage duration and exact point of first read.
2. Distinguish a declaration initializer from later assignment.
3. Inspect the linker map for data, bss, read-only data, and RAM copies.
4. Verify startup copies initialized data and clears zero-initialized data.
5. Compare reset classes and retained-memory behavior.
6. Enable uninitialized-use diagnostics and host sanitizers.
7. Check string capacity and termination separately from array initialization.
8. Replace byte-pattern assumptions with typed initialization or encoding.
9. Log subsystem initialization order and rollback.

## Related Topics

- [Language Fundamentals overview](./index.md)
- [Types, Values, And Objects](./types-values-and-objects.md)
- [Structures, Unions, And Enumerations](./structures-unions-and-enums.md)
- [Compilation, Linking, And ABI](../compilation-linking-and-abi/index.md)
- [Startup, Reset, And Vector Tables](../embedded-c-and-hardware/startup-reset-and-vector-tables.md)
- [Semantics And Memory Model](../semantics-and-memory/index.md)
- [C Programming](../index.md)

## References

- [ISO/IEC 9899 standards and drafts](https://open-std.org/jtc1/sc22/wg14/www/standards.html)
- [C11 public draft N1570](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n1570.pdf)
- [GCC variable attributes and storage details](https://gcc.gnu.org/onlinedocs/gcc/Variable-Attributes.html)
- [CERT C coding rules](https://wiki.sei.cmu.edu/confluence/display/c)

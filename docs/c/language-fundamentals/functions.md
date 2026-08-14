---
status: draft
reviewed: false
domain: c
difficulty: beginner
last_reviewed: null
---

# Functions

Functions are C’s main unit of behavior and interface design. A good function makes its inputs, outputs, ownership, timing, failure modes, and side effects reviewable. In embedded systems, stack use, blocking behavior, interrupt safety, and hardware effects are part of the contract even when the C type signature cannot express them.

## Learning Objectives

- Distinguish a function declaration, prototype, and definition.
- Design value, output-parameter, and status-returning interfaces.
- Explain that C passes arguments by value.
- Use function pointers for callbacks without losing context or ownership clarity.
- Recognize recursion, variadic calls, and inline as deliberate tradeoffs.
- Document reentrancy, blocking, timing, and side effects.

## Declaration, Prototype, And Definition

A declaration that includes a parameter list enables type checking at call sites:

~~~c
#include <stddef.h>
#include <stdint.h>

uint32_t checksum32(const uint8_t *data, size_t length);
~~~

A definition supplies the body:

~~~c
#include <stddef.h>
#include <stdint.h>

uint32_t checksum32(const uint8_t *data, size_t length)
{
    uint32_t result = 0u;

    for (size_t i = 0u; i < length; ++i) {
        result = (result << 5) - result + data[i];
    }

    return result;
}
~~~

Put interface declarations in headers and definitions in one source file. Include the header from the defining source file so declaration and definition mismatches are checked.

An old-style declaration with an empty parameter list does not specify parameters. Use void for a function that accepts none:

~~~c
int initialize(void);
~~~

## C Passes Arguments By Value

Every parameter receives a value. Passing a pointer copies the pointer value; it does not make the pointer itself an alias to the caller’s pointer variable.

~~~c
#include <stddef.h>

void set_local_pointer(int *pointer)
{
    pointer = NULL;
}

void set_caller_pointer(int **pointer)
{
    if (pointer != NULL) {
        *pointer = NULL;
    }
}
~~~

An array parameter is adjusted to a pointer parameter. A bound written in the parameter does not enforce the caller’s buffer size:

~~~c
#include <stddef.h>
#include <stdint.h>

void consume(uint8_t data[16], size_t length);
~~~

Put the length in the interface and validate it.

## Return Values And Output Parameters

Use the return value for the primary result when there is one. Use an output parameter when a function needs to return a status and a separate result:

~~~c
#include <stddef.h>
#include <stdint.h>

enum read_status {
    READ_OK,
    READ_NO_DATA,
    READ_IO_ERROR
};

enum read_status read_temperature(int16_t *temperature)
{
    if (temperature == NULL) {
        return READ_IO_ERROR;
    }
    if (!sensor_has_sample()) {
        return READ_NO_DATA;
    }

    *temperature = sensor_read_temperature();
    return READ_OK;
}
~~~

The contract should say whether an output is written on failure. A safe convention is that outputs are unchanged unless success is returned.

Never return a pointer to an automatic object:

~~~c
const char *bad_name(void)
{
    char name[8] = "sensor";
    return name;
}
~~~

Return a pointer only when the storage outlives the call and ownership is clear: static storage, caller-provided storage, or explicitly transferred allocation.

## Function Contracts

A production function contract should answer:

- Which inputs are valid? May pointers be null? Are ranges inclusive?
- Which input objects are read or modified?
- Who owns each pointer before, during, and after the call?
- Is it blocking, polling, or bounded by a timeout?
- Can it be called from an interrupt, multiple tasks, or a signal handler?
- Does it access hardware, global state, or persistent storage?
- What does each status mean, and which outputs are valid on failure?
- What is the expected stack use and worst-case execution time?
- Does it preserve state on failure or perform a partial update?

Types express some of this. Naming, documentation, assertions, tests, and review express the rest.

## static And Interface Visibility

At file scope, static gives an object or function internal linkage:

~~~c
#include <stdint.h>

static uint32_t retry_count;

static void reset_retry_count(void)
{
    retry_count = 0u;
}
~~~

This keeps private implementation details out of the link-visible interface. Do not confuse file-scope static with a local static object, which has block scope but static storage duration:

~~~c
unsigned int next_sequence(void)
{
    static unsigned int sequence;
    return sequence++;
}
~~~

A local static persists between calls and is shared state, so the function is not automatically reentrant or thread-safe.

## Function Pointers And Callbacks

A function pointer stores the address of a function with a compatible function type:

~~~c
#include <stddef.h>
#include <stdint.h>

typedef void (*event_callback)(void *context, uint32_t event);

struct event_source {
    event_callback callback;
    void *context;
};

static void notify(const struct event_source *source, uint32_t event)
{
    if (source != NULL && source->callback != NULL) {
        source->callback(source->context, event);
    }
}
~~~

The context pointer lets one callback implementation serve multiple instances. The owner must ensure that the context remains alive and synchronized for every callback.

Function pointer compatibility is strict. Never cast a function pointer just to silence a warning.

Callback lifecycle needs a contract:

- Can registration happen while callbacks run?
- Can a callback unregister itself?
- Which context invokes it: ISR, task, worker, or polling loop?
- May it block?
- Who serializes concurrent calls?
- What happens during shutdown?

## inline

inline is a request concerning definition and linkage, not a guarantee of inlining:

~~~c
#include <stdint.h>

static inline uint32_t min_u32(uint32_t left, uint32_t right)
{
    return left < right ? left : right;
}
~~~

static inline in a header is common for small helpers. The compiler may inline or not inline it depending on optimization, address-taking, debug options, and target cost models. Inspect generated code when performance matters.

## Recursion And Stack

Recursion is legal, but every call consumes stack and can be difficult to bound. Embedded code should use recursion only when maximum depth is known, the stack budget includes all callers and interrupt use, malformed input cannot create unbounded depth, and the toolchain supports the implementation. The standard does not require tail-call optimization.

An iterative implementation is often easier to analyze for parsers, tree walks, and protocol handling.

## Variadic Functions

Variadic functions use an ellipsis and stdarg.h:

~~~c
#include <stdarg.h>
#include <stdio.h>

static void log_values(const char *format, ...)
{
    va_list arguments;
    va_start(arguments, format);
    vprintf(format, arguments);
    va_end(arguments);
}
~~~

The fixed parameters and format contract determine how unnamed arguments are interpreted. The compiler cannot generally verify custom formats. A mismatch can corrupt interpretation of later arguments.

Variadic logging can have code-size, stack, timing, and reentrancy costs. Prefer a typed logging interface or reviewed format subset for constrained targets.

## Reentrancy And Thread Safety

A function is reentrant when a new invocation can safely begin before an earlier invocation finishes, including through an interrupt, callback, or recursion. Thread safety concerns correct concurrent use by multiple execution contexts. They overlap but are not identical.

A function using only local state can still be non-reentrant if a dependency or hardware sequence is not reentrant. A function protected by a lock may be thread-safe while still unsafe in an interrupt context because the interrupt cannot take that lock.

Document execution-context assumptions in the API.

## Small Interface Example

~~~c
#ifndef SENSOR_H
#define SENSOR_H

#include <stddef.h>
#include <stdint.h>

enum sensor_result {
    SENSOR_OK,
    SENSOR_BAD_ARGUMENT,
    SENSOR_NOT_READY,
    SENSOR_IO_ERROR
};

struct sensor_sample {
    int16_t temperature;
    uint16_t raw;
};

enum sensor_result sensor_read(struct sensor_sample *out);
enum sensor_result sensor_read_into(struct sensor_sample *out,
                                    uint8_t *scratch,
                                    size_t scratch_size);

#endif
~~~

The header says what exists but not every operational detail. Document whether the calls block, whether out must be non-null, whether scratch is retained, and whether the functions are safe from interrupt context.

## Exercises

1. Write a header and source file for a bounded ring-buffer interface.
2. Design a function that reports not-ready, invalid-input, and hardware-failure results.
3. Add a callback and context pointer to a fake driver, then test lifetime and unregister behavior.
4. Measure recursive parser stack use and rewrite it iteratively if needed.
5. Review functions and record input, output, ownership, blocking, and execution-context contracts.
6. Enable warnings for missing prototypes and incompatible function pointers; fix warnings without hiding mismatches with casts.

## Common Mistakes

- Defining a public function without a visible prototype.
- Using int function() when the function takes no arguments.
- Returning the address of an automatic object.
- Believing C passes arrays or structures by reference automatically.
- Assuming an array parameter bound enforces a caller buffer size.
- Casting incompatible function pointers.
- Using a global static buffer in a concurrently called function.
- Claiming a function is fast without target measurement or a worst-case bound.
- Treating inline as guaranteed optimization.
- Using variadic logging in an interrupt or hard real-time path without a reviewed contract.

## Debugging Checklist

1. Check that every call sees the intended prototype.
2. Compile the defining source with its own public header included.
3. Inspect parameter types, array bounds, and output-pointer validity.
4. Trace ownership and lifetime of every pointer crossing the call.
5. Record the invocation context and preemption possibilities.
6. Test each status result and output state on failure.
7. Measure stack and execution time on the target.
8. If a callback crashes, log registration, context, invocation context, and unregistration timing.

## Related Topics

- [Language Fundamentals overview](./index.md)
- [Declarations And Declarators](./declarations-and-declarators.md)
- [Modular Design And APIs](../modular-design-and-apis/index.md)
- [Semantics And Memory Model](../semantics-and-memory/index.md)
- [C Memory Model And Concurrency](../advanced-c/c-memory-model-and-concurrency.md)
- [C Programming](../index.md)

## References

- [ISO/IEC 9899 standards and drafts](https://open-std.org/jtc1/sc22/wg14/www/standards.html)
- [C11 public draft N1570](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n1570.pdf)
- [GCC function attributes](https://gcc.gnu.org/onlinedocs/gcc/Common-Function-Attributes.html)
- [CERT C Coding Standard](https://wiki.sei.cmu.edu/confluence/display/c)

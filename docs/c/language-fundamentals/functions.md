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

<!-- c-check: expect-warning -->

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
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

bool sensor_has_sample(void);
int16_t sensor_read_temperature(void);

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

<!-- c-check: expect-warning -->

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

### Model Answers

Try each exercise before opening its answer. Where an exercise asks you to build
something, the answer shows one good solution, not the only one.

??? note "Exercise 1: a bounded ring-buffer interface"
    ```c
    /* ring_buffer.h */
    #ifndef RING_BUFFER_H
    #define RING_BUFFER_H

    #include <stdbool.h>
    #include <stddef.h>
    #include <stdint.h>

    /* Members are visible so callers can allocate a ring without a heap.
       Use only the functions below to touch them. */
    struct ring_buffer {
        uint8_t *storage;
        size_t capacity;
        size_t head;                /* index of the oldest byte */
        size_t count;               /* bytes currently stored */
    };

    /* storage must hold capacity bytes and outlive the ring. False if capacity is 0. */
    bool ring_init(struct ring_buffer *ring, uint8_t *storage, size_t capacity);

    /* False when full; the ring is unchanged. */
    bool ring_push(struct ring_buffer *ring, uint8_t byte);

    /* False when empty; *byte is unchanged. */
    bool ring_pop(struct ring_buffer *ring, uint8_t *byte);

    size_t ring_count(const struct ring_buffer *ring);
    size_t ring_space(const struct ring_buffer *ring);

    #endif
    ```

    ```c
    /* ring_buffer.c */
    #include "ring_buffer.h"

    bool ring_init(struct ring_buffer *ring, uint8_t *storage, size_t capacity)
    {
        if (ring == NULL || storage == NULL || capacity == 0) {
            return false;
        }
        *ring = (struct ring_buffer){ .storage = storage, .capacity = capacity };
        return true;
    }

    bool ring_push(struct ring_buffer *ring, uint8_t byte)
    {
        if (ring->count == ring->capacity) {
            return false;
        }
        size_t tail = ring->head + ring->count;

        if (tail >= ring->capacity) {
            tail -= ring->capacity;             /* wrap without %: head, count < capacity */
        }
        ring->storage[tail] = byte;
        ring->count++;
        return true;
    }

    bool ring_pop(struct ring_buffer *ring, uint8_t *byte)
    {
        if (ring->count == 0) {
            return false;
        }
        *byte = ring->storage[ring->head];
        ring->head++;
        if (ring->head == ring->capacity) {
            ring->head = 0;
        }
        ring->count--;
        return true;
    }

    size_t ring_count(const struct ring_buffer *ring)
    {
        return ring->count;
    }

    size_t ring_space(const struct ring_buffer *ring)
    {
        return ring->capacity - ring->count;
    }
    ```

    ```c
    /* ring_test.c */
    #include <assert.h>
    #include "ring_buffer.h"

    int main(void)
    {
        uint8_t storage[3];
        struct ring_buffer ring;
        uint8_t byte = 0xAA;

        assert(!ring_init(&ring, storage, 0));
        assert(ring_init(&ring, storage, sizeof storage));
        assert(!ring_pop(&ring, &byte) && byte == 0xAA);        /* empty: output unchanged */

        assert(ring_push(&ring, 1) && ring_push(&ring, 2) && ring_push(&ring, 3));
        assert(!ring_push(&ring, 4));                           /* full: rejected */
        assert(ring_count(&ring) == 3 && ring_space(&ring) == 0);

        assert(ring_pop(&ring, &byte) && byte == 1);
        assert(ring_push(&ring, 4));                            /* wraps to slot 0 */
        for (uint8_t expected = 2; expected <= 4; expected++) {
            assert(ring_pop(&ring, &byte) && byte == expected); /* FIFO order across the wrap */
        }
        assert(ring_count(&ring) == 0);
        return 0;
    }
    ```

    - Every function that can fail says how, and what it leaves unchanged: a full ring
      rejects the byte, and an empty ring does not write `*byte`.
    - The caller supplies the storage, so the module needs no heap and the capacity is
      visible at the call site. The cost is that `struct ring_buffer` is complete in the
      header. If callers must not depend on the layout, make it opaque as in
      [Declarations exercise 3](declarations-and-declarators.md#model-answers) and allocate
      from a pool.
    - `head` plus `count` avoids the classic "is it full or empty?" ambiguity of storing
      `head` and `tail` alone, and allows any capacity, not just powers of two.
    - The tests hit the edges: capacity 0, popping when empty, pushing when full, and FIFO
      order across the wrap.
    - This version is for one execution context. If an interrupt pushes while a task pops,
      `count` is written by both. The usual fix is separate `head` and `tail` indices,
      each written by only one side, with atomic loads and stores.

??? note "Exercise 2: three different failure results"
    ```c
    #include <stdbool.h>
    #include <stddef.h>
    #include <stdint.h>

    #define ADC_CHANNELS 8u

    enum adc_status {
        ADC_OK,
        ADC_NOT_READY,          /* try again later; nothing is wrong */
        ADC_INVALID_ARGUMENT,   /* the caller has a bug; retrying cannot help */
        ADC_HW_FAILURE          /* the device needs recovery or reporting */
    };

    bool adc_conversion_done(unsigned int channel);
    bool adc_error_flag(void);
    uint16_t adc_raw(unsigned int channel);

    /* On anything except ADC_OK, *millivolts is left unchanged. */
    enum adc_status adc_read_mv(unsigned int channel, uint16_t *millivolts)
    {
        if (millivolts == NULL || channel >= ADC_CHANNELS) {
            return ADC_INVALID_ARGUMENT;
        }
        if (adc_error_flag()) {
            return ADC_HW_FAILURE;
        }
        if (!adc_conversion_done(channel)) {
            return ADC_NOT_READY;
        }
        *millivolts = (uint16_t)((adc_raw(channel) * 3300u) / 4095u);
        return ADC_OK;
    }
    ```

    | Result | Meaning | `*millivolts` | What the caller does |
    | --- | --- | --- | --- |
    | `ADC_OK` | a fresh value | written | use it |
    | `ADC_NOT_READY` | conversion still running | unchanged | poll again or wait for the interrupt |
    | `ADC_INVALID_ARGUMENT` | null pointer or bad channel | unchanged | nothing at run time; it is a bug, so assert in debug builds |
    | `ADC_HW_FAILURE` | the converter reported an error | unchanged | reset the peripheral, count the fault, report it |

    The test for a good status set is that each value leads to a *different* caller action.
    A single `-1` for all three forces the caller to retry a bug or ignore a hardware fault.
    Arguments are checked before the hardware is touched, and a fault is checked before
    "not ready", so a broken converter is not reported as merely busy forever.

??? note "Exercise 3: a callback with a context pointer"
    ```c
    /* fake_timer.h */
    #ifndef FAKE_TIMER_H
    #define FAKE_TIMER_H

    typedef void (*timer_callback)(void *context);

    enum timer_status { TIMER_OK, TIMER_BUSY, TIMER_INVALID };

    /* context must stay valid until timer_unregister() returns. */
    enum timer_status timer_register(timer_callback callback, void *context);
    void timer_unregister(void);    /* safe to call from inside the callback */
    void timer_fire(void);          /* test hook: what the timer interrupt would do */

    #endif
    ```

    ```c
    /* fake_timer.c */
    #include <stddef.h>
    #include "fake_timer.h"

    static timer_callback registered_callback;
    static void *registered_context;

    enum timer_status timer_register(timer_callback callback, void *context)
    {
        if (callback == NULL) {
            return TIMER_INVALID;
        }
        if (registered_callback != NULL) {
            return TIMER_BUSY;              /* one owner at a time: no silent replace */
        }
        registered_context = context;
        registered_callback = callback;
        return TIMER_OK;
    }

    void timer_unregister(void)
    {
        registered_callback = NULL;
        registered_context = NULL;
    }

    void timer_fire(void)
    {
        /* Copy first, so a callback that unregisters itself does not change
           what this call is using. */
        timer_callback callback = registered_callback;
        void *context = registered_context;

        if (callback != NULL) {
            callback(context);
        }
    }
    ```

    ```c
    /* timer_test.c */
    #include <assert.h>
    #include <stddef.h>
    #include "fake_timer.h"

    struct counter {
        int calls;
    };

    static void count_call(void *context)
    {
        struct counter *counter = context;

        counter->calls++;
    }

    static void one_shot(void *context)
    {
        count_call(context);
        timer_unregister();                     /* unregisters itself */
    }

    int main(void)
    {
        struct counter a = { 0 };
        struct counter b = { 0 };

        assert(timer_register(NULL, &a) == TIMER_INVALID);
        assert(timer_register(count_call, &a) == TIMER_OK);
        assert(timer_register(count_call, &b) == TIMER_BUSY);   /* no silent replace */
        timer_fire();
        timer_fire();
        assert(a.calls == 2 && b.calls == 0);   /* the context is the registered one */

        timer_unregister();
        timer_fire();
        assert(a.calls == 2);                   /* no call after unregister */

        assert(timer_register(one_shot, &b) == TIMER_OK);
        timer_fire();
        timer_fire();
        assert(b.calls == 1);                   /* self-unregister took effect */
        return 0;
    }
    ```

    The tests cover registration, a refused second registration, delivery of the right
    context, no calls after unregistering, and a callback that unregisters itself.
    `timer_fire` copies the callback and context before calling, which is what makes the
    self-unregister case safe.

    The lifetime rule is the one that fails in real code. A context on the stack that is
    never unregistered:

    <!-- c-check: skip -- fragment; uses struct counter and count_call from the test above -->

    ```c
    static void start_measurement(void)
    {
        struct counter local = { 0 };

        timer_register(count_call, &local);     /* bug: never unregistered */
    }
    ```

    After `start_measurement` returns, the next `timer_fire()` writes into a dead stack
    frame. ASan finds it when asked to check for this:

    ```text
    $ ASAN_OPTIONS=detect_stack_use_after_return=1 ./lifetime_bug
    ==641254==ERROR: AddressSanitizer: stack-use-after-return on address 0x7db1df700020 ...
    READ of size 4 at 0x7db1df700020 thread T0
        #0 ... in count_call leak_ctx.c:3
        #1 ... in timer_fire fake_timer.c:35
    Address 0x7db1df700020 is located in stack of thread T0 at offset 32 in frame
        #0 ... in start_measurement leak_ctx.c:5
    ```

    On real hardware the callback runs in an interrupt, so `timer_unregister` must also stop
    the interrupt from running between its two stores. Disable the timer interrupt around
    them, or store the callback with an atomic write and clear it first.

??? note "Exercise 4: measuring recursive stack use"
    A bracket matcher written both ways:

    ```c
    #include <stdbool.h>
    #include <stddef.h>

    /* Recursive: one call per nesting level. */
    static bool parse_group(const char **cursor, char close)
    {
        while (**cursor != '\0') {
            char c = *(*cursor)++;

            if (c == '(' || c == '[' || c == '{') {
                char want = c == '(' ? ')' : c == '[' ? ']' : '}';
                if (!parse_group(cursor, want)) {
                    return false;
                }
            } else if (c == ')' || c == ']' || c == '}') {
                return c == close;
            }
        }
        return close == '\0';
    }

    bool balanced_recursive(const char *text)
    {
        return parse_group(&text, '\0');
    }

    #define MAX_DEPTH 32u

    /* Iterative: the nesting lives in a fixed array, and depth is checked. */
    bool balanced_iterative(const char *text)
    {
        char expected[MAX_DEPTH];
        size_t depth = 0;

        for (; *text != '\0'; text++) {
            char c = *text;

            if (c == '(' || c == '[' || c == '{') {
                if (depth == MAX_DEPTH) {
                    return false;                  /* too deep: reject, do not crash */
                }
                expected[depth++] = c == '(' ? ')' : c == '[' ? ']' : '}';
            } else if (c == ')' || c == ']' || c == '}') {
                if (depth == 0 || expected[--depth] != c) {
                    return false;
                }
            }
        }
        return depth == 0;
    }
    ```

    GCC reports each function's frame with `-fstack-usage`:

    ```text
    $ gcc -std=c17 -O2 -fstack-usage -c parser.c && cat parser.su
    parser.c:5:13:parse_group	32	static
    parser.c:22:6:balanced_recursive	32	static
    parser.c:30:6:balanced_iterative	64	static
    ```

    At `-O0` `parse_group` uses 48 bytes, so measure the build you ship. The `.su` file gives
    the cost *per call*. For the recursive version the worst case is
    `32 bytes × nesting depth`, and the input sets the depth. With an 8 MB host stack the
    recursive version crashed between 250,000 and 300,000 nested `(` (8 MB / 32 bytes is
    about 262,000). The iterative version returned `false`. On a microcontroller with a 2 KB
    stack, about 60 levels are enough to overflow, and the overflow may silently corrupt
    other memory instead of faulting.

    - The rewrite moves the nesting into a fixed array, so its stack use is 64 bytes for any
      input. Deeper input is *rejected*, which is a defined, testable result.
    - `-Wstack-usage=N` warns about any single frame above N bytes, but it does not follow
      recursion. Call-graph tools such as GCC's `-fcallgraph-info=su` or a static stack
      analyzer flag recursive cycles, and those need a depth argument from a person.
    - Keep recursion only where the depth has a small, proven bound that does not come from
      input. Add that bound times the frame size to the stack budget, and leave room for
      interrupts.

??? note "Exercise 5: recording function contracts"
    A review of three functions from this page:

    | Contract | `checksum32(data, length)` | `read_temperature(temperature)` | `notify(source, event)` |
    | --- | --- | --- | --- |
    | Inputs | `data` may be null only if `length == 0`; any length | `temperature` must not be null | `source` may be null (ignored); `event` is any value |
    | Outputs | returns the checksum; writes nothing | writes `*temperature` only on `READ_OK` | none of its own; whatever the callback does |
    | Ownership | borrows `data` for the call only | borrows `temperature` for the call only | borrows `source`; `source->context` must stay valid while registered |
    | Blocking | no; O(`length`) time | no; one sensor poll | inherits the callback's blocking behavior |
    | Context | any: no shared state, reentrant | not stated: depends on the sensor driver functions | the same as the caller's, so an ISR caller makes the callback run in an ISR |

    The review findings are the gaps:

    - `checksum32`: it is not documented whether `data == NULL` with `length == 0` is
      allowed. The loop never reads it, so it works, but the contract should say so.
    - `read_temperature`: a null pointer returns `READ_IO_ERROR`, which tells the caller
      "the hardware failed" when the real problem is a caller bug. A separate
      invalid-argument result is better (see exercise 2). Whether the sensor functions may
      be called from an interrupt is unknown, so this function's context is unknown too.
    - `notify`: it runs the callback synchronously in the caller's context. Callers in an ISR
      must know that every registered callback runs in that ISR and must not block.

    Record the answers in the header comment. The point of the exercise is that "not
    stated" appears in the table at all.

??? note "Exercise 6: prototype and function-pointer warnings"
    <!-- c-check: expect-warning -->

    ```c
    #include <stdint.h>
    #include <stdlib.h>

    int initialize();                 /* old-style: says nothing about parameters */

    int scale(int value)              /* external, but no header declares it */
    {
        return value * 4;
    }

    struct reading {
        uint32_t timestamp;
        int16_t value;
    };

    static int by_timestamp(const struct reading *left, const struct reading *right)
    {
        return (left->timestamp > right->timestamp) - (left->timestamp < right->timestamp);
    }

    void sort_readings(struct reading *readings, size_t count)
    {
        qsort(readings, count, sizeof readings[0], by_timestamp);
    }
    ```

    ```text
    $ gcc -std=c17 -Wall -Wextra -Wmissing-prototypes -Wstrict-prototypes -c proto.c
    proto.c:4:1: warning: function declaration isn't a prototype [-Wstrict-prototypes]
    proto.c:6:5: warning: no previous prototype for 'scale' [-Wmissing-prototypes]
    proto.c:21:6: warning: no previous prototype for 'sort_readings' [-Wmissing-prototypes]
    proto.c:23:48: warning: passing argument 4 of 'qsort' from incompatible pointer type [-Wincompatible-pointer-types]
    note: expected '__compar_fn_t' {aka 'int (*)(const void *, const void *)'} but argument is of type 'int (*)(const struct reading *, const struct reading *)'
    ```

    Only the last warning is on by default. `-Wmissing-prototypes` and `-Wstrict-prototypes`
    must be added explicitly. The fixes:

    ```c
    #include <stdint.h>
    #include <stdlib.h>

    int initialize(void);             /* a prototype: takes no arguments */

    static int scale(int value)       /* only used here: internal linkage */
    {
        return value * 4;
    }

    struct reading {
        uint32_t timestamp;
        int16_t value;
    };

    void sort_readings(struct reading *readings, size_t count);   /* belongs in a header */

    static int by_timestamp(const void *left_ptr, const void *right_ptr)
    {
        const struct reading *left = left_ptr;
        const struct reading *right = right_ptr;

        return (left->timestamp > right->timestamp) - (left->timestamp < right->timestamp);
    }

    void sort_readings(struct reading *readings, size_t count)
    {
        qsort(readings, count, sizeof readings[0], by_timestamp);
    }
    ```

    - A missing prototype is fixed by deciding the linkage. If other files call the
      function, declare it in the header the defining file includes. If not, make it
      `static`.
    - The comparator gets `qsort`'s exact type and converts `const void *` inside.
      `(int (*)(const void *, const void *))by_timestamp` would silence the warning and keep
      the undefined call through an incompatible type.
    - C23 makes `int initialize();` mean "no parameters", but code that must also build as
      C17 should still write `void`.

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

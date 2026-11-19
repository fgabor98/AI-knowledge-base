---
status: draft
reviewed: false
domain: c
difficulty: intermediate
last_reviewed: null
---

# Storage Duration, Scope, And Linkage

C uses several independent concepts to describe names and objects. Scope says where a name is visible. Linkage says whether declarations in different scopes or translation units denote the same entity. Storage duration says how long the object’s storage exists. Lifetime says when the object may be accessed as an object of its type.

Confusing these concepts causes static-state leaks, duplicate symbols, dangling pointers, and startup bugs.

## Learning Objectives

- Distinguish scope, linkage, storage duration, and lifetime.
- Classify automatic, static, allocated, and thread storage.
- Use file-scope static and extern correctly.
- Explain tentative definitions and header ownership.
- Identify whether an address remains valid across calls, tasks, and resets.
- Connect source declarations to embedded sections and startup behavior.

## Scope

Scope is a property of an identifier:

- Function scope applies to labels.
- Function-prototype scope applies to parameter names in a prototype.
- Block scope applies to declarations inside a block, including function parameters.
- File scope applies to declarations outside functions.

A name can be invisible while the object it denotes still exists. A local static is the canonical example:

~~~c
#include <stdint.h>

uint32_t next_id(void)
{
    static uint32_t id;
    return ++id;
}
~~~

The identifier id has block scope, but the object has static storage duration and survives between calls. The function is not reentrant merely because the name is local.

## Linkage

Linkage determines whether declarations denote the same entity:

- No linkage: a local variable, parameter, or member name.
- Internal linkage: a file-scope name declared static; visible only within that translation unit.
- External linkage: a name that can be referred to from other translation units.

~~~c
/* counter.h */
#ifndef COUNTER_H
#define COUNTER_H

#include <stdint.h>

extern uint32_t counter;
void counter_reset(void);

#endif
~~~

~~~c
/* counter.c */
#include "counter.h"

uint32_t counter;

void counter_reset(void)
{
    counter = 0u;
}
~~~

~~~c
/* private.c */
#include <stdint.h>

static uint32_t diagnostic_count;
~~~

The header declares the public object but does not define storage. Exactly one source file owns the external definition. A file-scope static definition is private to its source file.

Prefer functions over public mutable objects. If an object must be shared, document synchronization, ownership, reset behavior, and whether callers may retain its address.

## Storage Durations

| Storage duration | Common source form | Lifetime | Embedded considerations |
| --- | --- | --- | --- |
| Automatic | Non-static local or parameter | Block entry to block exit | Usually stack; bounded depth matters |
| Static | File-scope or local static | Entire program execution | Often data or bss; startup initializes it |
| Allocated | Storage from an allocator | Allocation to release | Heap policy and failure are part of the design |
| Thread | Thread-local object | Thread creation to termination | Requires runtime and RTOS support |

An object’s storage duration does not by itself specify physical placement. A compiler or linker can place static data in RAM, retention RAM, a special memory section, or another target-specific region.

## Automatic Objects

Automatic objects are created when execution enters their block and cease to exist when execution leaves:

<!-- c-check: expect-warning -->

~~~c
#include <stddef.h>

const int *invalid_view(void)
{
    int values[4] = {1, 2, 3, 4};
    return values;
}
~~~

The pointer returned by invalid_view is dangling after the function returns. Returning a pointer to caller-provided storage or static storage requires a different contract:

~~~c
#include <stddef.h>

size_t fill_values(int values[4])
{
    if (values == NULL) {
        return 0u;
    }

    for (size_t i = 0u; i < 4u; ++i) {
        values[i] = (int)i;
    }
    return 4u;
}
~~~

Automatic storage is convenient, but a large local array in a deeply nested or interruptible path can exhaust a firmware stack.

## Static Objects And Initialization

Static-storage objects exist before main or the implementation’s equivalent program entry. They receive constant initialization and otherwise zero initialization according to C rules. A local static is initialized once, not each time its declaration is reached:

~~~c
int retry_count(void)
{
    static int count = 0;
    return count++;
}
~~~

The language does not make this operation thread-safe. If several tasks call it concurrently, protect the state or redesign the interface.

At file scope, const does not guarantee a particular physical section. Check the linker script and map file if placement in flash, RAM, or retention memory matters.

## Allocated Storage

Allocated storage is obtained and released through an allocator:

~~~c
#include <stddef.h>
#include <stdlib.h>

void *make_buffer(size_t length)
{
    return malloc(length);
}

void release_buffer(void *buffer)
{
    free(buffer);
}
~~~

The standard allocator guarantees suitable alignment for any type that can be represented by the implementation’s fundamental alignment requirements, but it does not guarantee DMA alignment, cache-line alignment, non-fragmenting behavior, bounded latency, or availability in a freestanding target.

Every successful allocation needs one clear release owner. A failed allocation returns a null pointer and must not be dereferenced.

## Thread Storage

Thread-local storage gives each thread its own instance:

~~~c
_Thread_local unsigned int error_number;
~~~

It is useful for per-thread diagnostics but may require runtime support, extra memory per thread, and a defined interaction with interrupts. Do not use it as a hidden substitute for explicit task context in an RTOS unless the runtime contract is clear.

## Tentative Definitions And Headers

At file scope, a declaration without an initializer can be a tentative definition:

~~~c
int samples_seen;
int samples_seen;
~~~

The language permits compatible tentative declarations in one translation unit, but putting such definitions in a header is a fragile design. Different compiler modes and toolchain defaults can expose duplicate-definition problems when the header is included by multiple source files.

Use an extern declaration in the header and one definition in a source file. Compile with options that diagnose common symbol and missing-definition mistakes.

## Lifetime Is Not Visibility

A pointer can outlive the name used to access its target, or the target can die while a copied pointer remains in another object:

~~~c
#include <stddef.h>

struct view {
    const int *data;
    size_t length;
};

struct view make_view(void)
{
    int local[2] = {10, 20};
    return (struct view){.data = local, .length = 2u};
}
~~~

The returned structure is a valid value, but its data pointer is dangling. Review pointer members by following the target object’s lifetime, not by looking only at the pointer variable’s scope.

## Embedded Placement

Embedded projects commonly attach section attributes or linker-script rules to objects:

~~~c
#include <stdint.h>

uint32_t boot_status __attribute__((section(".noinit")));
~~~

The attribute is a compiler extension and the section semantics come from the linker script and startup code. A no-init region may survive a warm reset, but it is not automatically valid data. Add a version, length, and integrity check before using retained contents.

For portable source, hide such declarations behind a platform header or macro and keep the language-level lifetime contract documented separately.

## Exercises

1. Split a shared counter into a public header, one external definition, and a private implementation state.
2. Return a view of caller-provided storage and write the lifetime contract.
3. Measure the stack cost of a local packet buffer and move it to a bounded pool if needed.
4. Create a local static counter, call it from two tasks, and document the race before adding synchronization.
5. Inspect a target map file and identify where static const, bss, no-init, and stack objects are placed.
6. Demonstrate the multiple-definition failure caused by defining a global in a header, then fix it with extern.

### Model Answers

Try each exercise before opening its answer. Where an exercise asks you to build
something, the answer shows one good solution, not the only one.

??? note "Exercise 1: a shared counter with private state"
    ```c
    /* event_counter.h */
    #ifndef EVENT_COUNTER_H
    #define EVENT_COUNTER_H

    #include <stdint.h>

    extern const uint32_t event_counter_limit;   /* public, read-only, defined once */

    void event_counter_add(uint32_t events);
    uint32_t event_counter_total(void);
    void event_counter_reset(void);

    #endif
    ```

    ```c
    /* event_counter.c */
    #include "event_counter.h"

    const uint32_t event_counter_limit = 1000000u;   /* the one external definition */

    static uint32_t total;                           /* private: internal linkage */
    static uint32_t saturated_adds;                  /* private diagnostic */

    void event_counter_add(uint32_t events)
    {
        if (events > event_counter_limit - total) {
            total = event_counter_limit;
            saturated_adds++;
            return;
        }
        total += events;
    }

    uint32_t event_counter_total(void)
    {
        return total;
    }

    void event_counter_reset(void)
    {
        total = 0;
        saturated_adds = 0;
    }
    ```

    The header declares what other files may use. The source file holds the one definition
    of the public object, and includes its own header so a mismatch between declaration and
    definition is a compile error. The state that can break invariants (`total`) is
    `static`, so no other file can write it directly. That is also why the public object is
    `const`: a writable `extern` counter would let any file bypass the limit.

??? note "Exercise 2: a view into caller-provided storage"
    ```c
    #include <assert.h>
    #include <stdbool.h>
    #include <stddef.h>
    #include <stdint.h>
    #include <string.h>

    struct text_view {
        const char *text;        /* points into the caller's buffer */
        size_t length;
    };

    /*
     * Formats "ID-<hex>" into buffer and returns a view of it.
     * Lifetime: the view is valid while buffer is alive and not modified. The function
     * keeps no reference to buffer after it returns.
     * Returns a view with text == NULL if capacity is too small.
     */
    struct text_view format_id(char *buffer, size_t capacity, uint16_t id)
    {
        static const char hex[] = "0123456789ABCDEF";

        if (capacity < 8u) {                         /* "ID-" + 4 digits + '\0' */
            return (struct text_view){ NULL, 0 };
        }
        memcpy(buffer, "ID-", 3);
        for (int i = 0; i < 4; i++) {
            buffer[3 + i] = hex[(id >> (12 - 4 * i)) & 0xFu];
        }
        buffer[7] = '\0';
        return (struct text_view){ buffer, 7 };
    }

    int main(void)
    {
        char buffer[16];
        struct text_view view = format_id(buffer, sizeof buffer, 0x1A2Bu);

        assert(view.text == buffer && view.length == 7 && strcmp(view.text, "ID-1A2B") == 0);
        assert(format_id(buffer, 7, 1u).text == NULL);
        return 0;
    }
    ```

    The view borrows storage whose lifetime the *caller* controls, so the function can
    return a pointer safely, unlike the page's `invalid_view` that points at its own
    automatic object. The contract states the two ways the view dies: the buffer goes out of
    scope, or the caller reuses it.

??? note "Exercise 3: the stack cost of a local packet buffer"
    ```c
    #include <stddef.h>
    #include <stdint.h>

    #define PACKET_MAX 512u

    size_t radio_receive(uint8_t *buffer, size_t capacity);
    void packet_handle(const uint8_t *packet, size_t length);

    void radio_poll(void)
    {
        uint8_t packet[PACKET_MAX];             /* 512 bytes of stack on every call */
        size_t length = radio_receive(packet, sizeof packet);

        packet_handle(packet, length);
    }
    ```

    ```text
    $ gcc -std=c17 -O2 -fstack-usage -c radio.c && cat radio.su
    radio.c:9:6:radio_poll	544	static
    ```

    544 bytes is the buffer plus 32 bytes of frame, before `packet_handle` adds its own. On
    an RTOS task with a 1 KB stack that is over half the budget in one frame. Moving the
    buffer to a bounded pool:

    ```c
    #include <stddef.h>
    #include <stdint.h>

    #define PACKET_MAX 512u
    #define PACKET_BUFFERS 2u

    size_t radio_receive(uint8_t *buffer, size_t capacity);
    void packet_handle(const uint8_t *packet, size_t length);

    /* Static storage: counted in .bss and the RAM budget, not in any task's stack. */
    static uint8_t packet_pool[PACKET_BUFFERS][PACKET_MAX];
    static uint8_t packet_in_use[PACKET_BUFFERS];

    static uint8_t *packet_acquire(void)
    {
        for (size_t i = 0; i < PACKET_BUFFERS; i++) {
            if (!packet_in_use[i]) {
                packet_in_use[i] = 1;
                return packet_pool[i];
            }
        }
        return NULL;
    }

    static void packet_release(uint8_t *packet)
    {
        packet_in_use[(size_t)(packet - packet_pool[0]) / PACKET_MAX] = 0;
    }

    void radio_poll(void)
    {
        uint8_t *packet = packet_acquire();

        if (packet == NULL) {
            return;                             /* pool exhausted: a defined, testable result */
        }
        packet_handle(packet, radio_receive(packet, PACKET_MAX));
        packet_release(packet);
    }
    ```

    Move it only when the measurement says so. A pool turns stack pressure into a capacity
    limit that callers must handle. If more than one task uses it, `packet_acquire` also
    needs a lock.

??? note "Exercise 4: a local static counter shared by two tasks"
    ```c
    #include <pthread.h>
    #include <stdio.h>

    unsigned int next_sequence(void)
    {
        static unsigned int sequence;       /* one object shared by every caller */

        return sequence++;
    }

    static void *task(void *arg)
    {
        (void)arg;
        for (int i = 0; i < 1000000; i++) {
            (void)next_sequence();
        }
        return NULL;
    }

    int main(void)
    {
        pthread_t a, b;

        pthread_create(&a, NULL, task, NULL);
        pthread_create(&b, NULL, task, NULL);
        pthread_join(a, NULL);
        pthread_join(b, NULL);
        printf("issued %u sequence numbers, expected 2000000\n", next_sequence());
        return 0;
    }
    ```

    At `-O0`, three runs printed 1078941, 1132967 and 1160756. At `-O2` the same program
    printed 2000000 every time, because GCC folded the loop into one addition, which only
    hid the race. ThreadSanitizer reports it either way:
    `WARNING: ThreadSanitizer: data race ... Read of size 4 ... in next_sequence`.

    The documentation to write *before* the fix: "`next_sequence` is not reentrant. It
    keeps one shared counter, and concurrent callers can receive duplicate numbers." The fix
    makes the counter `static _Atomic unsigned int` and returns
    `atomic_fetch_add(&sequence, 1)`. That is enough for unique numbers. Numbers that must
    also be issued in a global order alongside other state need a lock.

??? note "Exercise 5: placement of static const, bss, no-init, and stack objects"
    The [linker-script session](../compilation-linking-and-abi/linker-scripts-and-memory-layout.md#link-it-and-look)
    firmware image, extended with a `.noinit` section, a stack region, and these objects:

    ```c
    #include <stdint.h>

    uint32_t boot_count = 1u;                                     /* .data */
    const char firmware_name[] = "sensor-node";                   /* .rodata */
    static const uint16_t gain_table[4] = { 1, 2, 4, 8 };         /* .rodata */
    __attribute__((section(".noinit"))) uint32_t reset_reason;    /* .noinit */
    uint8_t sample_buffer[4096];                                  /* .bss */

    int main(void)
    {
        uint32_t local_count = boot_count;      /* automatic: stack, or a register */

        boot_count = local_count + gain_table[reset_reason & 3u];
        sample_buffer[0] = (uint8_t)firmware_name[0];
        return 0;
    }
    ```

    ```text
    $ grep -E '^\.(text|data|bss|noinit|stack) ' firmware.map
    .text           0x0000000008000000       0xdc
    .data           0x0000000020000000        0x4 load address 0x0000000008000124
    .bss            0x0000000020000020     0x1000 load address 0x0000000008000128
    .noinit         0x0000000020001020        0x4 load address 0x0000000008000128
    .stack          0x0000000020001024      0x804 load address 0x0000000008000128
    $ nm -n firmware.elf
    00000000080000c8 t gain_table
    00000000080000d0 T firmware_name
    0000000020000000 D boot_count
    0000000020000020 B sample_buffer
    0000000020001020 B reset_reason
    0000000020001828 B __stack_top__
    ```

    - `static const` data is in flash (`t`: local, read-only). It costs no RAM.
    - `.bss` is in RAM and takes no space in flash. The "load address" the map prints for
      `NOLOAD` sections is meaningless.
    - `reset_reason` is placed after `.bss`, outside `__bss_start__`..`__bss_end__`, so the
      startup code does not clear it. Its value after power-on is garbage, so it needs a
      validity marker.
    - The stack is only a reserved range (2 KB here, `__stack_top__` at `0x20001828`).
      Automatic objects such as `local_count` in `main` have no symbol: they live in that
      range, or in registers.

??? note "Exercise 6: a global defined in a header"
    [Declarations exercise 5](../language-fundamentals/declarations-and-declarators.md#model-answers)
    shows this with real output. In short, with `int retry_limit = 3;` in `config.h`,
    included by `config.c` and `main.c`:

    ```text
    /usr/bin/ld: main.o:(.data+0x0): multiple definition of `retry_limit'; config.o:(.data+0x0): first defined here
    ```

    The fix is `extern int retry_limit;` in the header and `int retry_limit = 3;` in exactly
    one source file. The include guard does not help, because it works within one translation
    unit and each `.c` file is its own. Without the initializer, GCC 10 and later still
    report the error, because `-fno-common` is the default.
    [Case study 14](../failure-case-studies/14-common-symbol-merge.md) shows what the older
    `-fcommon` behavior hides.

## Common Mistakes

- Treating block scope as if it implied short lifetime for local static objects.
- Returning a pointer to an automatic object.
- Defining global storage in a header.
- Assuming file-scope const always resides in flash.
- Assuming malloc is available, bounded, or non-fragmenting on an embedded target.
- Using a local static as hidden shared state.
- Treating retained RAM as initialized state.
- Forgetting that each thread-local instance consumes memory.
- Keeping a pointer after its target’s lifetime ends.

## Debugging Checklist

1. Identify the name’s scope and the target object’s storage duration separately.
2. Search headers for non-static definitions.
3. Inspect symbol visibility with nm or object-file tools.
4. Check the exact source file that owns external storage.
5. Draw the lifetime of every object referenced by a returned or stored pointer.
6. Inspect stack usage and interrupt stack requirements on the target.
7. Verify startup initialization and retained-memory validation.
8. Check allocator and thread-runtime guarantees before using dynamic or thread-local storage.

## Related Topics

- [Semantics And Memory overview](./index.md)
- [Pointer Fundamentals](./pointer-fundamentals.md)
- [Memory Layout And Allocation](./memory-layout-and-allocation.md)
- [Modular Design And APIs](../modular-design-and-apis/index.md)
- [Compilation, Linking, And ABI](../compilation-linking-and-abi/index.md)
- [Embedded C And Hardware](../embedded-c-and-hardware/index.md)
- [C Programming](../index.md)
- [Case study 14: Common Symbol Merge](../failure-case-studies/14-common-symbol-merge.md)

## References

- [ISO/IEC 9899 standards and drafts](https://open-std.org/jtc1/sc22/wg14/www/standards.html)
- [C11 public draft N1570](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n1570.pdf)
- [GCC variable attributes](https://gcc.gnu.org/onlinedocs/gcc/Variable-Attributes.html)
- [GCC Link Options and symbol handling](https://gcc.gnu.org/onlinedocs/gcc/Link-Options.html)

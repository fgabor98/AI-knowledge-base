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

## References

- [ISO/IEC 9899 standards and drafts](https://open-std.org/jtc1/sc22/wg14/www/standards.html)
- [C11 public draft N1570](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n1570.pdf)
- [GCC variable attributes](https://gcc.gnu.org/onlinedocs/gcc/Variable-Attributes.html)
- [GCC Link Options and symbol handling](https://gcc.gnu.org/onlinedocs/gcc/Link-Options.html)

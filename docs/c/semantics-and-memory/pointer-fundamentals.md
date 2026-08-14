---
status: draft
reviewed: false
domain: c
difficulty: intermediate
last_reviewed: null
---

# Pointer Fundamentals

A pointer is a value that can designate an object or function. It is not automatically an address that can be dereferenced: validity depends on the target object, lifetime, alignment, bounds, type, and execution context.

Pointers are the language mechanism behind arrays, output parameters, opaque handles, callbacks, memory-mapped I/O, and nearly every C interface. Treat them as typed capabilities rather than unstructured integers.

## Learning Objectives

- Use address-of and indirection correctly.
- Distinguish object pointers, function pointers, null pointers, and void pointers.
- Explain pointer-to-pointer interfaces.
- Design borrowed, owned, optional, and output-pointer contracts.
- Recognize invalid, dangling, misaligned, and improperly converted pointers.
- Connect pointer values to embedded addresses without confusing language and hardware contracts.

## Address-Of And Indirection

For a live object, unary address-of produces a pointer to that object:

~~~c
#include <stddef.h>
#include <stdint.h>

void increment(uint32_t *value)
{
    if (value != NULL) {
        ++*value;
    }
}
~~~

Read the expression *value as “the object designated by value.” The pointer must be non-null, correctly aligned for the pointed-to type, and point to an object whose lifetime is active and whose access is permitted.

The address of an array is a pointer to the whole array, not a pointer to its first element:

~~~c
#include <stddef.h>
#include <stdint.h>

void show_array_types(void)
{
    uint8_t bytes[4];
    uint8_t *element_pointer = bytes;
    uint8_t (*array_pointer)[4] = &bytes;

    (void)element_pointer;
    (void)array_pointer;
}
~~~

The two pointers may have the same numeric address while pointer arithmetic and type compatibility differ.

## Null Pointers

A null pointer is a pointer value that compares unequal to a pointer to any object or function. The null pointer constant used in source may be 0, an integer constant expression with value zero, or the implementation’s null pointer macro:

~~~c
#include <stddef.h>

void use_optional(int *value)
{
    if (value == NULL) {
        return;
    }

    *value = 42;
}
~~~

A null pointer is not guaranteed to have an all-bits-zero representation. Do not create one with memset or by assuming a hardware zero address has the same language meaning.

Checking for null is useful only when the pointer is itself a valid pointer value to inspect. An arbitrary integer or corrupted bit pattern is not repaired by comparing it with NULL.

## Object Pointer Types

Pointer type affects dereference interpretation and arithmetic:

~~~c
#include <stdint.h>
#include <stddef.h>

void write_words(uint32_t *words, size_t count)
{
    for (size_t i = 0u; i < count; ++i) {
        words[i] = 0u;
    }
}
~~~

A uint32_t pointer advances by the size of uint32_t, not one byte. Convert to an unsigned-character pointer for byte inspection only when the lifetime, bounds, and representation rules permit it.

A pointer to const data prevents modification through that access path:

~~~c
void inspect_bytes(const uint8_t *data, size_t length);
~~~

It does not prove that no other alias can modify the object, and it does not transfer ownership.

## void Pointers

A void pointer can hold a pointer to an object type and can be converted back to a compatible object pointer:

~~~c
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

void demonstrate_void_pointer(void)
{
    void *storage = NULL;
    uint32_t value = 7u;

    storage = &value;
    uint32_t *value_pointer = storage;
    (void)value_pointer;
}
~~~

In C, conversion between void pointer and object pointer is implicit. A void pointer has no element type for arithmetic or dereference. Never use it as a generic escape from bounds or lifetime checks.

The standard allocator returns void pointer storage that can be converted to a pointer to a suitably aligned object. The allocation still needs a size, owner, initialization policy, and release path.

## Pointer-to-Pointer Interfaces

A pointer-to-pointer is used when a function must change the caller’s pointer:

~~~c
#include <stddef.h>
#include <stdlib.h>

int allocate_integer(int **result)
{
    if (result == NULL) {
        return -1;
    }

    *result = malloc(sizeof **result);
    if (*result == NULL) {
        return -2;
    }

    **result = 0;
    return 0;
}
~~~

Decide what happens on failure. This example leaves result unchanged only when result itself is null; a stronger API can set *result to NULL before attempting allocation or use a temporary pointer and commit only on success.

A double pointer can also describe a pointer into a caller-owned data structure, a cursor that a parser advances, or a linked-list insertion point. Name that meaning; pointer depth alone does not explain ownership.

## Function Pointers

Function pointers designate functions, not data:

~~~c
#include <stddef.h>
#include <stdint.h>

typedef void (*event_handler)(void *context, uint32_t event);

void dispatch(event_handler handler, void *context, uint32_t event)
{
    if (handler != NULL) {
        handler(context, event);
    }
}
~~~

Object pointers and function pointers are different categories. Do not convert between them. A callback’s parameter and return types must be compatible; casting an incompatible function pointer can break the ABI.

## Ownership And Borrowing

A pointer contract should classify the relationship:

| Kind | Meaning | Typical rule |
| --- | --- | --- |
| Borrowed input | Callee reads storage temporarily | Caller keeps it alive and unchanged as required |
| Borrowed mutable | Callee may modify but not retain | Caller keeps storage alive and accepts writes |
| Output pointer | Callee writes a result | Caller supplies valid writable storage |
| Owned pointer | Callee owns and must release | The owner is explicit in the API |
| Transferred pointer | Ownership moves at a defined boundary | Old owner must not use or release it afterward |
| Optional pointer | Null has documented meaning | Null is checked before access |

Use names, comments, attributes, and types consistently. A typedef such as handle_t can hide whether the handle is an object pointer, an integer token, or an opaque resource; document it.

## Pointer Representations And Integer Conversions

Some platforms provide uintptr_t for converting an object pointer to an integer and back, if the type exists:

~~~c
#include <stdint.h>

uintptr_t pointer_token(const void *pointer)
{
    return (uintptr_t)pointer;
}
~~~

This does not make the integer dereferenceable, preserve provenance in every implementation model, or make the value portable across processes, boots, or address spaces. Use integer tokens only for a documented ABI, logging, table indexing, or platform interface.

Do not use arithmetic on an integer token as a substitute for pointer arithmetic. Convert back only to the appropriate pointer type and verify the platform contract.

## Embedded Addresses

A hardware address is not automatically a valid C object. A vendor or platform implementation defines how an address is exposed:

~~~c
#include <stdint.h>

#define TIMER_STATUS_ADDRESS 0x40000000u

static volatile uint32_t *const timer_status =
    (volatile uint32_t *)TIMER_STATUS_ADDRESS;
~~~

The address, access width, volatile requirement, alignment, reset state, and read/write side effects come from the reference manual and toolchain ABI. The cast itself does not prove that the address is mapped or that the access is atomic.

Prefer vendor headers or a reviewed hardware abstraction. Keep target-specific address constants out of portable modules.

## Exercises

1. Implement an output-parameter function that leaves the output unchanged on failure.
2. Write a borrowed-buffer API with pointer, length, mutability, and lifetime documented.
3. Build a small linked-list insertion function using a pointer-to-pointer and test empty and non-empty lists.
4. Register callbacks with context objects and test callback lifetime during shutdown.
5. Compare object-pointer and function-pointer diagnostics by attempting an invalid conversion.
6. Inspect a vendor register declaration and document every assumption beyond the C pointer type.

## Common Mistakes

- Dereferencing without checking lifetime, alignment, bounds, and access permission.
- Treating a pointer as an integer or a numeric address.
- Assuming null is all-bits-zero.
- Using void pointers to hide missing size or ownership information.
- Returning pointers to local objects.
- Retaining a borrowed pointer after the documented call boundary.
- Casting incompatible function pointers.
- Treating volatile hardware addresses as ordinary RAM.
- Passing an uninitialized pointer-to-pointer output.
- Freeing storage while another pointer still designates it.

## Debugging Checklist

1. Print pointer values only for diagnostics; do not infer validity from a pleasing address.
2. Record the designated object, its lifetime, and its bounds.
3. Check null, alignment, and ownership at API boundaries.
4. Use AddressSanitizer and UndefinedBehaviorSanitizer on host-representable code.
5. Inspect pointer type and conversion warnings.
6. For callbacks, record registration, context, and deregistration order.
7. For MMIO, compare generated access width and ordering with the hardware manual.
8. Replace pointer arithmetic on addresses with typed objects or explicit platform APIs.

## Related Topics

- [Semantics And Memory overview](./index.md)
- [Storage Duration, Scope, And Linkage](./storage-duration-scope-and-linkage.md)
- [Pointer Arithmetic And Bounds](./pointer-arithmetic-and-bounds.md)
- [Memory Safety And Lifetime](./memory-safety-and-lifetime.md)
- [Embedded C And Hardware](../embedded-c-and-hardware/index.md)
- [C Programming](../index.md)

## References

- [ISO/IEC 9899 standards and drafts](https://open-std.org/jtc1/sc22/wg14/www/standards.html)
- [C11 public draft N1570](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n1570.pdf)
- [GCC pointer conversion and optimization documentation](https://gcc.gnu.org/onlinedocs/gcc/Optimize-Options.html)
- [CERT C pointer rules](https://wiki.sei.cmu.edu/confluence/display/c)

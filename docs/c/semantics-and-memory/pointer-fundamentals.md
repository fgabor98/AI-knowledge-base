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
#include <stddef.h>
#include <stdint.h>

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

### Model Answers

Try each exercise before opening its answer. Where an exercise asks you to build
something, the answer shows one good solution, not the only one.

??? note "Exercise 1: an output parameter left unchanged on failure"
    ```c
    #include <assert.h>
    #include <stdbool.h>
    #include <stddef.h>
    #include <stdint.h>

    /* Parses a decimal 0..65535. On failure *value is not written. */
    bool parse_u16(const char *text, uint16_t *value)
    {
        uint32_t result = 0;

        if (text == NULL || value == NULL || *text == '\0') {
            return false;
        }
        for (; *text != '\0'; text++) {
            if (*text < '0' || *text > '9') {
                return false;
            }
            result = result * 10u + (uint32_t)(*text - '0');
            if (result > UINT16_MAX) {
                return false;
            }
        }
        *value = (uint16_t)result;          /* the only write, after every check */
        return true;
    }

    int main(void)
    {
        uint16_t value = 1234;

        assert(parse_u16("65535", &value) && value == 65535);
        assert(!parse_u16("65536", &value) && value == 65535);
        assert(!parse_u16("12x", &value) && value == 65535);
        assert(!parse_u16("", &value) && value == 65535);
        return 0;
    }
    ```

    The result is built in a local and written through the pointer once, at the end. Every
    failure test checks that the old value survived. Writing `*value` digit by digit would
    leave half a number behind on `"12x"`.

??? note "Exercise 2: a documented borrowed-buffer API"
    ```c
    #include <stddef.h>
    #include <stdint.h>

    /*
     * Computes a CRC over data[0..length).
     *
     * data:      read-only, borrowed for the duration of the call only; not retained.
     *            May be NULL only when length == 0.
     * length:    number of bytes; 0 is valid and returns the initial value.
     * Lifetime:  the caller may free or reuse data as soon as the call returns.
     * Context:   reentrant; no shared state; safe from interrupts.
     */
    uint16_t crc16_compute(const uint8_t *data, size_t length);

    /*
     * Starts an asynchronous transmit.
     *
     * data:      read-only for this module, but BORROWED UNTIL COMPLETION: the caller
     *            must keep it alive and unmodified until on_done runs.
     * length:    1..UART_TX_MAX bytes.
     * Returns 0 if started; the buffer is then owned by the driver until on_done.
     */
    int uart_send_async(const uint8_t *data, size_t length, void (*on_done)(void *), void *context);
    ```

    The two declarations look alike, but their lifetime contracts differ completely. Only the
    comment says that `uart_send_async` keeps the pointer after it returns. `const` says the
    module will not write the buffer. It does not say how long the borrow lasts.

??? note "Exercise 3: linked-list insertion with a pointer to a pointer"
    ```c
    #include <assert.h>
    #include <stddef.h>

    struct node {
        int key;
        struct node *next;
    };

    /* Inserts node in ascending key order, after any equal keys.
       The list head needs no special case. */
    void list_insert_sorted(struct node **link, struct node *node)
    {
        while (*link != NULL && (*link)->key <= node->key) {
            link = &(*link)->next;              /* the pointer that will point at node */
        }
        node->next = *link;
        *link = node;
    }

    int main(void)
    {
        struct node a = { 2, NULL }, b = { 1, NULL }, c = { 3, NULL }, d = { 2, NULL };
        struct node *head = NULL;

        list_insert_sorted(&head, &a);          /* empty list */
        assert(head == &a && a.next == NULL);
        list_insert_sorted(&head, &b);          /* new head */
        list_insert_sorted(&head, &c);          /* new tail */
        list_insert_sorted(&head, &d);          /* middle; equal keys keep insertion order */
        assert(head == &b && b.next == &a && a.next == &d && d.next == &c && c.next == NULL);
        return 0;
    }
    ```

    `link` always points at the pointer to change: `head` itself, or some node's `next`.
    Inserting at the head is therefore the same operation as inserting anywhere else. With a
    plain `struct node *` the function would need a separate "is it the first node?"
    branch and would have to return the new head.

??? note "Exercise 4: callback lifetime during shutdown"
    [Functions exercise 3](../language-fundamentals/functions.md#model-answers) builds the
    fake driver and shows ASan catching a dead context. During shutdown, the order of
    teardown is what matters:

    ```c
    #include <stdlib.h>

    typedef void (*timer_callback)(void *context);

    int timer_register(timer_callback callback, void *context);
    void timer_unregister(void);    /* returns only after any running callback finished */

    struct sampler {
        unsigned int samples;
    };

    static void sampler_tick(void *context)
    {
        struct sampler *sampler = context;

        sampler->samples++;
    }

    void sampler_stop(struct sampler *sampler)
    {
        timer_unregister();         /* 1: no new calls, and none still running */
        free(sampler);              /* 2: only now may the context die */
    }
    ```

    The test: register a heap context, call `sampler_stop`, fire the timer, and check under
    ASan that nothing touches the freed context. A single-threaded test cannot show why the
    order matters. Freeing first is only wrong when the callback runs *between* the two
    lines, from an interrupt or another thread. So the real requirement is in
    `timer_unregister`: it must disable the source and wait for an in-flight call before it
    returns. Test that with a thread that fires the timer continuously while another thread
    stops the sampler, under TSan and ASan.

??? note "Exercise 5: object-pointer and function-pointer diagnostics"
    <!-- c-check: expect-warning -->

    ```c
    #include <stdint.h>

    typedef void (*event_handler)(void *context, uint32_t event);

    void on_event(void *context, uint32_t event);
    static uint32_t counter;

    void conversions(void)
    {
        void *object = on_event;                     /* function -> object pointer */
        event_handler handler = &counter;            /* object -> function pointer */
        uint32_t *words = on_event;                  /* function -> unrelated object type */
        (void)object; (void)handler; (void)words;
    }
    ```

    GCC 13 with `-Wall -Wextra` (types after `aka` trimmed):

    ```text
    fp.c:11:29: warning: initialization of 'event_handler' from incompatible pointer type 'uint32_t *' [-Wincompatible-pointer-types]
    fp.c:12:23: warning: initialization of 'uint32_t *' from incompatible pointer type 'void (*)(void *, uint32_t)' [-Wincompatible-pointer-types]
    ```

    Line 10, a function pointer stored in `void *`, gets **no warning** at `-Wall -Wextra`.
    Only `-Wpedantic` reports it: `ISO C forbids initialization between function pointer
    and 'void *'`. ISO C does not define converting between function and object pointers.
    POSIX requires it to work (`dlsym` depends on it), and on Harvard-architecture targets
    such as AVR, code and data pointers are not interchangeable. Add `-Wpedantic`, or
    store callbacks in a function-pointer type such as `void (*)(void)`, which can be
    converted to and from any other function-pointer type.

??? note "Exercise 6: assumptions behind a vendor register declaration"
    A typical vendor (CMSIS-style) declaration:

    ```c
    #include <stdint.h>

    #define __IO volatile

    typedef struct {
        __IO uint32_t CR1;     /* 0x00 */
        __IO uint32_t CR2;     /* 0x04 */
        __IO uint32_t SR;      /* 0x08 */
        __IO uint32_t DR;      /* 0x0C */
    } UART_TypeDef;

    #define UART1_BASE 0x40011000u
    #define UART1 ((UART_TypeDef *)UART1_BASE)
    ```

    What the C type does not say, and what to write down:

    | Assumption | Where it comes from |
    | --- | --- |
    | The structure has no padding, so `DR` is at offset 0x0C | the ABI: four `uint32_t` are never padded; add `_Static_assert(offsetof(UART_TypeDef, DR) == 0x0C)` |
    | Every access is a 32-bit load or store | `volatile uint32_t` makes GCC emit word accesses, but the standard does not promise it |
    | `UART1_BASE` is the right address for this part number | the reference manual's memory map |
    | Reading `SR` or `DR` has side effects (clears flags, pops the FIFO) | the register description; `volatile` only keeps the read from being removed |
    | Bits marked reserved must be written back as read | the register description; plain `UART1->CR1 = x` breaks it |
    | The peripheral clock is enabled before the first access | the clock-tree chapter; otherwise accesses may fault or read 0 |
    | Ordering with DMA or other bus masters needs barriers | the core's memory model; `volatile` orders only volatile accesses |

    The pointer type covers the first two rows at best. The remaining rows are the reasons
    driver code still needs review against the manual.

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

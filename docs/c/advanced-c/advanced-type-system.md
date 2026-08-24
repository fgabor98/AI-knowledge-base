---
status: draft
reviewed: false
domain: c
difficulty: advanced
last_reviewed: null
---

# Advanced Type System

C's type system is modest, but its less-common features can express useful contracts
for embedded interfaces: alignment, object size, atomicity, optional fields, generic
dispatch, compile-time configuration, and storage layout. These features do not prove
everything automatically. They help move assumptions from comments into declarations,
diagnostics, and tests.

## Learning Objectives

- Use `_Generic`, `_Static_assert`, `_Alignas`, `_Alignof`, and `_Atomic` with a clear
  portability and semantic contract.
- Choose between compound literals, designated initializers, VLAs, and flexible array
  members based on lifetime, stack, ABI, and allocation behavior.
- Recognize where anonymous structs/unions and compiler extensions affect portability.
- Build type-directed APIs without pretending that C has compile-time reflection or
  type-safe overloads.
- Validate layout, alignment, atomicity, and size assumptions at compile time.

## `_Static_assert`: Make Invariants Build-Time Failures

`_Static_assert` checks an integer constant expression during translation. It is useful
for properties that must never vary silently between targets:

```c
#include <stdint.h>

struct wire_header {
    uint16_t version;
    uint16_t length;
    uint32_t sequence;
};

_Static_assert(sizeof(uint16_t) == 2, "uint16_t is required by this format");
_Static_assert(sizeof(struct wire_header) >= 8, "header unexpectedly shrank");
_Static_assert(_Alignof(struct wire_header) <= 4,
               "header alignment is unsuitable for the transport");
```

The assertion does not guarantee that the structure is a wire format. Padding, endian
order, compiler packing, and integer representation still need explicit handling. Use
it to fail fast on a violated prerequisite, then serialize fields by width.

In C11 and later, the keyword is `_Static_assert`; newer dialects also permit the
shorter `static_assert` spelling when the appropriate header or language version
provides it. If a project supports older dialects, wrap the spelling in a compatibility
macro rather than scattering compiler-specific alternatives.

## `_Alignof` And `_Alignas`

`_Alignof(T)` yields the required alignment of a type. `_Alignas(T)` or `_Alignas(n)`
requests alignment for an object or member where the implementation supports the
requested value. Alignment can matter for:

- atomic lock-free access;
- DMA descriptors and cache-line ownership;
- SIMD loads and stores;
- ABI-required stack or aggregate alignment;
- memory-mapped controller blocks;
- avoiding false sharing between cores.

Alignment is not size, and alignment does not establish cacheability or DMA reachability.
An over-aligned object may also create a linker or allocator requirement. The storage
provider must honor it: an aligned declaration inside a packed structure is not a safe
way to obtain aligned storage, and a custom allocator must return pointers satisfying
the requested alignment.

Use `alignof`/`alignas` from `<stdalign.h>` when the project wants the non-underscore
convenience macros, but confirm the language mode and library implementation. Check
the result with `_Static_assert` for fixed layouts and runtime assertions for dynamic
allocators.

## `_Generic`: Type-Directed Selection

`_Generic` selects an expression based on its type at compile time. It does not
evaluate the controlling expression, but the selected expression can contain a call
that is evaluated normally. Qualifiers, lvalue conversions, compatible types, and
default selection are subtle; design a small macro and test every supported type.

```c
#include <stddef.h>

static size_t count_ints(const int *value)
{
    (void)value;
    return sizeof(int);
}

static size_t count_bytes(const unsigned char *value)
{
    (void)value;
    return sizeof(unsigned char);
}

#define element_width(value) _Generic((value), \
    int *: count_ints, \
    const int *: count_ints, \
    unsigned char *: count_bytes, \
    const unsigned char *: count_bytes \
)(value)
```

This pattern is most useful for type-safe convenience APIs where the alternatives have
the same semantic contract. It becomes dangerous when macros evaluate arguments more
than once, choose a surprising compatible type, or hide a conversion. Prefer a static
inline function when one type is enough; use `_Generic` for a deliberately small set of
overloads and add compile tests for unsupported types.

## `_Atomic` And Atomic Qualifiers

`_Atomic(T)` is an atomic type specifier; `_Atomic` as a qualifier can qualify a type.
The `<stdatomic.h>` typedefs such as `atomic_uint` and functions/macros such as
`atomic_load_explicit` are usually easier to read. Atomicity is a property of the
object and access protocol, not of a cast performed at one call site.

```c
#include <stdatomic.h>

struct counters {
    atomic_uint submitted;
    atomic_uint completed;
};

static void record_submission(struct counters *counters)
{
    (void)atomic_fetch_add_explicit(&counters->submitted, 1u,
                                    memory_order_relaxed);
}
```

The relaxed order is appropriate only when the counter is independent telemetry. If a
counter publishes accompanying data, use an acquire/release protocol and document it.
Do not copy atomic objects with `memcpy` or assign them as if they were ordinary
structures unless the implementation and API explicitly permit it. Query
`atomic_is_lock_free` when a no-lock property is a product requirement.

## `_Thread_local`

`_Thread_local` gives an object thread storage duration: each thread has a distinct
instance, initialized according to the implementation's startup rules and destroyed
according to the runtime's rules. It can simplify per-thread error state, scratch
storage, and recursion guards, but it is not automatically available in every
freestanding runtime or interrupt context.

Consider:

- startup cost and TLS block layout;
- whether an ISR runs on the same execution context as a task;
- whether a thread can be created after the image starts;
- destructor or teardown behavior in a hosted runtime;
- whether a pointer to one thread's instance can escape to another thread.

For an RTOS, use the RTOS's thread-local facility when its scheduler and task model do
not match the toolchain TLS implementation. For a bare-metal system, a per-core array
indexed by a validated core ID may be more explicit and easier to place.

## Compound Literals

A compound literal creates an unnamed object with the type and initializer specified.
At block scope its lifetime ends at the end of the enclosing block; at file scope it
has static storage duration. It is useful for temporary configuration values and
structured call arguments:

```c
struct limits {
    unsigned int minimum;
    unsigned int maximum;
};

static int within_limits(unsigned int value, struct limits limits)
{
    return value >= limits.minimum && value <= limits.maximum;
}

static int valid_sample(unsigned int value)
{
    return within_limits(value, (struct limits){
        .minimum = 10u,
        .maximum = 1000u,
    });
}
```

Never return a pointer to a block-scope compound literal. Be explicit when a pointer
to the temporary is passed to an asynchronous operation, stored by a callback, or used
after the full expression; the object may no longer exist. A compound literal is not a
heap allocation and has no independent ownership operation.

## Designated Initializers

Designated initializers tie values to member or index names rather than positional
order. They reduce breakage when a structure grows and make sparse lookup tables clear.
Unspecified members are initialized as if they had static zero initialization for the
relevant initializer form.

```c
enum device_state { DEVICE_OFF, DEVICE_READY, DEVICE_ERROR };

struct state_name {
    const char *text;
    unsigned int retry_limit;
};

static const struct state_name names[] = {
    [DEVICE_OFF] = { .text = "off", .retry_limit = 0u },
    [DEVICE_READY] = { .text = "ready", .retry_limit = 3u },
    [DEVICE_ERROR] = { .text = "error", .retry_limit = 1u },
};
```

For protocol or ABI-visible structures, named initializers improve readability but do
not solve padding or versioning. For arrays indexed by external values, validate the
index before accessing the table; a designated initializer does not constrain runtime
indices.

## Variable-Length Arrays

A variable-length array (VLA) has a runtime-bound size and automatic storage duration.
It can express a two-dimensional view with a runtime stride and avoid a heap allocation,
but its size is not a constant expression and a large or untrusted bound can exhaust a
task stack. Optional VLA support and implementation limits also matter for freestanding
targets.

```c
#include <stddef.h>

static int matrix_sum(size_t rows, size_t columns,
                      const int matrix[static 1][columns], int *result)
{
    int sum = 0;
    if (result == NULL || columns == 0u) {
        return -1;
    }
    for (size_t row = 0u; row < rows; ++row) {
        for (size_t column = 0u; column < columns; ++column) {
            sum += matrix[row][column];
        }
    }
    *result = sum;
    return 0;
}
```

The `static 1` array parameter contract tells the caller that the pointer is non-null
and points to at least one element for each adjusted dimension. It is a precondition,
not a runtime check. In production embedded code, a checked span structure or caller-
provided scratch buffer is often easier to bound than a VLA. Never use a VLA for a
length that came from an unvalidated packet or device register.

## Flexible Array Members

A flexible array member (FAM) is the final member of a structure and contributes no
fixed element count to `sizeof`. The complete allocation must include both the fixed
header and the requested tail, with overflow checked before allocation. Use
`offsetof`/`sizeof` carefully: the offset of the flexible member may include tail
padding, and the structure's alignment still applies.

```c
#include <stddef.h>
#include <stdint.h>

struct message {
    uint16_t kind;
    uint16_t length;
    unsigned char payload[];
};

static int message_storage_size(size_t payload_size, size_t *result)
{
    const size_t header_size = offsetof(struct message, payload);
    if (result == NULL || payload_size > SIZE_MAX - header_size) {
        return -1;
    }
    *result = header_size + payload_size;
    return 0;
}
```

The allocation can be one object, but the lifetime and deallocation mechanism must be
defined. Do not copy a FAM object with a plain assignment and expect the tail to copy;
copy the fixed header and the validated payload separately. Do not form a pointer one
past the allocated tail and then read from it. A FAM is a representation technique,
not an automatic bounds-checking facility.

## Anonymous Structures And Unions

Anonymous structures/unions allow promoted member access and are supported by common
C11 implementations, but portability and diagnostics vary across older compilers and
strict freestanding environments. They can make register definitions or tagged variant
representations readable, yet they may obscure which storage overlaps and which member
is active.

Use named nested members when the type crosses a stable ABI or is consumed by multiple
toolchains. If an anonymous union represents a tagged variant, keep the tag next to it
and enforce the active-member rule in constructors/accessors. Never assume reading an
inactive union member is portable serialization or numerical conversion; use `memcpy`
or explicit conversion when representation inspection is the goal.

## Generic Programming Patterns In C

Useful patterns include:

- macros for small, type-checked dispatch through `_Generic`;
- `static inline` functions for type-safe single-type operations;
- opaque handles for hiding representation and ownership;
- X-macros for one source of truth when generating enums, tables, and strings;
- callback tables for polymorphic operations with explicit context pointers;
- tagged unions for closed variants;
- `_Static_assert` and `offsetof` checks for layout contracts;
- explicit span types such as `{ pointer, length }` instead of naked array pointers.

Avoid macros that evaluate an argument twice, depend on statement-expression extensions,
capture identifiers unexpectedly, or expose an internal type layout. A generic API is
successful when the caller can understand its ownership, failure, and type rules from
the declaration and not from macro expansion archaeology.

## Exercises And Diagnostics

1. Build a `span` type with pointer, length, and element-size invariants; test empty,
   null, overflow, and subspan operations.
2. Create a `_Generic` logging API for signed, unsigned, pointer, and string values;
   add compile tests for qualifiers and unsupported types.
3. Implement a FAM message constructor with checked size arithmetic and a matching
   destructor; fuzz the requested payload size near `SIZE_MAX`.
4. Compare a VLA-based matrix view with a caller-provided scratch-buffer API and measure
   stack usage on the target.
5. Add `_Static_assert` checks for a DMA descriptor, then build with every supported
   ABI and compiler to verify the layout contract.

## Common Mistakes

- Treating `_Generic` as a full overload system while ignoring qualifiers and compatible
  types.
- Assuming `_Static_assert` proves a wire layout without checking padding and endian
  order.
- Requesting alignment that the allocator, linker, or DMA fabric does not honor.
- Returning or storing a pointer to a compound literal whose lifetime has ended.
- Using VLAs with untrusted sizes or in tasks with small stacks.
- Adding to a flexible array member with unchecked `header + length` arithmetic.
- Copying a FAM object with structure assignment and losing the tail payload.
- Hiding ownership and lifetime in macro-generated generic interfaces.

## Related Topics

- [Advanced C overview](./index.md)
- [Types, Values, And Objects](../language-fundamentals/types-values-and-objects.md)
- [Structures, Unions, And Enumerations](../language-fundamentals/structures-unions-and-enums.md)
- [Object Representation, Alignment, And Padding](../semantics-and-memory/object-representation-alignment-and-padding.md)
- [C Memory Model And Concurrency](./c-memory-model-and-concurrency.md)
- [Modular Design And APIs](../modular-design-and-apis/index.md)

## References

- [WG14 C working documents](https://www.open-std.org/jtc1/sc22/wg14/www/projects.html)
- [C11 draft N1570](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n1570.pdf)
- [GCC C language extensions](https://gcc.gnu.org/onlinedocs/gcc/Extensions-to-the-C-Language-Family.html)
- [Clang language extensions](https://clang.llvm.org/docs/LanguageExtensions.html)
- The exact C standard edition and compiler documentation for supported dialects

---
status: draft
reviewed: false
domain: c
difficulty: beginner
last_reviewed: null
---

# Declarations And Declarators

C declarations combine declaration specifiers with a declarator. The specifiers describe the base type, storage, linkage, and qualifiers; the declarator describes how an identifier relates to that type. A small punctuation change can turn pointer-to-function into function-returning-pointer.

Learning to parse declarations mechanically is more useful than memorizing isolated examples.

## Learning Objectives

- Separate declaration specifiers from the declarator.
- Parse pointers, arrays, and function declarators.
- Distinguish an array of pointers from a pointer to an array.
- Use const and volatile in the intended position.
- Use typedef, extern, static, and incomplete types appropriately.
- Design header declarations that expose a stable interface.

## The Two-Part Model

Start with declaration specifiers:

~~~c
static const unsigned long
~~~

Then read the declarator:

~~~c
*volatile register_pointer
~~~

Together, at file scope, this declares register_pointer as an internally linked volatile pointer to const unsigned long. In the declarator, volatile qualifies the pointer; const in the specifiers qualifies the pointed-to base type.

A simpler set:

~~~c
const uint32_t *read_only_data;
uint32_t *const fixed_address = (uint32_t *)0x40000000u;
const uint32_t *const fixed_read_only_address =
    (const uint32_t *)0x40000000u;
~~~

- read_only_data is a modifiable pointer to const uint32_t.
- fixed_address is a const pointer to modifiable uint32_t.
- fixed_read_only_address is a const pointer to const uint32_t.

The address is target-specific and shown only to illustrate declaration reading.

## Read From The Identifier Outward

Start at the identifier and move outward, respecting parentheses:

~~~c
int *table[4];
int (*matrix)[4];
int *make_value(void);
int (*operation)(int, int);
~~~

- table is an array of four pointers to int.
- matrix is a pointer to an array of four int.
- make_value is a function taking no arguments and returning a pointer to int.
- operation is a pointer to a function taking two int arguments and returning int.

Without parentheses, int *matrix[4] is an array of pointers, and int *operation(int, int) is a function returning a pointer. Break declarations into typedefs or smaller interfaces when parsing is difficult.

## Pointers, Arrays, And Functions

~~~c
#include <stddef.h>
#include <stdint.h>

void write_bytes(uint8_t *data, size_t length);
const uint8_t *find_sync(const uint8_t *data, size_t length);
uint8_t *allocate_bytes(size_t length);
void (*set_handler(void (*handler)(void)))(void);
~~~

The last declaration is legal but difficult to read. A typedef clarifies the callback:

~~~c
typedef void (*handler_fn)(void);

handler_fn set_handler(handler_fn handler);
~~~

Typedefs create aliases, not new runtime types. Use them when they clarify opaque handles, callbacks, and domain concepts. Do not use them to hide ownership or pointer depth.

## Function Prototypes

A function prototype specifies parameter types:

~~~c
int send_frame(const uint8_t *data, size_t length);
int reset_device(void);
~~~

Parameter names are useful in headers:

~~~c
int copy_frame(uint8_t *destination,
               size_t destination_capacity,
               const uint8_t *source,
               size_t source_length);
~~~

An empty parameter list is not a prototype:

~~~c
int legacy_function();
int modern_function(void);
~~~

Function types must be compatible across declarations and definitions. Include the owning header from the defining source file so mismatches are diagnosed.

## Qualifiers

The common qualifiers are:

- const: access through this lvalue cannot modify the referred object;
- volatile: accesses through this lvalue are observable and cannot be treated as ordinary dead accesses;
- restrict: an aliasing promise used to enable optimization;
- _Atomic: atomic access and operation semantics for a type.

Placement changes what is qualified:

~~~c
const int *a;
int const *b;
int *const c = 0;
const int *const d = 0;
~~~

const is not an ownership or thread-safety guarantee. volatile is not a lock, memory barrier, or atomicity guarantee. restrict is a promise the caller must keep; violating it can make otherwise plausible code undefined.

## Storage, Scope, And Linkage

At file scope:

~~~c
extern int shared_counter;
static int private_counter;
~~~

A definition in one source file might be:

~~~c
int shared_counter;
~~~

Use extern for declarations referring to objects or functions defined elsewhere. Use file-scope static for implementation-private names. Avoid defining non-static objects in headers; multiple translation units can then create duplicate or fragile definitions.

A local static has block scope but static storage duration:

~~~c
unsigned int next_sequence(void)
{
    static unsigned int sequence;
    return sequence++;
}
~~~

It persists between calls and is shared state, so the function is not automatically reentrant or thread-safe.

## Incomplete Types

An incomplete type has identity but not a complete size:

~~~c
struct device;
void device_start(struct device *device);
~~~

A source file can complete it:

~~~c
struct device {
    int state;
    unsigned int flags;
};
~~~

Pointers to incomplete types are useful for opaque modules. You cannot define an object of incomplete type or apply sizeof until the type is complete. An array can also be incomplete until a bound is supplied:

~~~c
extern uint8_t receive_buffer[];
~~~

## Compatible Types And Redeclarations

Declarations in different translation units must be compatible. Compatibility includes parameter types, qualifiers in relevant positions, tags, and return types.

~~~c
int read_value(void);
long read_value(void);
~~~

A mismatch between the prototype seen by a caller and the definition can corrupt arguments or return values at the ABI boundary even if the source is small.

## Header Pattern

A practical interface header contains guards, includes, public types, and declarations:

~~~c
#ifndef COUNTER_H
#define COUNTER_H

#include <stdint.h>

struct counter;

struct counter *counter_create(uint32_t initial);
void counter_destroy(struct counter *counter);
uint32_t counter_get(const struct counter *counter);
void counter_increment(struct counter *counter);

#endif
~~~

The opaque struct keeps representation private. The API documentation must still describe allocation, ownership, thread safety, and errors.

## Exercises

1. Parse ten declarations into plain English before looking at compiler diagnostics.
2. Rewrite an array-of-pointers and pointer-to-array declaration using typedefs.
3. Design an opaque driver handle with create, destroy, and operation functions.
4. Change a writable-data parameter to pointer-to-const and identify safer call sites.
5. Put a global definition in a header in a two-file program, observe the link failure, then use extern plus one definition.
6. Register callbacks and deliberately introduce an incompatible callback to see the warning.

## Common Mistakes

- Reading int *a[4] as a pointer to an array.
- Omitting parentheses around pointer-to-function or pointer-to-array declarators.
- Qualifying the pointer when the data should be read-only, or vice versa.
- Treating volatile as synchronization.
- Hiding ownership and pointer depth behind typedefs.
- Defining global storage in a header.
- Using an incomplete type where a complete object or sizeof is required.
- Declaring a no-argument function with empty parentheses.
- Assuming matching sizes imply compatible types.

## Debugging Checklist

1. Copy a declaration into a minimal file and ask the compiler for warnings.
2. Split complex declarators into typedefs and compare resulting types.
3. Check the exact header seen by every translation unit.
4. Search for duplicate definitions and missing extern declarations.
5. Inspect qualifiers on both pointer and pointed-to object.
6. Check incomplete-type uses requiring size.
7. Verify callback compatibility without function-pointer casts.
8. If an ABI symptom appears, inspect argument and return types in assembly.

## Related Topics

- [Language Fundamentals overview](./index.md)
- [Functions](./functions.md)
- [Compilation, Linking, And ABI](../compilation-linking-and-abi/index.md)
- [Modular Design And APIs](../modular-design-and-apis/index.md)
- [Semantics And Memory Model](../semantics-and-memory/index.md)
- [C Programming](../index.md)

## References

- [ISO/IEC 9899 standards and drafts](https://open-std.org/jtc1/sc22/wg14/www/standards.html)
- [C11 public draft N1570](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n1570.pdf)
- [GCC variable attributes and declaration syntax](https://gcc.gnu.org/onlinedocs/gcc/Variable-Attributes.html)
- [Clang attribute reference](https://clang.llvm.org/docs/AttributeReference.html)

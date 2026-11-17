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

<!-- c-check: skip -- partial declaration, read piece by piece -->

~~~c
static const unsigned long
~~~

Then read the declarator:

<!-- c-check: skip -- partial declaration, read piece by piece -->

~~~c
*volatile register_pointer
~~~

Together, at file scope, this declares register_pointer as an internally linked volatile pointer to const unsigned long. In the declarator, volatile qualifies the pointer; const in the specifiers qualifies the pointed-to base type.

A simpler set:

~~~c
#include <stdint.h>

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
#include <stddef.h>
#include <stdint.h>

int send_frame(const uint8_t *data, size_t length);
int reset_device(void);
~~~

Parameter names are useful in headers:

~~~c
#include <stddef.h>
#include <stdint.h>

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
#include <stdint.h>

extern uint8_t receive_buffer[];
~~~

## Compatible Types And Redeclarations

Declarations in different translation units must be compatible. Compatibility includes parameter types, qualifiers in relevant positions, tags, and return types.

<!-- c-check: expect-error -->

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

### Model Answers

Try each exercise before opening its answer. Where an exercise asks you to build
something, the answer shows one good solution, not the only one.

??? note "Exercise 1: worked examples"
    Start at the identifier, then read right while you can (`[]` and `()`), and left when you
    must (`*` and qualifiers), with parentheses overriding the order:

    | Declaration | In words |
    | --- | --- |
    | `int *p[4];` | `p` is an array of 4 pointers to `int` |
    | `int (*p)[4];` | `p` is a pointer to an array of 4 `int` |
    | `int (*f)(void);` | `f` is a pointer to a function taking no arguments and returning `int` |
    | `int *(*table[8])(const char *);` | `table` is an array of 8 pointers to functions taking `const char *` and returning a pointer to `int` |
    | `volatile uint32_t *const reg;` | `reg` is a `const` pointer to a `volatile uint32_t` |
    | `const char *const names[];` | `names` is an array of `const` pointers to `const char` |

    Typedefs make the long ones readable:

    ```c
    typedef int *lookup_function(const char *key);

    lookup_function *table[8];
    ```

    See also [test-out Q10](./test-out.md#q10-reading-a-declaration).

??? note "Exercise 2: typedefs for pointer and array declarators"
    ```c
    #include <stddef.h>

    typedef int *int_pointer;       /* "pointer to int" */
    typedef int sample_row[4];      /* "array of 4 int" */

    int_pointer channels[4];        /* same type as: int *channels[4];   */
    sample_row *current_row;        /* same type as: int (*current_row)[4]; */

    int row_sum(const sample_row *row)
    {
        int sum = 0;

        for (size_t i = 0; i < sizeof *row / sizeof (*row)[0]; i++) {
            sum += (*row)[i];
        }
        return sum;
    }
    ```

    The typedef names the inner part, so the outer declaration reads left to right. A
    pointer to a whole row also keeps the row length in the type: `sizeof *row` is 16 here,
    not the size of a pointer.

    Be careful with typedefs that hide a pointer. `const int_pointer p` declares
    `int *const p`, a constant *pointer* to a modifiable `int`, not a pointer to `const int`.
    GCC rejects `p = &other;` with `assignment of read-only variable 'p'` but accepts
    `*p = 1;`. That is why style guides such as the Linux kernel's avoid pointer typedefs
    for ordinary objects. Function-pointer typedefs are the common exception.

??? note "Exercise 3: an opaque driver handle"
    ```c
    /* adc.h */
    #ifndef ADC_H
    #define ADC_H

    #include <stdint.h>

    struct adc;                                   /* incomplete type: callers see a name only */

    enum adc_status { ADC_OK, ADC_BUSY, ADC_INVALID, ADC_NO_RESOURCE };

    struct adc *adc_create(unsigned int channel);
    void adc_destroy(struct adc *adc);            /* accepts NULL */
    enum adc_status adc_read(struct adc *adc, uint16_t *value);

    #endif
    ```

    ```c
    /* adc.c */
    #include <stddef.h>
    #include "adc.h"

    #define ADC_INSTANCES 4u

    struct adc {
        unsigned int channel;
        int in_use;
    };

    static struct adc instances[ADC_INSTANCES];   /* no heap: a fixed pool */

    struct adc *adc_create(unsigned int channel)
    {
        for (size_t i = 0; i < ADC_INSTANCES; i++) {
            if (!instances[i].in_use) {
                instances[i].in_use = 1;
                instances[i].channel = channel;
                return &instances[i];
            }
        }
        return NULL;
    }

    void adc_destroy(struct adc *adc)
    {
        if (adc != NULL) {
            adc->in_use = 0;
        }
    }

    enum adc_status adc_read(struct adc *adc, uint16_t *value)
    {
        if (adc == NULL || value == NULL || !adc->in_use) {
            return ADC_INVALID;
        }
        *value = (uint16_t)(adc->channel * 100u);    /* stands in for the hardware read */
        return ADC_OK;
    }
    ```

    - Callers can hold and pass a `struct adc *`, but cannot declare a `struct adc`, read its
      fields, or take its `sizeof`. The layout can change without recompiling callers.
    - `adc_create` reports exhaustion with `NULL` instead of failing silently.
    - `adc_destroy(NULL)` is allowed, like `free(NULL)`, which simplifies cleanup paths.
    - A fixed pool keeps the API heap-free. Swapping in `malloc` later does not change the
      header, which is the point of the opaque type.

??? note "Exercise 4: const-correct parameters and safer call sites"
    Before: the parameter is writable, although the function only reads.

    ```c
    #include <stddef.h>
    #include <stdint.h>

    uint8_t checksum(uint8_t *data, size_t length);

    static const uint8_t calibration[4] = { 1, 2, 3, 4 };

    uint8_t check_calibration(void)
    {
        return checksum((uint8_t *)calibration, sizeof calibration);   /* cast needed */
    }
    ```

    Without the cast, GCC 13 warns:
    `passing argument 1 of 'checksum' discards 'const' qualifier from pointer target type`.
    The cast silences that, and also removes the compiler's protection: if `checksum` ever
    writes through the pointer, it writes to a `const` object, which is undefined and may
    fault in flash.

    After: the parameter says the data is read-only.

    ```c
    #include <stddef.h>
    #include <stdint.h>

    uint8_t checksum(const uint8_t *data, size_t length)
    {
        uint8_t sum = 0;

        for (size_t i = 0; i < length; i++) {
            sum = (uint8_t)(sum + data[i]);
        }
        return sum;
    }

    static const uint8_t calibration[4] = { 1, 2, 3, 4 };

    uint8_t check_calibration(void)
    {
        return checksum(calibration, sizeof calibration);           /* no cast */
    }
    ```

    The call sites that become safer are the ones that needed a cast: `const` tables, string
    literals, and data received through a `const` pointer. Every call that passed writable
    data still compiles unchanged, because adding `const` is an implicit conversion. Inside
    the function, an accidental `data[i] = 0;` is now a compile error instead of a silent
    write.

??? note "Exercise 5: a definition in a header"
    A header that *defines* an object, included by two source files:

    <!-- c-check: skip -- header from a two-file example; it compiles alone but fails at link time -->

    ```c
    /* config.h */
    #ifndef CONFIG_H
    #define CONFIG_H

    int retry_limit = 3;    /* a definition in a header */

    int get_retry_limit(void);

    #endif
    ```

    `config.c` and `main.c` both include it. Each compiles, and the link fails:

    ```text
    $ gcc -std=c17 -c config.c
    $ gcc -std=c17 -c main.c
    $ gcc config.o main.o -o app
    /usr/bin/ld: main.o:(.data+0x0): multiple definition of `retry_limit'; config.o:(.data+0x0): first defined here
    collect2: error: ld returned 1 exit status
    ```

    The include guard does not help: it prevents a second inclusion *within one*
    translation unit, and here each `.c` file is a separate translation unit with its own
    copy of the definition. The fix is one declaration in the header and one definition in
    one source file:

    ```c
    /* config.h */
    #ifndef CONFIG_H
    #define CONFIG_H

    extern int retry_limit;     /* declaration only */

    int get_retry_limit(void);

    #endif
    ```

    ```c
    /* config.c */
    #include "config.h"

    int retry_limit = 3;        /* the one definition */

    int get_retry_limit(void)
    {
        return retry_limit;
    }
    ```

    With the initializer removed (`int retry_limit;` in the header), GCC 10 and later still
    report `multiple definition`, but older compilers and `-fcommon` merge the copies
    silently. [Case study 14](../failure-case-studies/14-common-symbol-merge.md) shows how
    that goes wrong.

??? note "Exercise 6: an incompatible callback"
    <!-- c-check: expect-warning -->

    ```c
    #include <stdint.h>

    typedef void (*event_callback)(void *context, uint32_t event);

    int register_callback(event_callback callback, void *context);

    struct uart;

    /* Wrong: the first parameter is struct uart *, not void *. */
    static void on_uart_event(struct uart *uart, uint32_t event)
    {
        (void)uart;
        (void)event;
    }

    /* Right: matches event_callback exactly and converts inside. */
    static void on_uart_event_adapter(void *context, uint32_t event)
    {
        struct uart *uart = context;

        (void)uart;
        (void)event;
    }

    void setup(struct uart *uart)
    {
        register_callback(on_uart_event, uart);
        register_callback(on_uart_event_adapter, uart);
    }
    ```

    GCC 13 warns about the first registration:

    ```text
    warning: passing argument 1 of ‘register_callback’ from incompatible pointer type [-Wincompatible-pointer-types]
    note: expected ‘event_callback’ {aka ‘void (*)(void *, unsigned int)’} but argument is of type ‘void (*)(struct uart *, uint32_t)’ {aka ‘void (*)(struct uart *, unsigned int)’}
    ```

    GCC 14 makes this an error by default. Calling a function through a pointer of an
    incompatible type is undefined behavior (C11 6.3.2.3p8), even when `void *` and
    `struct uart *` happen to have the same representation. Do not silence it with a cast
    such as `(event_callback)on_uart_event`. Write the adapter, which has the exact
    signature and converts the `void *` back to the real type inside.

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

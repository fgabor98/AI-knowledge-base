---
status: draft
reviewed: false
domain: c
difficulty: beginner
last_reviewed: null
---

# Types, Values, And Objects

A type tells the compiler which values an expression may have, how operations are interpreted, and which object representations are valid. An object is a region of storage that can hold a value during execution. In embedded work, a byte pattern, a register field, and a C value are often incorrectly treated as automatically interchangeable.

## Learning Objectives

- Classify the major ISO C type categories.
- Distinguish an object, its stored value, and an expression’s type.
- Select integer types using range and interface requirements.
- Use sizeof, stdint.h, limits.h, and inttypes.h.
- Recognize implementation-defined width, signedness, representation, and formatting.
- Identify where a hardware or wire interface needs a representation contract beyond the C type system.

## Type Categories

| Category | Examples | Typical use |
| --- | --- | --- |
| void | void, void pointer | No value; generic object-pointer interface |
| Arithmetic | integer, floating, _Bool, character types | Numbers, flags, text units, measurements |
| Scalar | arithmetic types and pointers | A single value usable in a condition |
| Aggregate | arrays and structures | Collections and records |
| Union | union packet_value | Overlapping member storage |
| Function | int (int) | Function type; callback targets |
| Incomplete | struct device, int array without a bound | Size or members not yet known |

An enumeration is an integer type with named constants. A pointer is scalar, but its representation, validity, alignment, and dereference rules are separate concerns. const, volatile, restrict, and _Atomic qualify types; they do not create unrelated base categories.

## Objects, Values, And Expressions

~~~c
#include <stdint.h>

void show_object_and_value(void)
{
    uint16_t count = 3u;
    uint16_t next = (uint16_t)(count + 1u);
    (void)next;
}
~~~

- count names an object with storage.
- Its current stored value is 3.
- count + 1u is an expression whose type follows arithmetic-conversion rules.
- next is a different object initialized with the result.

An object also has storage duration, alignment, lifetime, effective type, and rules governing whether its current value is valid to read. An extern declaration describes an object without allocating it in that translation unit; a definition creates it.

## Integer Types

The standard integer types include signed char, short, int, long, and long long, with unsigned counterparts. They have minimum ranges, but exact widths are implementation-defined. char is distinct and has the range and representation of either signed char or unsigned char, chosen by the implementation.

Use char for character data; unsigned char or uint8_t for raw bytes according to project convention; and signed char only for deliberate signed character arithmetic.

The stdint.h types express width intent:

~~~c
#include <stdint.h>

uint8_t  packet_type;
int16_t  temperature_centi_degrees;
uint32_t sample_count;
int64_t  timestamp_ticks;
~~~

Exact-width typedefs exist only if the implementation provides a type with exactly that width and no padding. Least and fast types are useful when exact width is not required:

~~~c
#include <stdint.h>

uint_least16_t persistent_counter;
uint_fast32_t working_counter;
~~~

Use size_t for object sizes and ptrdiff_t for differences between pointers into one array. Do not choose a type only because it is pointer-sized on the current board.

## Ranges And Limits

Use standard headers instead of hard-coded assumptions:

~~~c
#include <inttypes.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>

void print_platform_limits(uint32_t count)
{
    printf("CHAR_BIT=%d\n", CHAR_BIT);
    printf("sizeof(int)=%zu, INT_MIN=%d, INT_MAX=%d\n",
           sizeof(int), INT_MIN, INT_MAX);
    printf("count=%" PRIu32 "\n", count);
}
~~~

CHAR_BIT is bits per C byte. Most targets use eight, but ISO C does not require it. sizeof(type) is measured in C bytes and returns size_t.

A format mismatch in printf is a correctness error. Use %zu for size_t and inttypes.h macros for fixed-width integers.

## Boolean And Character Values

Include stdbool.h in pre-C23 code for bool, true, and false:

~~~c
#include <stdbool.h>

void set_ready(void)
{
    bool ready = false;
    if (!ready) {
        ready = true;
    }
}
~~~

Zero is false and nonzero is true in conditions. Conversion to _Bool produces 0 or 1; a multi-bit status register should be compared explicitly when “any bit set” is the intended meaning.

A character function counts code units, not Unicode characters:

~~~c
#include <stddef.h>

size_t text_length(const char *text)
{
    size_t length = 0u;
    while (text[length] != '\0') {
        ++length;
    }
    return length;
}
~~~

For protocols and commands, define whether data is ASCII, UTF-8, another encoding, or uninterpreted bytes.

## Floating-Point Types

float, double, and long double are distinct. Their precision, range, representation, evaluation, and library support are implementation-dependent. Many microcontrollers use software floating point unless configured with an FPU.

Before using floating point in a real-time path, decide the precision and error tolerance, inspect code generation and runtime support, and consider fixed-point arithmetic. Never compare computed floating values for exact equality unless the algorithm is specifically designed for it.

## sizeof And Widths

~~~c
#include <stddef.h>
#include <stdint.h>

struct sample {
    uint16_t value;
    uint8_t status;
};

size_t sample_size(void)
{
    struct sample sample = {0};
    return sizeof sample;
}
~~~

sizeof includes padding inserted for alignment. It does not give the number of meaningful protocol bytes.

An array’s size is preserved only where it has not converted to a pointer:

<!-- c-check: expect-warning -->

~~~c
#include <stddef.h>
#include <stdint.h>

void inspect(uint8_t data[10])
{
    size_t parameter_size = sizeof data;
    (void)parameter_size;
}

void inspect_local(void)
{
    uint8_t data[10];
    size_t array_size = sizeof data;
    (void)array_size;
}
~~~

The parameter declaration is adjusted to a pointer type, so parameter_size is the pointer size while array_size covers all ten elements.

## Representation Is A Separate Contract

A C type does not by itself define endianness, packet framing, peripheral side effects, DMA ownership, ABI compatibility, or atomicity of a multi-byte access.

Use explicit conversion for external representations:

~~~c
#include <stdint.h>

uint16_t read_be16(const uint8_t bytes[2])
{
    return (uint16_t)(((uint16_t)bytes[0] << 8) | bytes[1]);
}

void write_be16(uint8_t bytes[2], uint16_t value)
{
    bytes[0] = (uint8_t)(value >> 8);
    bytes[1] = (uint8_t)value;
}
~~~

This is visible and testable instead of relying on structure layout or a cast.

## Exercises

1. Print CHAR_BIT, standard type sizes, and limits on host and target.
2. Replace a protocol field declared unsigned long with a type expressing its required width.
3. Test read_be16 and write_be16 at 0, 1, 0x80, and UINT16_MAX.
4. Change an array parameter to pointer plus explicit length and explain the contract improvement.
5. Check every fixed-width integer printf in a project for the correct format macro.

### Model Answers

Try each exercise before opening its answer. Where an exercise asks you to build
something, the answer shows one good solution, not the only one.

??? note "Exercise 1: type sizes and limits on host and target"
    ```c
    #include <limits.h>
    #include <stddef.h>
    #include <stdint.h>
    #include <stdio.h>

    #define SHOW_SIZE(type) printf("%-12s %zu\n", #type, sizeof(type))

    int main(void)
    {
        printf("CHAR_BIT     %d\n", CHAR_BIT);
        printf("char signed  %s\n", CHAR_MIN < 0 ? "yes" : "no");
        SHOW_SIZE(short);
        SHOW_SIZE(int);
        SHOW_SIZE(long);
        SHOW_SIZE(long long);
        SHOW_SIZE(void *);
        SHOW_SIZE(size_t);
        SHOW_SIZE(float);
        SHOW_SIZE(double);
        SHOW_SIZE(long double);
        printf("INT_MAX      %d\n", INT_MAX);
        printf("LONG_MAX     %ld\n", LONG_MAX);
        printf("SIZE_MAX     %zu\n", SIZE_MAX);
        return 0;
    }
    ```

    The x86-64 Linux column was measured with GCC 13. The 32-bit Arm column comes from the
    Arm ABI (AAPCS), not from a run, so print it on your own board to confirm:

    | | x86-64 Linux (LP64) | 32-bit Arm, `arm-none-eabi` (ILP32) |
    | --- | --- | --- |
    | `CHAR_BIT` | 8 | 8 |
    | `char` signed | yes | **no** |
    | `short` / `int` | 2 / 4 | 2 / 4 |
    | `long` | **8** | **4** |
    | `long long` | 8 | 8 |
    | `void *`, `size_t` | **8** | **4** |
    | `float` / `double` | 4 / 8 | 4 / 8 |
    | `long double` | **16** (80-bit x87 format, padded) | **8** (same as `double`) |
    | `LONG_MAX` | 9223372036854775807 | 2147483647 |
    | `SIZE_MAX` | 18446744073709551615 | 4294967295 |

    The rows in bold are the ones that break code. Plain `char` changes sign, so
    `char c = 0xFF; if (c < 0)` behaves differently. `long` and pointers halve in size.
    `long double` stops being more precise than `double`. On an 8-bit AVR, `int` is 2
    bytes as well. Run this on every new target and keep the output with the port notes.

??? note "Exercise 2: a protocol field with a stated width"
    Before: the field is "whatever `unsigned long` is".

    ```c
    struct telemetry {
        unsigned long sequence;     /* 32 bits on the wire */
        unsigned short flags;
    };
    ```

    `unsigned long` is 32 bits on 32-bit Arm and 64 bits on x86-64 Linux. The same source then
    means two different things. A host-side tool that decodes target data, or a struct that is
    copied to a file, disagrees with the device. A counter that wraps at 2^32 on the device
    never wraps in the host test.

    After: the width comes from the protocol, and the wire format is written explicitly.

    ```c
    #include <stdint.h>

    struct telemetry {
        uint32_t sequence;          /* protocol: 32-bit, big-endian, wraps to 0 */
        uint16_t flags;             /* protocol: 16-bit, big-endian */
    };

    void encode_telemetry(uint8_t out[6], const struct telemetry *t)
    {
        out[0] = (uint8_t)(t->sequence >> 24);
        out[1] = (uint8_t)(t->sequence >> 16);
        out[2] = (uint8_t)(t->sequence >> 8);
        out[3] = (uint8_t)t->sequence;
        out[4] = (uint8_t)(t->flags >> 8);
        out[5] = (uint8_t)t->flags;
    }
    ```

    `uint32_t` fixes the range and wrap behavior on every platform. It does not fix byte
    order or padding, so the structure is still not the wire format. The encoder is. The
    wire size is 6 bytes, while `sizeof(struct telemetry)` is usually 8 because of padding.

??? note "Exercise 3: boundary tests for read_be16 and write_be16"
    ```c
    #include <assert.h>
    #include <stddef.h>
    #include <stdint.h>

    uint16_t read_be16(const uint8_t bytes[2])
    {
        return (uint16_t)(((uint16_t)bytes[0] << 8) | bytes[1]);
    }

    void write_be16(uint8_t bytes[2], uint16_t value)
    {
        bytes[0] = (uint8_t)(value >> 8);
        bytes[1] = (uint8_t)value;
    }

    int main(void)
    {
        static const struct {
            uint16_t value;
            uint8_t bytes[2];
        } cases[] = {
            { 0x0000u, { 0x00, 0x00 } },
            { 0x0001u, { 0x00, 0x01 } },      /* low byte goes last */
            { 0x0080u, { 0x00, 0x80 } },      /* top bit of the low byte: no sign extension */
            { 0xFFFFu, { 0xFF, 0xFF } },      /* UINT16_MAX */
            { 0x1234u, { 0x12, 0x34 } },      /* the two bytes differ: catches a swap */
        };

        for (size_t i = 0; i < sizeof cases / sizeof cases[0]; i++) {
            uint8_t out[2] = { 0xAA, 0xAA };

            write_be16(out, cases[i].value);
            assert(out[0] == cases[i].bytes[0] && out[1] == cases[i].bytes[1]);
            assert(read_be16(cases[i].bytes) == cases[i].value);
            assert(read_be16(out) == cases[i].value);              /* round trip */
        }
        return 0;
    }
    ```

    Each value tests something specific. `0x0080` catches an implementation that goes
    through a signed `char` and sign-extends to `0xFF80`. `UINT16_MAX` catches shifts done in
    a type too narrow for the result. `0x0001` catches swapped bytes. The extra `0x1234`
    has two different non-zero bytes, so each output byte is checked against a real value.
    Checking the exact bytes, not only the round trip, catches an encoder and decoder that
    are wrong in the same way, such as both writing little-endian.

??? note "Exercise 4: an array parameter versus pointer and length"
    Before:

    <!-- c-check: expect-warning -->

    ```c
    #include <stddef.h>
    #include <stdint.h>

    void fill_pattern(uint8_t data[16])
    {
        for (size_t i = 0; i < sizeof data; i++) {    /* sizeof a pointer: 8, or 4 on Arm */
            data[i] = (uint8_t)i;
        }
    }
    ```

    GCC warns: `'sizeof' on array function parameter 'data' will return size of 'uint8_t *'`.
    The `[16]` is decoration: the parameter *is* a `uint8_t *`, callers can pass a buffer of
    any size, and the loop fills 8 bytes on x86-64 and 4 on 32-bit Arm.

    After:

    ```c
    #include <stddef.h>
    #include <stdint.h>

    /* Writes exactly length bytes to data. data may be NULL only if length is 0. */
    void fill_pattern(uint8_t *data, size_t length)
    {
        for (size_t i = 0; i < length; i++) {
            data[i] = (uint8_t)i;
        }
    }
    ```

    The improvements are in the contract, not the code:

    - The length is a value the caller must supply, so it is visible at every call site and
      reviewable there, usually as `sizeof buffer`.
    - The function no longer pretends to know the buffer size. The size it uses is the one it
      is given.
    - The null-pointer rule is written down.

    When the size really is fixed, C99's `static` says so and lets GCC check calls:

    <!-- c-check: skip -- GCC reports this only when it generates code, which the syntax-only check does not do -->

    ```c
    #include <stdint.h>

    void fill_block(uint8_t data[static 16]);    /* at least 16 bytes, never NULL */

    void caller(void)
    {
        uint8_t small[8];

        fill_block(small);
    }
    ```

    ```text
    warning: 'fill_block' accessing 16 bytes in a region of size 8 [-Wstringop-overflow=]
    ```

??? note "Exercise 5: checking printf formats for fixed-width integers"
    `-Wformat` (part of `-Wall`) checks formats against the types *of the target being
    compiled*, so a wrong format can pass on one platform and fail on another:

    ```c
    #include <inttypes.h>
    #include <stdint.h>
    #include <stdio.h>

    void print_counters(uint32_t packets, uint64_t bytes)
    {
        printf("packets=%" PRIu32 " bytes=%" PRIu64 "\n", packets, bytes);
    }
    ```

    With guessed formats, `printf("packets=%u bytes=%llu\n", packets, bytes);`, x86-64
    GCC 13 reports:

    ```text
    warning: format '%llu' expects argument of type 'long long unsigned int', but argument 3 has type 'uint64_t' {aka 'long unsigned int'} [-Wformat=]
    ```

    On 32-bit platforms `uint64_t` is usually `unsigned long long`, so `%llu` is right there,
    and only the host build complains. The reverse also happens. `arm-none-eabi` (newlib)
    defines `uint32_t` as `unsigned long`, so `%u` warns on the target and passes on the
    host.

    A project-wide check:

    1. Build with `-Wall -Wformat=2` for *both* the host and every target, and treat format
       warnings as errors.
    2. Search for format strings that are only correct by luck:
       `grep -rnE '%(l|ll|h)?[udx]' --include='*.c' src/`, and review each hit whose argument
       is a `<stdint.h>` type.
    3. Replace them with the `<inttypes.h>` macros: `PRIu32`, `PRId16`, `PRIx64`, and so on.
       Use `%zu` for `size_t`.
    4. Give custom logging functions `__attribute__((format(printf, N, M)))`, so the same
       checks apply to them. Without it, every call to the project's own logger is unchecked.

## Common Mistakes

- Assuming int is 32 bits or char is unsigned.
- Using sizeof as a protocol width.
- Printing size_t or fixed-width integers with guessed formats.
- Assuming uint8_t exists on every conforming implementation.
- Assuming all-bits-zero is a valid representation for every pointer or float.
- Using floating point without checking code generation and runtime size.
- Assuming enum width or multi-byte access atomicity.

## Debugging Checklist

1. Inspect sizeof, CHAR_BIT, and limit macros.
2. Enable conversion and sign-conversion warnings.
3. Check both operand types, not only the destination type.
4. Check the format string and required header.
5. Inspect the target ABI when layout or calls matter.
6. Use explicit byte conversion for serialized data.
7. Test values across signed/unsigned and range boundaries.
8. For hardware, verify access width, alignment, and side effects in the reference manual.

## Related Topics

- [Language Fundamentals overview](./index.md)
- [Expressions And Operators](./expressions-and-operators.md)
- [Structures, Unions, And Enumerations](./structures-unions-and-enums.md)
- [Semantics And Memory Model](../semantics-and-memory/index.md)
- [Platform-Specific C](../platform-specific-c/index.md)
- [C Programming](../index.md)

## References

- [ISO/IEC 9899 standards and drafts](https://open-std.org/jtc1/sc22/wg14/www/standards.html)
- [C11 public draft N1570](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n1570.pdf)
- [C integer types — cppreference](https://en.cppreference.com/w/c/language/arithmetic_types)
- [C stdint.h — cppreference](https://en.cppreference.com/w/c/types/integer)
- [GCC type attributes](https://gcc.gnu.org/onlinedocs/gcc/Attribute-Syntax.html)

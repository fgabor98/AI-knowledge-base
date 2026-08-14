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

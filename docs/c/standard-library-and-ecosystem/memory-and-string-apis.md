---
status: draft
reviewed: false
domain: c
difficulty: intermediate
last_reviewed: null
---

# Memory And String APIs

The functions in string.h operate on either raw bytes or null-terminated character sequences. Their contracts are precise and unforgiving: a correct function with an invalid length, lifetime, overlap relationship, or terminator can still produce undefined behavior.

## Learning Objectives

- Distinguish byte operations from string operations.
- Use memcpy, memmove, memset, and memcmp under their exact contracts.
- Apply strlen and string-copy functions only to valid strings.
- Design explicit capacity and truncation policies.
- Use library calls for representation copying without creating invalid typed access.
- Review library calls for overlap, bounds, and integer overflow.

## Byte Operations

Memory functions operate on arrays of unsigned-character-sized bytes:

~~~c
#include <stddef.h>
#include <stdint.h>
#include <string.h>

int copy_packet(uint8_t *destination, size_t capacity,
                const uint8_t *source, size_t length)
{
    if (destination == NULL || source == NULL || length > capacity) {
        return -1;
    }

    memcpy(destination, source, length);
    return 0;
}
~~~

The destination must contain at least length bytes, the source must contain at least length readable bytes, and the regions must not overlap. If overlap is possible, use memmove:

~~~c
#include <stddef.h>
#include <string.h>

void shift_right(unsigned char *buffer, size_t length)
{
    if (buffer != NULL && length > 1u) {
        memmove(buffer + 1u, buffer, length - 1u);
    }
}
~~~

The destination and source may overlap for memmove, but both must still be valid for the specified ranges.

## memset

memset writes the same unsigned-char value to every byte:

~~~c
#include <stddef.h>
#include <string.h>

void clear_bytes(void *memory, size_t length)
{
    if (memory != NULL) {
        memset(memory, 0, length);
    }
}
~~~

It does not set an array of int values to an arbitrary integer, and it is not a universal typed initializer. Zero bytes are useful for byte buffers; typed objects may have nonzero valid representations or semantic defaults.

Use a typed loop or initializer for values:

~~~c
#include <stddef.h>

void clear_ints(int *values, size_t count)
{
    if (values != NULL) {
        for (size_t i = 0u; i < count; ++i) {
            values[i] = 0;
        }
    }
}
~~~

## memcmp

memcmp compares object representations, not semantic values:

~~~c
#include <stddef.h>
#include <string.h>

int bytes_equal(const void *left, const void *right, size_t length)
{
    if (left == NULL || right == NULL) {
        return length == 0u && left == right;
    }
    return memcmp(left, right, length) == 0;
}
~~~

Do not use memcmp to compare structures with padding, pointers, floating values with multiple representations, or fields whose semantic equality differs from byte equality.

## Strings

A C string is a character array with a terminating null character:

~~~c
#include <stddef.h>
#include <string.h>

int copy_name(char *destination, size_t capacity, const char *source)
{
    size_t length;

    if (destination == NULL || source == NULL || capacity == 0u) {
        return -1;
    }

    length = strlen(source);
    if (length >= capacity) {
        destination[0] = '\0';
        return -2;
    }

    memcpy(destination, source, length + 1u);
    return 0;
}
~~~

strlen requires source to be a valid null-terminated string. It has no capacity parameter and cannot safely inspect untrusted or unterminated data. Validate or bound input before calling it.

For a bounded inspection, write or use a function whose contract reports termination:

~~~c
#include <stddef.h>

int bounded_length(const char *text, size_t capacity, size_t *length)
{
    if (text == NULL || length == NULL) {
        return -1;
    }

    for (size_t i = 0u; i < capacity; ++i) {
        if (text[i] == '\0') {
            *length = i;
            return 0;
        }
    }

    return -2;
}
~~~

## Formatted String Functions

snprintf writes at most capacity bytes including the terminator when capacity is nonzero:

~~~c
#include <stddef.h>
#include <stdio.h>

int format_status(char *buffer, size_t capacity, unsigned int value)
{
    int result = snprintf(buffer, capacity, "status=%u", value);
    if (result < 0) {
        return -1;
    }
    if ((size_t)result >= capacity) {
        return -2;
    }
    return 0;
}
~~~

The return value is the number of characters that would have been written excluding the terminator. A result greater than or equal to capacity indicates truncation. An encoding or implementation error may return a negative value.

Formatted output can pull in substantial code and may allocate or lock depending on libc. Use a bounded project formatter in constrained paths.

## Explicit Serialization

Do not serialize a structure by copying its size blindly:

~~~c
#include <stdint.h>

void encode_u16_be(uint8_t output[2], uint16_t value)
{
    output[0] = (uint8_t)(value >> 8);
    output[1] = (uint8_t)value;
}
~~~

Explicit encoding defines byte order and width. For a known same-implementation representation, memcpy can copy bytes, but it does not create a portable wire format.

## Annex K And Nonstandard “Safe” APIs

Some implementations provide bounds-checking interfaces such as the optional Annex K functions. Availability, behavior, and adoption vary widely. A function named safe does not replace a clear ownership, capacity, overlap, and error policy.

Project wrappers can be useful when they establish consistent truncation and diagnostics, but they should have a simple contract and tests.

## Embedded Considerations

On a target, check:

- whether the implementation uses optimized word copies and what alignment they require;
- whether copying a volatile object is permitted and what access semantics are required;
- whether buffers are in cacheable or DMA-visible memory;
- whether lengths can be attacker- or hardware-controlled;
- whether a string function can run in an interrupt or fault context;
- whether locale or floating formatting is pulled in;
- whether the operation is bounded enough for the deadline.

A standard function can be correct in ISO C and still be inappropriate for a register window, DMA buffer, or hard real-time path.

## Exercises

1. Write tests for memcpy with zero, one, full, and maximum lengths.
2. Demonstrate overlapping copy with memcpy and memmove; keep the invalid case only in a negative test.
3. Compare structures by members instead of memcmp.
4. Implement a truncation-reporting string copy that always terminates when capacity is nonzero.
5. Measure flash and stack cost for snprintf and a project-specific integer formatter.
6. Encode and decode a packet with explicit endianness and reject truncated input.

## Common Mistakes

- Passing an invalid length to memcpy, memmove, or memset.
- Using memcpy on overlapping regions.
- Calling strlen on binary or unterminated input.
- Forgetting that snprintf capacity includes the terminator.
- Treating a negative snprintf result as ordinary truncation.
- Using memcmp for semantic structure equality.
- Assuming memset creates typed zero values.
- Serializing structures with padding and endianness assumptions.
- Copying volatile or DMA storage without a platform ownership contract.
- Believing a bounds-named API is safe without reading its exact contract.

## Debugging Checklist

1. Record source, destination, length, capacity, and overlap status.
2. Validate termination before strlen or string comparisons.
3. Check snprintf return values for error and truncation.
4. Run AddressSanitizer and fuzz boundary lengths on host builds.
5. Inspect alignment, cache, DMA ownership, and volatile semantics on target.
6. Compare semantic fields rather than raw structures.
7. Add tests for zero capacity, empty strings, maximum lengths, and overflow.
8. Check linked-image and timing impact of general-purpose formatting.

## Related Topics

- [Standard Library And Ecosystem overview](./index.md)
- [Language Fundamentals](../language-fundamentals/arrays-strings-and-buffers.md)
- [Semantics And Memory](../semantics-and-memory/index.md)
- [I/O, Diagnostics, And Errors](./io-diagnostics-and-errors.md)
- [C Programming](../index.md)

## References

- [ISO/IEC 9899 standards and drafts](https://open-std.org/jtc1/sc22/wg14/www/standards.html)
- [C11 public draft N1570](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n1570.pdf)
- [cppreference C string and byte functions](https://en.cppreference.com/w/c/string/byte)
- [CERT C array and string rules](https://wiki.sei.cmu.edu/confluence/display/c)

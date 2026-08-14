---
status: draft
reviewed: false
domain: c
difficulty: beginner
last_reviewed: null
---

# Arrays, Strings, And Buffers

Arrays provide contiguous storage for a fixed number of elements. A string is a convention—a sequence of characters terminated by a null character—not a distinct built-in C type. A buffer may contain text, binary data, samples, register images, or partially filled DMA storage. The type alone does not tell you its length, capacity, encoding, or ownership.

## Learning Objectives

- Declare and initialize arrays with known bounds.
- Explain when an array converts to a pointer.
- Pass buffers with explicit length and capacity.
- Distinguish strings from byte buffers.
- Handle multidimensional arrays and row-major layout.
- Recognize the difference between sizeof, capacity, and string length.
- Design bounded operations for embedded input and output paths.

## Array Objects

An array contains a fixed number of elements of one type:

~~~c
#include <stdint.h>

uint16_t samples[8];
uint8_t packet[64];
char label[12];
~~~

The array object owns storage for every element. sizeof samples is the size of all eight elements, and sizeof packet is 64 C bytes.

Initialization can infer the bound:

~~~c
#include <stdint.h>

static const uint8_t header[] = {0xA5u, 0x5Au, 0x01u};
static const char name[] = "sensor";
~~~

The first array has three elements. The second has seven: six visible characters plus the terminating null.

## Array-To-Pointer Conversion

In most expressions an array converts to a pointer to its first element:

~~~c
#include <stddef.h>
#include <stdint.h>

void inspect(void)
{
    uint8_t bytes[4] = {1u, 2u, 3u, 4u};
    uint8_t *first = bytes;
    (void)first;

    size_t element_count = sizeof bytes / sizeof bytes[0];
    (void)element_count;
}
~~~

Important exceptions include sizeof, unary address-of, and a string literal used to initialize an array. Once an array is passed to a function, the function receives a pointer and cannot infer the caller’s bound.

~~~c
#include <stddef.h>
#include <stdint.h>

void receive(uint8_t data[32], size_t length)
{
    (void)sizeof data;
    for (size_t i = 0u; i < length; ++i) {
        data[i] = 0u;
    }
}
~~~

The bound in a parameter declaration is documentation unless another mechanism enforces it.

## Buffer Contracts

A robust mutable-buffer interface names pointer, count, and capacity:

~~~c
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

bool append_byte(uint8_t *buffer,
                 size_t capacity,
                 size_t *length,
                 uint8_t value)
{
    if (buffer == NULL || length == NULL || *length >= capacity) {
        return false;
    }

    buffer[*length] = value;
    ++*length;
    return true;
}
~~~

The contract is that buffer points to writable storage with capacity elements, length points to the initialized count, and 0 <= length <= capacity on entry. The function writes one element only on success. The caller retains ownership.

For binary data, zero is a valid byte and cannot mark the end. Use pointer plus count for a view of existing storage, and pointer plus capacity plus current length for a mutable builder.

## Strings

A C string is a character sequence ending at the first null character:

~~~c
#include <stddef.h>

size_t bounded_string_length(const char *text, size_t capacity)
{
    if (text == NULL) {
        return 0u;
    }

    for (size_t i = 0u; i < capacity; ++i) {
        if (text[i] == '\0') {
            return i;
        }
    }

    return capacity;
}
~~~

Returning capacity means no terminator was found within the inspected range; the caller must decide whether that is an error.

For a writable character array:

~~~c
char name[8] = "sensor";
~~~

There is room for six characters, the terminator, and one remaining zero element. Writing eight non-null characters leaves no terminator; writing a ninth overflows.

A string literal initializes an array but must not be modified through a pointer:

~~~c
const char *message = "ready";
char mutable_message[] = "ready";
~~~

The first refers to non-writable literal storage; the second creates a writable array.

## sizeof Versus strlen

sizeof is an object or type size operation. strlen scans for a terminator and requires a valid string:

~~~c
#include <string.h>

void compare_sizes(void)
{
    char text[16] = "abc";
    size_t capacity = sizeof text;
    size_t length = strlen(text);
    (void)capacity;
    (void)length;
}
~~~

When text is a pointer, sizeof reports pointer size, not the referenced storage size. Never call strlen on a binary buffer or on input not proven to contain a terminator.

## Multidimensional Arrays

A multidimensional array is an array of arrays:

~~~c
#include <stddef.h>
#include <stdint.h>

static void clear_matrix(uint16_t matrix[3][4])
{
    for (size_t row = 0u; row < 3u; ++row) {
        for (size_t column = 0u; column < 4u; ++column) {
            matrix[row][column] = 0u;
        }
    }
}
~~~

The parameter is adjusted to a pointer to an array of four uint16_t elements. The second dimension is required for address calculation. C stores elements in row-major order.

Variable-length array parameters can express relationships between dimensions:

~~~c
#include <stddef.h>
#include <stdint.h>

void clear_matrix_n(size_t rows, size_t columns,
                    uint16_t matrix[rows][columns])
{
    for (size_t row = 0u; row < rows; ++row) {
        for (size_t column = 0u; column < columns; ++column) {
            matrix[row][column] = 0u;
        }
    }
}
~~~

These declarations still do not validate storage. Check multiplication for overflow before allocating or indexing a dynamically sized matrix.

## Binary Buffers

Binary buffers need an explicit length and representation contract:

~~~c
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

bool read_frame(const uint8_t *frame, size_t frame_length,
                uint16_t *sequence)
{
    if (frame == NULL || sequence == NULL || frame_length < 2u) {
        return false;
    }

    *sequence = (uint16_t)(((uint16_t)frame[0] << 8) | frame[1]);
    return true;
}
~~~

The function does not assume the frame is a structure in memory. It checks length and decodes defined byte order explicitly.

For DMA or peripheral buffers, also document producer, consumer, ownership handoff, cache maintenance, alignment, access width, concurrent modification, and when the length becomes stable.

## Flexible Array Members

A flexible array member is an incomplete array at the end of a structure:

~~~c
#include <stddef.h>
#include <stdint.h>

struct message {
    uint16_t type;
    uint16_t length;
    uint8_t payload[];
};

size_t message_size(size_t payload_length)
{
    return offsetof(struct message, payload) + payload_length;
}
~~~

The structure does not include payload storage. Allocate enough contiguous storage, validate arithmetic overflow, and obey lifetime and alignment rules. The member must be last.

## Memory Functions And Overlap

The standard byte-copy functions have strict contracts:

- memcpy requires non-overlapping regions;
- memmove supports overlap;
- memcmp compares bytes, not semantic values;
- memset writes one repeated byte and is not a universal typed initializer.

For structures with padding, memcmp may report inequality even when named members compare equal.

## Exercises

1. Append bytes to a bounded buffer and test empty, full, and one-byte-free cases.
2. Implement a length-prefixed frame parser rejecting truncated and overlong frames.
3. Create a two-dimensional sample buffer and verify row-major indexing.
4. Copy a string into caller-provided storage and report truncation without losing termination.
5. Demonstrate why sizeof works for a local array but not an array parameter.
6. Design a DMA-buffer contract naming producer, consumer, ownership, alignment, and cache operations.

## Common Mistakes

- Using sizeof(pointer) where the array size was intended.
- Treating an array parameter bound as runtime validation.
- Forgetting a null terminator or writing past capacity.
- Calling string functions on binary or unterminated input.
- Modifying a string literal.
- Passing overlapping objects to memcpy.
- Treating structure layout as a serialized packet format.
- Multiplying dimensions without checking size_t overflow.
- Reading while DMA or another task is writing.
- Assuming memset zero creates valid values for every object type.

## Debugging Checklist

1. Log pointer, length, capacity, and ownership at every boundary.
2. Assert or test length <= capacity before indexing.
3. Test empty, full, maximum, truncated, and unterminated input.
4. Use address and undefined-behavior sanitizers in host builds when possible.
5. Check whether an array has decayed before applying sizeof.
6. Check source/destination overlap for every copy.
7. Verify encoding and terminator policy.
8. On target, inspect DMA descriptors, cache state, alignment, and handoff.

## Related Topics

- [Language Fundamentals overview](./index.md)
- [Types, Values, And Objects](./types-values-and-objects.md)
- [Structures, Unions, And Enumerations](./structures-unions-and-enums.md)
- [Standard Library And Ecosystem](../standard-library-and-ecosystem/index.md)
- [Semantics And Memory Model](../semantics-and-memory/index.md)
- [C Programming](../index.md)

## References

- [ISO/IEC 9899 standards and drafts](https://open-std.org/jtc1/sc22/wg14/www/standards.html)
- [C11 public draft N1570](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n1570.pdf)
- [C string handling — cppreference](https://en.cppreference.com/w/c/string/byte)
- [CERT C array rules](https://wiki.sei.cmu.edu/confluence/display/c)

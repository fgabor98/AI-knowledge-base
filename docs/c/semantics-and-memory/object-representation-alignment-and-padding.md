---
status: draft
reviewed: false
domain: c
difficulty: intermediate
last_reviewed: null
---

# Object Representation, Alignment, And Padding

C values live in object representations: sequences of bytes that occupy storage. The representation of a value is not automatically the same as a wire format, a register layout, or a portable serialization. Alignment and padding also mean that the size and byte layout of a structure are implementation and ABI properties.

## Learning Objectives

- Distinguish a value from its object representation.
- Inspect bytes safely with character types and memcpy.
- Explain alignment and use _Alignof and _Alignas.
- Account for structure padding and tail padding.
- Identify bit-field and endianness assumptions.
- Design representation boundaries that are explicit and testable.

## Values And Object Representations

An object has a size in C bytes and a representation made of bits. Some bit patterns represent values; padding bits or bytes may not participate in the value. Two objects with equal values do not necessarily have byte-for-byte equal representations.

~~~c
#include <stdint.h>
#include <string.h>

uint32_t copy_u32_representation(uint32_t value, unsigned char bytes[sizeof value])
{
    uint32_t copy = 0u;
    memcpy(bytes, &value, sizeof value);
    memcpy(&copy, bytes, sizeof copy);
    return copy;
}
~~~

A character type can inspect the representation of any object. Copying bytes back into an object of the same type with memcpy preserves the representation. The result is still dependent on the implementation’s integer representation and byte order.

Use named fields and explicit encoding for protocols instead of copying a structure’s representation.

## Bytes And CHAR_BIT

The C standard defines a byte as the unit of sizeof. It may contain more than eight bits; CHAR_BIT from limits.h gives the width:

~~~c
#include <limits.h>
#include <stddef.h>
#include <stdio.h>

void print_byte_model(void)
{
    printf("CHAR_BIT=%d, sizeof(unsigned long)=%zu\n",
           CHAR_BIT, sizeof(unsigned long));
}
~~~

Most embedded targets use eight-bit bytes, but portable code should not silently confuse a C byte with an octet. Protocol interfaces normally define octets explicitly and should use an agreed representation.

## Alignment

Every complete object type has an alignment requirement. An object of type T must be stored at an address suitable for that type unless the implementation provides a supported alternative.

~~~c
#include <stdalign.h>
#include <stdint.h>

struct aligned_sample {
    uint8_t flag;
    uint32_t value;
};

_Static_assert(alignof(struct aligned_sample) >= alignof(uint32_t),
               "unexpected alignment");
~~~

C11 provides alignof through stdalign.h and _Alignof directly. An implementation can require stricter alignment for a type than its size suggests.

Misaligned access can trap, require multiple bus operations, or be accepted with a performance penalty. Even on a tolerant CPU, converting an inadequately aligned address to a pointer and dereferencing it violates the type and alignment contract.

Use _Alignas or alignas when a storage area needs stronger alignment:

~~~c
#include <stdalign.h>
#include <stdint.h>

alignas(32) static uint8_t dma_buffer[256];
~~~

The required alignment belongs to the DMA and cache contract, not merely to the C declaration.

## Structure Padding

The implementation may insert padding between members and after the last member:

~~~c
#include <stddef.h>
#include <stdint.h>

struct record {
    uint8_t kind;
    uint32_t sequence;
    uint16_t length;
};

_Static_assert(offsetof(struct record, sequence) >= sizeof(uint8_t),
               "member offset must include the first member");
~~~

Padding allows each member to meet its alignment and allows arrays of the structure to place every element correctly. Reordering members can change size and reduce padding on one ABI, but layout changes can break binary compatibility.

Do not initialize padding by assuming every byte of a structure is meaningful. Do not compare structures with memcmp unless the representation, padding, and member semantics are explicitly part of the contract.

## Endianness

Endianness is the order in which multi-byte values are represented in memory. It is an ABI or platform property, not a portable C expression.

Use explicit encoding:

~~~c
#include <stdint.h>

uint32_t load_le32(const uint8_t bytes[4])
{
    return ((uint32_t)bytes[0])
         | ((uint32_t)bytes[1] << 8)
         | ((uint32_t)bytes[2] << 16)
         | ((uint32_t)bytes[3] << 24);
}

void store_le32(uint8_t bytes[4], uint32_t value)
{
    bytes[0] = (uint8_t)value;
    bytes[1] = (uint8_t)(value >> 8);
    bytes[2] = (uint8_t)(value >> 16);
    bytes[3] = (uint8_t)(value >> 24);
}
~~~

This also makes the field width and byte order testable. For signed values, define the encoding range and conversion policy before casting.

## Bit-Fields

Bit-field allocation order, storage unit, alignment, and straddling are implementation-defined:

~~~c
struct status_bits {
    unsigned ready : 1;
    unsigned error : 1;
    unsigned mode : 3;
};
~~~

Bit-fields can be useful in a compiler- and target-specific register abstraction, but they are poor portable wire-format definitions. Use masks and shifts for protocols and document any implementation ABI on which a hardware map depends.

## Trap And Invalid Representations

Some types or implementations can have object representations that are not valid values of the type. Reading an invalid representation can be undefined behavior or otherwise constrained by the standard. Indeterminate values and uninitialized padding require special care.

Do not:

- read uninitialized storage through a typed lvalue;
- assume arbitrary bytes form a valid pointer, float, enum, or structure state;
- copy untrusted bytes into a typed object and immediately use every member;
- use a byte pattern as a universal initializer.

Validate external bytes first, then decode into typed objects with explicit rules.

## Packed Structures

Compiler packing attributes can remove padding but often create misaligned members:

~~~c
struct __attribute__((packed)) wire_header {
    uint8_t type;
    uint32_t length;
};
~~~

This is a compiler extension and may generate slow or faulting unaligned accesses. A packed declaration does not solve endianness, validation, padding semantics, or lifetime. Prefer byte buffers and decode functions for portable protocols.

If a packed representation is required for a target ABI, copy individual fields with safe accessors and enforce layout with compile-time assertions and tests.

## Representation Boundaries

At every boundary, choose one of these approaches:

- typed object access when both sides share the same C object and lifetime;
- memcpy when copying representation within a known compatible implementation;
- explicit shifts and masks for portable integer encoding;
- serialization functions for persistent, network, or cross-processor formats;
- vendor-defined register accessors for MMIO.

A cast is rarely a complete representation boundary.

## Exercises

1. Print sizeof, alignof, and offsetof for a structure on host and target.
2. Implement little-endian and big-endian encode/decode functions with boundary tests.
3. Compare a structure with padding using member comparisons and then with memcmp; explain the difference.
4. Inspect compiler output for an unaligned packed-member access.
5. Use a static assertion to protect a documented ABI layout.
6. Define a protocol decoder that rejects invalid lengths, reserved bits, and unsupported versions.

## Common Mistakes

- Treating sizeof(struct) as a wire-format size.
- Assuming structure members have no padding.
- Using memcmp for semantic structure equality.
- Assuming host endianness matches protocol endianness.
- Assuming bit-fields map to hardware bits in declaration order.
- Using packed structures to avoid writing a decoder.
- Reading arbitrary bytes as a pointer, float, enum, or structure.
- Confusing C bytes with eight-bit octets.
- Using alignment casts without proving the source address is aligned.
- Treating an all-zero representation as a valid value for every type.

## Debugging Checklist

1. Inspect CHAR_BIT, sizeof, alignof, and offsetof on the actual target.
2. Dump bytes only through unsigned-character access or memcpy.
3. Compare named members rather than padded representations.
4. Check endianness at every external boundary.
5. Inspect generated instructions for unaligned or packed accesses.
6. Verify bit-field layout against compiler and ABI documentation.
7. Add compile-time layout assertions for documented contracts.
8. Validate external bytes before creating typed values.

## Related Topics

- [Semantics And Memory overview](./index.md)
- [Conversions, Promotions, And Aliasing](./conversions-promotions-and-aliasing.md)
- [Memory Layout And Allocation](./memory-layout-and-allocation.md)
- [Structures, Unions, And Enumerations](../language-fundamentals/structures-unions-and-enums.md)
- [Platform-Specific C](../platform-specific-c/index.md)
- [C Programming](../index.md)

## References

- [ISO/IEC 9899 standards and drafts](https://open-std.org/jtc1/sc22/wg14/www/standards.html)
- [C11 public draft N1570](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n1570.pdf)
- [GCC type attributes](https://gcc.gnu.org/onlinedocs/gcc/Type-Attributes.html)
- [GCC alignment options](https://gcc.gnu.org/onlinedocs/gcc/Optimize-Options.html)

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
#include <stdint.h>

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

### Model Answers

Try each exercise before opening its answer. Where an exercise asks you to build
something, the answer shows one good solution, not the only one.

??? note "Exercise 1: sizeof, alignof, and offsetof on host and target"
    [Structures exercise 3](../language-fundamentals/structures-unions-and-enums.md#model-answers)
    does this for one structure on three ABIs: x86-64 and 32-bit x86 measured, 32-bit Arm from
    the AAPCS. The short version of its result:

    | `struct { uint8_t; uint64_t; uint16_t; void *; }` | x86-64 | i386 | Arm AAPCS |
    | --- | --- | --- | --- |
    | offset of the `uint64_t` | 8 | 4 | 8 |
    | `sizeof` | 32 | 20 | 24 |
    | `alignof` | 8 | 4 | 8 |

    Two rules explain every difference: pointer size, and the alignment of 64-bit integers
    inside structures (4 on i386, 8 elsewhere). When the target numbers differ from the host
    numbers, any code that copies the structure between them is wrong.

??? note "Exercise 2: little- and big-endian encoders with boundary tests"
    ```c
    #include <assert.h>
    #include <stddef.h>
    #include <stdint.h>

    uint32_t load_le32(const uint8_t b[4])
    {
        return (uint32_t)b[0] | (uint32_t)b[1] << 8 | (uint32_t)b[2] << 16 | (uint32_t)b[3] << 24;
    }

    uint32_t load_be32(const uint8_t b[4])
    {
        return (uint32_t)b[0] << 24 | (uint32_t)b[1] << 16 | (uint32_t)b[2] << 8 | (uint32_t)b[3];
    }

    void store_le32(uint8_t b[4], uint32_t v)
    {
        b[0] = (uint8_t)v;
        b[1] = (uint8_t)(v >> 8);
        b[2] = (uint8_t)(v >> 16);
        b[3] = (uint8_t)(v >> 24);
    }

    void store_be32(uint8_t b[4], uint32_t v)
    {
        b[0] = (uint8_t)(v >> 24);
        b[1] = (uint8_t)(v >> 16);
        b[2] = (uint8_t)(v >> 8);
        b[3] = (uint8_t)v;
    }

    int main(void)
    {
        static const uint32_t values[] = { 0u, 1u, 0x80u, 0x80000000u, 0xFFFFFFFFu, 0x01020304u };
        uint8_t b[4];

        store_le32(b, 0x01020304u);
        assert(b[0] == 0x04 && b[3] == 0x01);           /* exact bytes, not only round trip */
        store_be32(b, 0x01020304u);
        assert(b[0] == 0x01 && b[3] == 0x04);
        for (size_t i = 0; i < sizeof values / sizeof values[0]; i++) {
            store_le32(b, values[i]);
            assert(load_le32(b) == values[i]);
            store_be32(b, values[i]);
            assert(load_be32(b) == values[i]);
        }
        return 0;
    }
    ```

    `0x01020304` has four different bytes, so it pins down the byte order. `0x80` and
    `0x80000000` catch sign extension and a missing `(uint32_t)` cast. Without the cast,
    `b[3] << 24` is done in `int` and shifting `0x80` there is undefined. The code never
    depends on the host byte order, so the same tests pass on any target.

??? note "Exercise 3: member comparison versus memcmp"
    ```c
    #include <stdio.h>
    #include <string.h>

    struct sample {
        char kind;
        int value;
    };

    static int same_members(const struct sample *a, const struct sample *b)
    {
        return a->kind == b->kind && a->value == b->value;
    }

    int main(void)
    {
        struct sample a;
        struct sample b = { .kind = 'T', .value = 21 };

        memset(&a, 0xff, sizeof a);
        a.kind = 'T';
        a.value = 21;
        printf("members equal: %d, memcmp: %s\n", same_members(&a, &b),
               memcmp(&a, &b, sizeof a) == 0 ? "equal" : "different");
        return 0;
    }
    ```

    GCC 13 on x86-64 prints `members equal: 1, memcmp: different`. `struct sample` has three
    padding bytes after `kind`. In `a` they still hold `0xff` from the `memset`, and in `b`
    they hold whatever the initialization left there. Padding bytes take unspecified values
    (C11 6.2.6.1p6), so `memcmp` compares bytes that are not part of the value. Compare
    structures member by member, or serialize them into a defined byte format first. The
    same unspecified bytes are what leak in
    [case study 10](../failure-case-studies/10-struct-padding-leak.md).

??? note "Exercise 4: compiler output for a packed member"
    <!-- c-check: expect-warning -->

    ```c
    #include <stdint.h>

    struct __attribute__((packed)) wire_header {
        uint8_t type;
        uint32_t length;
    };

    uint32_t read_length(const struct wire_header *header)
    {
        return header->length;                  /* the compiler knows it may be unaligned */
    }

    uint32_t read_through_pointer(const uint32_t *length)
    {
        return *length;                         /* assumes 4-byte alignment */
    }

    uint32_t take_address(const struct wire_header *header)
    {
        return read_through_pointer(&header->length);
    }
    ```

    GCC 13 on x86-64 at `-O2`:

    ```text
    read_length:
    	movl	1(%rdi), %eax
    	ret
    packed.c:20:33: warning: taking address of packed member of 'struct wire_header' may result in an unaligned pointer value [-Waddress-of-packed-member]
    ```

    - Direct member access is safe, because the compiler knows the alignment. x86 handles
      unaligned loads in hardware, so it is one `movl` from offset 1. On a core without
      unaligned access, such as Cortex-M0, GCC emits byte loads and shifts instead (not
      measured here; inspect with `arm-none-eabi-objdump -d`).
    - Taking the member's address loses that knowledge. `read_through_pointer` emits an
      aligned load, which faults on a strict-alignment core. On x86 the misaligned access
      is still undefined, and UBSan reports it:
      `runtime error: load of misaligned address 0x7ffe9c1f913f for type 'const uint32_t', which requires 4 byte alignment`
      (`-fsanitize=alignment`, with the warning suppressed).

??? note "Exercise 5: a static assertion for a documented ABI layout"
    [Structures exercise 6](../language-fundamentals/structures-unions-and-enums.md#model-answers)
    works through a DMA descriptor. The same pattern for a structure shared with a
    bootloader, whose layout is fixed by the bootloader's interface document:

    ```c
    #include <stddef.h>
    #include <stdint.h>

    /* Boot handoff block, interface document rev. C: little-endian, 4-byte aligned. */
    struct boot_handoff {
        uint32_t magic;          /* +0  0x424F4F54 "BOOT" */
        uint32_t version;        /* +4  */
        uint32_t image_address;  /* +8  */
        uint32_t image_size;     /* +12 */
        uint32_t crc32;          /* +16 */
    };

    _Static_assert(offsetof(struct boot_handoff, magic) == 0, "doc rev. C");
    _Static_assert(offsetof(struct boot_handoff, version) == 4, "doc rev. C");
    _Static_assert(offsetof(struct boot_handoff, image_address) == 8, "doc rev. C");
    _Static_assert(offsetof(struct boot_handoff, image_size) == 12, "doc rev. C");
    _Static_assert(offsetof(struct boot_handoff, crc32) == 16, "doc rev. C");
    _Static_assert(sizeof(struct boot_handoff) == 20, "doc rev. C");
    ```

    The numbers come from the document, not from printing `offsetof`. The assertions guard
    the layout, but byte order is not a layout property: they pass on a big-endian target
    that would misread every field. Assert that separately, for example with GCC's
    `__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__`.

??? note "Exercise 6: a protocol decoder that rejects invalid input"
    Header: byte 0 is the version (only 1 is supported), byte 1 holds flags (bits 0–1
    defined, bits 2–7 reserved and must be 0), bytes 2–3 are a big-endian payload length
    of at most 64.

    ```c
    #include <assert.h>
    #include <stddef.h>
    #include <stdint.h>

    #define HEADER_SIZE 4u
    #define PAYLOAD_MAX 64u
    #define FLAGS_DEFINED 0x03u

    enum decode_result { DECODE_OK, DECODE_SHORT, DECODE_VERSION, DECODE_RESERVED, DECODE_LENGTH };

    struct message {
        uint8_t flags;
        const uint8_t *payload;
        size_t length;
    };

    enum decode_result decode(const uint8_t *in, size_t available, struct message *out)
    {
        if (available < HEADER_SIZE) {
            return DECODE_SHORT;
        }
        if (in[0] != 1u) {
            return DECODE_VERSION;
        }
        if ((in[1] & (uint8_t)~FLAGS_DEFINED) != 0u) {
            return DECODE_RESERVED;
        }
        size_t length = (size_t)in[2] << 8 | in[3];
        if (length > PAYLOAD_MAX) {
            return DECODE_LENGTH;
        }
        if (length > available - HEADER_SIZE) {
            return DECODE_SHORT;
        }
        *out = (struct message){ .flags = in[1], .payload = in + HEADER_SIZE, .length = length };
        return DECODE_OK;
    }

    int main(void)
    {
        struct message m;
        const uint8_t ok[] = { 1, 0x01, 0, 2, 'h', 'i' };
        const uint8_t v2[] = { 2, 0, 0, 0 };
        const uint8_t reserved[] = { 1, 0x04, 0, 0 };
        const uint8_t too_long[] = { 1, 0, 0, 65 };
        const uint8_t truncated[] = { 1, 0, 0, 3, 'a' };

        assert(decode(ok, sizeof ok, &m) == DECODE_OK && m.length == 2 && m.payload[1] == 'i');
        assert(decode(ok, 3, &m) == DECODE_SHORT);
        assert(decode(v2, sizeof v2, &m) == DECODE_VERSION);
        assert(decode(reserved, sizeof reserved, &m) == DECODE_RESERVED);
        assert(decode(too_long, sizeof too_long, &m) == DECODE_LENGTH);
        assert(decode(truncated, sizeof truncated, &m) == DECODE_SHORT);
        return 0;
    }
    ```

    The checks run in the order the fields are needed, and each failure has its own result.
    Rejecting set reserved bits, instead of ignoring them, keeps the bits usable later: a
    future version can give them a meaning without old devices silently misreading
    messages. The version is checked before anything else, because a different version may
    have a different header.

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
- [Case study 10: Struct Padding Leak](../failure-case-studies/10-struct-padding-leak.md)
- [Case study 12: Enum Size ABI Drift](../failure-case-studies/12-enum-size-abi-drift.md)

## References

- [ISO/IEC 9899 standards and drafts](https://open-std.org/jtc1/sc22/wg14/www/standards.html)
- [C11 public draft N1570](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n1570.pdf)
- [GCC type attributes](https://gcc.gnu.org/onlinedocs/gcc/Type-Attributes.html)
- [GCC alignment options](https://gcc.gnu.org/onlinedocs/gcc/Optimize-Options.html)

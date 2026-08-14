---
status: draft
reviewed: false
domain: c
difficulty: beginner
last_reviewed: null
---

# Structures, Unions, And Enumerations

Structures group named members into one object. Unions overlay several member interpretations on shared storage. Enumerations give names to integer constants and make state and status code readable. These features are central to drivers and protocols, but their in-memory layout is an implementation contract unless a specification says otherwise.

## Learning Objectives

- Define, initialize, and access structures.
- Use tags, typedef, designated initializers, and opaque declarations.
- Model a variant with a tag and union.
- Use enums for states and status values without assuming storage width.
- Inspect padding and alignment.
- Explain why structures, bit-fields, and unions are not automatically portable wire formats.

## Structure Definitions

~~~c
#include <stdint.h>

struct sensor_sample {
    uint16_t raw;
    int16_t temperature;
    uint8_t status;
};
~~~

An object contains each member, in declaration order, with possible padding:

~~~c
#include <stdint.h>

struct sensor_sample {
    uint16_t raw;
    int16_t temperature;
    uint8_t status;
};

struct sensor_sample sample;

void set_sample(void)
{
    sample.raw = 1000u;
    sample.temperature = 250;
    sample.status = 0u;
}
~~~

The dot operator accesses a member of an object. The arrow operator accesses a member through a pointer:

~~~c
#include <stddef.h>
#include <stdint.h>

struct sensor_sample {
    uint16_t raw;
    int16_t temperature;
    uint8_t status;
};

void clear_sample(struct sensor_sample *sample)
{
    if (sample != NULL) {
        sample->raw = 0u;
        sample->temperature = 0;
        sample->status = 0u;
    }
}
~~~

Structure assignment copies member values. It does not duplicate resources referenced by pointer members. If a structure owns a pointer, define whether assignment is a shallow view or requires a clone function.

## Tags And typedef

The tag and typedef name are separate:

~~~c
#include <stdint.h>

struct device_config {
    uint32_t baud_rate;
};

typedef struct device_config device_config_t;
~~~

Typedefs are useful for opaque handles and callbacks, but avoid aliases that hide pointer depth or ownership.

An opaque structure can be declared in a header and defined only in a source file:

~~~c
struct device;

struct device *device_open(void);
void device_close(struct device *device);
~~~

Callers can hold pointers to the incomplete type but cannot access members. This preserves representation freedom.

## Initialization And Designated Members

Designated initializers name members explicitly:

~~~c
#include <stdint.h>

struct sensor_sample {
    uint16_t raw;
    int16_t temperature;
    uint8_t status;
};

static const struct sensor_sample default_sample = {
    .raw = 0u,
    .temperature = 0,
    .status = 0u
};

static const struct sensor_sample alarm_sample = {
    .status = 1u,
    .temperature = 850
};
~~~

Members not named are initialized as zero for this initialization. Designators make intent robust when fields are reordered or added.

Nested aggregates can be initialized clearly:

~~~c
#include <stdint.h>

struct calibration {
    int16_t offset;
    uint16_t scale;
};

struct channel {
    uint8_t id;
    struct calibration calibration;
};

static const struct channel channel0 = {
    .id = 0u,
    .calibration = {
        .offset = -3,
        .scale = 1024u
    }
};
~~~

## Enums

An enum declares named integer constants:

~~~c
enum boot_phase {
    BOOT_RESET,
    BOOT_CLOCKS_READY,
    BOOT_DRIVERS_READY,
    BOOT_APPLICATION
};
~~~

Successive values start at zero and increment by one unless explicitly set:

~~~c
enum status_code {
    STATUS_OK = 0,
    STATUS_INVALID = 1,
    STATUS_TIMEOUT = 2,
    STATUS_IO = 5
};
~~~

Enumeration constants have integer types, while an object declared with an enum type has an implementation-defined compatible integer type. Do not assume an enum object is one byte, can represent every arbitrary integer, or can be serialized by copying its bytes.

Validate untrusted numeric input before converting it to a state enum. A cast can be syntactically accepted even when the result is not a state the logic handles.

## Tagged Unions

A union stores members in overlapping storage. A tag records which interpretation is active:

~~~c
#include <stdint.h>

enum value_kind {
    VALUE_INTEGER,
    VALUE_FLOAT
};

struct value {
    enum value_kind kind;
    union {
        int32_t integer;
        float real;
    } data;
};

static struct value make_integer(int32_t integer)
{
    return (struct value){
        .kind = VALUE_INTEGER,
        .data.integer = integer
    };
}
~~~

Consumers should switch on the tag:

~~~c
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum value_kind {
    VALUE_INTEGER,
    VALUE_FLOAT
};

struct value {
    enum value_kind kind;
    union {
        int32_t integer;
        float real;
    } data;
};

bool value_is_positive(const struct value *value)
{
    if (value == NULL) {
        return false;
    }

    switch (value->kind) {
    case VALUE_INTEGER:
        return value->data.integer > 0;
    case VALUE_FLOAT:
        return value->data.real > 0.0f;
    }

    return false;
}
~~~

Do not use a union as a general license for type-punning. For representation conversions, use explicit byte operations or memcpy into a destination object and document the target representation.

## Layout, Padding, And Alignment

The implementation may insert padding:

~~~c
#include <stddef.h>
#include <stdint.h>

struct layout_example {
    uint8_t flag;
    uint32_t value;
    uint16_t count;
};

size_t value_offset(void)
{
    return offsetof(struct layout_example, value);
}
~~~

Offsets and total size depend on alignment rules and ABI. Reordering members can reduce padding on one target, but do not change a public or persistent structure without considering compatibility.

A structure with padding may contain unspecified padding bytes; comparing its raw representation with memcmp is not the same as comparing members.

For a binary protocol, use encode/decode functions with explicit widths and byte order. For a memory-mapped register block, use documented target definitions and verify access width, alignment, volatility, and reserved bits.

## Bit-Fields

Bit-fields can express packed fields:

~~~c
struct flags {
    unsigned ready : 1;
    unsigned error : 1;
    unsigned mode : 3;
};
~~~

Allocation order, alignment, whether fields straddle storage units, and relationship to byte order are implementation-defined. They are risky for portable protocol formats and hardware registers. Prefer masks and shifts when representation must be exact.

If a project uses bit-fields for a target ABI, treat compiler, options, ABI, and layout checks as part of the contract.

## Copying And Ownership

Structure assignment copies member values:

~~~c
#include <stdint.h>

struct sensor_sample {
    uint16_t raw;
    int16_t temperature;
    uint8_t status;
};

void copy_sample(void)
{
    struct sensor_sample a = {0};
    struct sensor_sample b = a;
    (void)b;
}
~~~

For pointer members, it copies the address, not the pointed-to object:

~~~c
#include <stddef.h>
#include <stdint.h>

struct message_view {
    const uint8_t *data;
    size_t length;
};
~~~

This is naturally a non-owning view. An owning message type needs an explicit allocation, copy, and destruction policy.

## Embedded Register Maps

A register map is not merely a structure with guessed fields. Correctness depends on register offsets and access widths, reserved and write-one-to-clear bits, read side effects, volatile access, reset values, ordering, barriers, compiler, and ABI.

Use the vendor header or a reviewed target-specific abstraction. Do not cast arbitrary packet bytes into a register structure and assume the layout matches.

## Exercises

1. Define a configuration structure with designated initializers and validate every field.
2. Create a tagged union for a command containing either an integer or byte span. Test every tag and an invalid tag.
3. Print sizeof and offsetof on host and target, then explain differences.
4. Replace a protocol structure cast with explicit encode/decode functions.
5. Test an enum state machine with an out-of-range numeric input.
6. Use static assertions to protect a target layout only after verifying its documented ABI.

## Common Mistakes

- Treating structure bytes as a portable serialization format.
- Assuming no padding exists.
- Using memcmp on structures with padding or pointer members.
- Reading a union member without a valid active-member design.
- Assuming enum storage width or range.
- Using bit-fields for protocol or register layout without a contract.
- Copying a structure with pointers and accidentally sharing ownership.
- Exposing a private structure definition unnecessarily.
- Updating a union without updating its tag.
- Relying on default packing options.

## Debugging Checklist

1. Print sizeof and offsetof for layouts crossing binary boundaries.
2. Compare named members, not raw padded bytes, unless representation is defined.
3. Validate enum values before state dispatch.
4. Verify the active union tag at every read.
5. Inspect compiler ABI and packing options.
6. Check pointer members for ownership, lifetime, and aliasing.
7. For registers, compare generated accesses with the reference manual and disassembly.
8. Add compile-time layout assertions only for documented target contracts.

## Related Topics

- [Language Fundamentals overview](./index.md)
- [Types, Values, And Objects](./types-values-and-objects.md)
- [Initialization](./initialization.md)
- [Semantics And Memory Model](../semantics-and-memory/index.md)
- [Platform-Specific C](../platform-specific-c/index.md)
- [C Programming](../index.md)

## References

- [ISO/IEC 9899 standards and drafts](https://open-std.org/jtc1/sc22/wg14/www/standards.html)
- [C11 public draft N1570](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n1570.pdf)
- [GCC type attributes](https://gcc.gnu.org/onlinedocs/gcc/Type-Attributes.html)
- [CERT C coding rules](https://wiki.sei.cmu.edu/confluence/display/c)

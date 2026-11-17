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

### Model Answers

Try each exercise before opening its answer. Where an exercise asks you to build
something, the answer shows one good solution, not the only one.

??? note "Exercise 1: a validated configuration structure"
    ```c
    #include <assert.h>
    #include <stdbool.h>
    #include <stdint.h>

    enum uart_parity { PARITY_NONE, PARITY_EVEN, PARITY_ODD };

    struct uart_config {
        uint32_t baud;
        uint8_t data_bits;
        enum uart_parity parity;
        uint8_t stop_bits;
        uint16_t rx_timeout_ms;
    };

    static const struct uart_config uart_config_default = {
        .baud = 115200u,
        .data_bits = 8u,
        .parity = PARITY_NONE,
        .stop_bits = 1u,
        .rx_timeout_ms = 20u,
    };

    enum config_error {
        CONFIG_OK,
        CONFIG_BAD_BAUD,
        CONFIG_BAD_DATA_BITS,
        CONFIG_BAD_PARITY,
        CONFIG_BAD_STOP_BITS,
        CONFIG_BAD_TIMEOUT
    };

    /* Checks every field; reports the first one that is invalid. */
    enum config_error uart_config_check(const struct uart_config *config)
    {
        static const uint32_t supported_baud[] = { 9600u, 19200u, 57600u, 115200u, 921600u };
        bool baud_ok = false;

        for (unsigned int i = 0; i < sizeof supported_baud / sizeof supported_baud[0]; i++) {
            baud_ok = baud_ok || config->baud == supported_baud[i];
        }
        if (!baud_ok) {
            return CONFIG_BAD_BAUD;
        }
        if (config->data_bits != 7u && config->data_bits != 8u) {
            return CONFIG_BAD_DATA_BITS;
        }
        switch (config->parity) {           /* an enum field can still hold any int value */
        case PARITY_NONE:
        case PARITY_EVEN:
        case PARITY_ODD:
            break;
        default:
            return CONFIG_BAD_PARITY;
        }
        if (config->stop_bits != 1u && config->stop_bits != 2u) {
            return CONFIG_BAD_STOP_BITS;
        }
        if (config->rx_timeout_ms == 0u || config->rx_timeout_ms > 1000u) {
            return CONFIG_BAD_TIMEOUT;
        }
        return CONFIG_OK;
    }

    int main(void)
    {
        struct uart_config config = uart_config_default;

        assert(uart_config_check(&config) == CONFIG_OK);
        config.baud = 115201u;
        assert(uart_config_check(&config) == CONFIG_BAD_BAUD);
        config = uart_config_default;
        config.data_bits = 9u;
        assert(uart_config_check(&config) == CONFIG_BAD_DATA_BITS);
        config = uart_config_default;
        config.parity = (enum uart_parity)7;    /* e.g. a corrupted byte from flash */
        assert(uart_config_check(&config) == CONFIG_BAD_PARITY);
        config = uart_config_default;
        config.stop_bits = 0u;
        assert(uart_config_check(&config) == CONFIG_BAD_STOP_BITS);
        config = uart_config_default;
        config.rx_timeout_ms = 0u;
        assert(uart_config_check(&config) == CONFIG_BAD_TIMEOUT);
        return 0;
    }
    ```

    - The default is written with designators, so it is correct regardless of member order.
    - Every field has a rule, and each rule describes what the *hardware* supports: a list
      of baud rates, not "non-zero".
    - The `enum` field is validated too. A value loaded from storage or a message can be any
      integer, and the `default` case catches it.
    - Each test changes exactly one field of a valid configuration, so a failure points at
      one rule. Returning the first bad field gives the log a useful message.

??? note "Exercise 2: a tagged union for a command"
    ```c
    #include <assert.h>
    #include <stddef.h>
    #include <stdint.h>

    enum command_kind { COMMAND_SET_LEVEL, COMMAND_WRITE_BYTES };

    struct byte_span {
        const uint8_t *data;
        size_t length;
    };

    struct command {
        enum command_kind kind;
        union {
            int32_t level;              /* valid when kind == COMMAND_SET_LEVEL */
            struct byte_span bytes;     /* valid when kind == COMMAND_WRITE_BYTES */
        } as;
    };

    enum command_result { COMMAND_DONE, COMMAND_REJECTED };

    static int32_t current_level;
    static size_t bytes_written;

    enum command_result command_execute(const struct command *command)
    {
        switch (command->kind) {
        case COMMAND_SET_LEVEL:
            if (command->as.level < 0 || command->as.level > 100) {
                return COMMAND_REJECTED;
            }
            current_level = command->as.level;
            return COMMAND_DONE;
        case COMMAND_WRITE_BYTES:
            if (command->as.bytes.data == NULL && command->as.bytes.length != 0u) {
                return COMMAND_REJECTED;
            }
            bytes_written += command->as.bytes.length;
            return COMMAND_DONE;
        default:
            return COMMAND_REJECTED;    /* unknown tag: never read the union */
        }
    }

    int main(void)
    {
        static const uint8_t payload[3] = { 1, 2, 3 };
        struct command set = { .kind = COMMAND_SET_LEVEL, .as.level = 42 };
        struct command write = { .kind = COMMAND_WRITE_BYTES,
                                 .as.bytes = { payload, sizeof payload } };
        struct command bad_tag = { .kind = (enum command_kind)9 };
        struct command bad_span = { .kind = COMMAND_WRITE_BYTES, .as.bytes = { NULL, 4 } };

        assert(command_execute(&set) == COMMAND_DONE && current_level == 42);
        assert(command_execute(&write) == COMMAND_DONE && bytes_written == 3);
        assert(command_execute(&bad_tag) == COMMAND_REJECTED);
        assert(command_execute(&bad_span) == COMMAND_REJECTED);
        assert(current_level == 42 && bytes_written == 3);    /* rejected commands change nothing */
        return 0;
    }
    ```

    - Each union member's comment names the tag that makes it valid. Code reads a member
      only inside the `case` for that tag.
    - The `default` case rejects unknown tags *before* any union member is read, because the
      union's contents mean nothing without a valid tag.
    - The span is a borrowed view: the command does not own `payload`, so the caller must
      keep it alive until `command_execute` returns.
    - The tests cover each valid tag, an invalid tag, and an invalid payload within a valid
      tag, and check that rejection has no side effects.

??? note "Exercise 3: sizeof and offsetof on different ABIs"
    ```c
    #include <stddef.h>
    #include <stdint.h>
    #include <stdio.h>

    struct record {
        uint8_t kind;
        uint64_t timestamp;
        uint16_t id;
        void *payload;
    };

    int main(void)
    {
        printf("kind      %zu\n", offsetof(struct record, kind));
        printf("timestamp %zu\n", offsetof(struct record, timestamp));
        printf("id        %zu\n", offsetof(struct record, id));
        printf("payload   %zu\n", offsetof(struct record, payload));
        printf("size      %zu\n", sizeof(struct record));
        printf("align     %zu\n", _Alignof(struct record));
        return 0;
    }
    ```

    The first two columns were measured with GCC 13 (`-m32` compiled to assembly, since
    32-bit libraries are not installed here). The Arm column comes from the AAPCS rules.

    | | x86-64 Linux | 32-bit x86 Linux (`-m32`) | 32-bit Arm (AAPCS) |
    | --- | --- | --- | --- |
    | `kind` | 0 | 0 | 0 |
    | `timestamp` | 8 | **4** | 8 |
    | `id` | 16 | 12 | 16 |
    | `payload` | 24 | 16 | **20** |
    | `sizeof` | 32 | **20** | **24** |
    | `_Alignof` | 8 | 4 | 8 |

    The differences come from two ABI rules:

    - **Pointer size.** 8 bytes on x86-64, 4 on both 32-bit ABIs, so `payload` takes less
      room.
    - **Alignment of 64-bit integers inside structures.** x86-64 and AAPCS align `uint64_t`
      to 8 bytes, so `timestamp` starts at 8 after 7 bytes of padding. The i386 System V ABI
      aligns it to only 4 inside a structure, so it starts at 4.

    Three platforms give three layouts for the same source. That is why a structure cannot
    be a file or wire format, and why a host-side tool cannot read target memory using the
    host's view of the structure. Ordering members from largest to smallest (`timestamp`,
    `payload`, `id`, `kind`) removes most of the padding on all three.

??? note "Exercise 4: explicit encode and decode instead of a structure cast"
    Before: the buffer is cast to a structure.

    <!-- c-check: skip -- deliberately wrong fragment; see the case study -->

    ```c
    struct header {
        uint8_t type;
        uint8_t flags;
        uint16_t length;        /* big-endian on the wire */
        uint32_t sequence;      /* big-endian on the wire */
    };

    const struct header *h = (const struct header *)rx_buffer;   /* layout, alignment, aliasing */
    uint16_t length = ntohs(h->length);
    ```

    This depends on the structure having no padding, on `rx_buffer` being 4-byte aligned,
    and on the effective-type rules allowing the access. None of these is guaranteed.
    [Case study 08](../failure-case-studies/08-strict-aliasing-type-pun.md) shows how it
    breaks. It also never checks how many bytes were received.

    After:

    ```c
    #include <assert.h>
    #include <stddef.h>
    #include <stdint.h>

    #define HEADER_WIRE_SIZE 8u

    struct header {
        uint8_t type;
        uint8_t flags;
        uint16_t length;
        uint32_t sequence;
    };

    static uint16_t get_be16(const uint8_t *p)
    {
        return (uint16_t)((uint16_t)p[0] << 8 | p[1]);
    }

    static uint32_t get_be32(const uint8_t *p)
    {
        return (uint32_t)p[0] << 24 | (uint32_t)p[1] << 16 | (uint32_t)p[2] << 8 | p[3];
    }

    static void put_be16(uint8_t *p, uint16_t value)
    {
        p[0] = (uint8_t)(value >> 8);
        p[1] = (uint8_t)value;
    }

    static void put_be32(uint8_t *p, uint32_t value)
    {
        p[0] = (uint8_t)(value >> 24);
        p[1] = (uint8_t)(value >> 16);
        p[2] = (uint8_t)(value >> 8);
        p[3] = (uint8_t)value;
    }

    /* Returns 0 on success, -1 if fewer than HEADER_WIRE_SIZE bytes are available. */
    int header_decode(struct header *out, const uint8_t *in, size_t available)
    {
        if (available < HEADER_WIRE_SIZE) {
            return -1;
        }
        out->type = in[0];
        out->flags = in[1];
        out->length = get_be16(&in[2]);
        out->sequence = get_be32(&in[4]);
        return 0;
    }

    void header_encode(uint8_t out[HEADER_WIRE_SIZE], const struct header *in)
    {
        out[0] = in->type;
        out[1] = in->flags;
        put_be16(&out[2], in->length);
        put_be32(&out[4], in->sequence);
    }

    int main(void)
    {
        const uint8_t wire[] = { 0x01, 0x80, 0x00, 0x10, 0x12, 0x34, 0x56, 0x78 };
        struct header header;
        uint8_t again[HEADER_WIRE_SIZE];

        assert(header_decode(&header, wire, 7) == -1);              /* short input */
        assert(header_decode(&header, wire, sizeof wire) == 0);
        assert(header.type == 0x01 && header.flags == 0x80);
        assert(header.length == 0x0010 && header.sequence == 0x12345678u);
        header_encode(again, &header);
        for (size_t i = 0; i < sizeof wire; i++) {
            assert(again[i] == wire[i]);                            /* exact round trip */
        }
        return 0;
    }
    ```

    The decoder reads bytes one at a time, so it works for any alignment, any padding, and
    either host byte order. The wire size is a named constant, not `sizeof(struct header)`.
    The length check comes first. GCC at `-O2` usually turns `get_be32` into one load plus a
    byte-swap instruction, so the explicit version is rarely slower.

??? note "Exercise 5: an enum state machine with out-of-range input"
    ```c
    #include <assert.h>
    #include <stdbool.h>
    #include <stdint.h>

    enum motor_state { MOTOR_STOPPED, MOTOR_STARTING, MOTOR_RUNNING, MOTOR_FAULT };

    /* Converts a stored or received byte; rejects values that are not states. */
    bool motor_state_from_byte(uint8_t raw, enum motor_state *state)
    {
        switch (raw) {
        case MOTOR_STOPPED:
        case MOTOR_STARTING:
        case MOTOR_RUNNING:
        case MOTOR_FAULT:
            *state = (enum motor_state)raw;
            return true;
        default:
            return false;
        }
    }

    enum motor_state motor_step(enum motor_state state, bool start_request, bool at_speed)
    {
        switch (state) {
        case MOTOR_STOPPED:
            return start_request ? MOTOR_STARTING : MOTOR_STOPPED;
        case MOTOR_STARTING:
            return at_speed ? MOTOR_RUNNING : MOTOR_STARTING;
        case MOTOR_RUNNING:
            return start_request ? MOTOR_RUNNING : MOTOR_STOPPED;
        case MOTOR_FAULT:
            return MOTOR_FAULT;                         /* only a reset leaves FAULT */
        default:
            return MOTOR_FAULT;                         /* not a state: fail safe */
        }
    }

    int main(void)
    {
        enum motor_state state = MOTOR_RUNNING;

        assert(motor_state_from_byte(2, &state) && state == MOTOR_RUNNING);
        assert(!motor_state_from_byte(4, &state) && state == MOTOR_RUNNING);   /* rejected, unchanged */
        assert(!motor_state_from_byte(255, &state));

        /* Defense in depth: even if a bad value gets in, the step function fails safe. */
        assert(motor_step((enum motor_state)4, true, true) == MOTOR_FAULT);
        assert(motor_step((enum motor_state)-1, false, false) == MOTOR_FAULT);

        assert(motor_step(MOTOR_STOPPED, true, false) == MOTOR_STARTING);
        assert(motor_step(MOTOR_STARTING, true, true) == MOTOR_RUNNING);
        return 0;
    }
    ```

    - The boundary between untrusted numbers and the enum is one function, which accepts
      only listed values. `(enum motor_state)raw` without the check compiles silently.
    - The first invalid value (4, one past the last state) and an extreme value (255) are
      the inputs to test.
    - `motor_step` still has a `default` that goes to the safe state, because memory
      corruption or a missed conversion can bypass the first check. GCC 13 reports nothing
      for these out-of-range values, even built with `-fsanitize=undefined,enum`, so only
      tests like these catch them.

??? note "Exercise 6: static assertions after checking the ABI"
    A DMA descriptor whose layout is set by the hardware, not by the compiler. The
    reference manual says: four 32-bit words, in this order, and the descriptor must be
    16-byte aligned.

    ```c
    #include <stdalign.h>
    #include <stddef.h>
    #include <stdint.h>

    struct dma_descriptor {
        uint32_t source;        /* word 0: source address */
        uint32_t destination;   /* word 1: destination address */
        uint32_t control;       /* word 2: length and flags */
        uint32_t next;          /* word 3: next descriptor, or 0 */
    };

    /* Layout required by the DMA controller (reference manual, descriptor format). */
    _Static_assert(offsetof(struct dma_descriptor, source) == 0, "word 0");
    _Static_assert(offsetof(struct dma_descriptor, destination) == 4, "word 1");
    _Static_assert(offsetof(struct dma_descriptor, control) == 8, "word 2");
    _Static_assert(offsetof(struct dma_descriptor, next) == 12, "word 3");
    _Static_assert(sizeof(struct dma_descriptor) == 16, "no trailing padding");

    alignas(16) struct dma_descriptor descriptors[4];
    ```

    The order of work matters:

    1. **Find the required layout in the specification**: the reference manual for a
       hardware structure, or the protocol document. That is the source of the numbers.
    2. **Check that the ABI can produce it.** On AAPCS, `uint32_t` has size and alignment 4,
       so four of them have no padding. If the specification needed a `uint32_t` at offset 2,
       no ordinary structure could express it, and the answer is byte-level encoding, not
       `packed`.
    3. **Write the assertions from the specification**, then compile for the real target.

    The mistake to avoid is printing `offsetof` on the host and copying the numbers into
    assertions. That only freezes the host's layout, and the assertions pass even if the
    layout is wrong for the hardware. Alignment is not a size property, so it is enforced
    separately with `alignas(16)` on the objects. The assertions fail the build if someone
    later inserts a field or changes a type.

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

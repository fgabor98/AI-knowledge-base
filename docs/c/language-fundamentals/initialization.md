---
status: draft
reviewed: false
domain: c
difficulty: beginner
last_reviewed: null
---

# Initialization

Initialization gives an object its first value. Assignment changes an existing object. The distinction controls whether an object is valid to read, whether a value can be computed before startup, and how embedded startup code moves data into RAM.

Make initialization explicit at interfaces and state boundaries. “It happens to be zero on this board” is not a sufficient contract unless the language, startup code, and hardware all guarantee it.

## Learning Objectives

- Distinguish initialization from assignment.
- Explain static, automatic, allocated, and thread storage initialization.
- Use scalar, array, structure, union, designated, and compound initializers.
- Reason about omitted members and partial initialization.
- Understand the data and bss startup model.
- Identify why memset is not a universal initializer.
- Avoid initialization-order assumptions across modules.

## Initialization Versus Assignment

An initializer is part of a declaration:

~~~c
#include <stdint.h>

void update_retry_limit(void)
{
    uint32_t retry_limit = 3u;
    retry_limit = 4u;
    (void)retry_limit;
}
~~~

Initialization establishes the first value. A const object can be initialized but cannot later be assigned:

~~~c
#include <stdint.h>

const uint32_t protocol_version = 2u;
~~~

An assignment requires a modifiable lvalue and a live object.

## Storage Duration And Default Initialization

| Storage duration | Typical declaration | If no initializer is supplied |
| --- | --- | --- |
| Automatic | local non-static variable | Indeterminate value |
| Static | file-scope object or local static | Zero-initialized before startup |
| Allocated | storage returned by an allocator | Bytes are not automatically initialized |
| Thread | thread-local object | Declaration and runtime rules apply |

A local automatic object without an initializer cannot be read safely:

~~~c
#include <stdint.h>

uint32_t safe_counter(void)
{
    uint32_t counter;
    counter = 0u;
    return counter;
}
~~~

The example is safe because counter is assigned before it is read. Debug builds must not be used as evidence that accidental stack contents are valid.

Static-storage objects are initialized before program startup. Without an initializer, arithmetic types become zero, pointers become a null pointer value, and aggregates are initialized recursively. A null pointer’s representation is not required to be all-bits-zero.

## Constant Initialization And Startup

Static-storage initializers use permitted constant expressions and aggregate forms:

~~~c
#include <stdint.h>

static const uint8_t calibration_table[] = {0u, 10u, 20u, 30u};
static uint32_t boot_flags;
static uint32_t retry_limit = 3u;
~~~

A typical embedded image contains:

- text and read-only data in nonvolatile memory;
- data initial values in the image, copied to RAM by startup;
- bss objects represented by a size and zeroed in RAM by startup.

Exact section names and startup sequence are toolchain and linker-script details. A freestanding startup may configure clocks, stack, memory protection, caches, floating point, and the runtime environment before C entry.

If startup fails to copy data or clear bss, correct C source can still observe invalid state. Treat startup and linker scripts as part of the language/runtime boundary.

## Scalar Initialization

Use an initializer appropriate to the type:

~~~c
#include <stdbool.h>
#include <stdint.h>

static uint32_t timeout_ticks = 1000u;
static bool enabled = true;
static float gain = 1.5f;
static const char terminator = '\0';
~~~

The compiler converts the initializer to the destination type. Narrowing can lose information:

<!-- c-check: expect-warning -->

~~~c
#include <stdint.h>

static uint8_t truncated = 300u;
~~~

For safety-critical code, make range checks and conversions explicit.

## Array Initialization

An array initializer can infer its bound:

~~~c
#include <stdint.h>

static const uint8_t preamble[] = {0xA5u, 0x5Au, 0x01u};
static uint16_t samples[4] = {100u, 200u};
static char label[8] = "run";
~~~

Remaining sample elements are zero. Designated initializers make sparse tables readable:

~~~c
#include <stdint.h>

static const uint8_t lookup[16] = {
    [0] = 0x10u,
    [3] = 0x30u,
    [15] = 0xF0u
};
~~~

Unspecified elements are initialized to zero.

A character array can have exactly enough elements for visible characters but no terminator:

~~~c
char exact[3] = "abc";
~~~

This is a valid array but not a C string.

## Structure Initialization

Designated initialization makes configuration intent visible:

~~~c
#include <stdbool.h>
#include <stdint.h>

struct uart_config {
    uint32_t baud;
    uint8_t data_bits;
    uint8_t stop_bits;
    bool parity;
};

static const struct uart_config default_uart = {
    .baud = 115200u,
    .data_bits = 8u,
    .stop_bits = 1u,
    .parity = false
};
~~~

Unspecified members are initialized recursively to zero. Later structure assignment copies member values; pointer members are copied as addresses and do not duplicate pointed-to storage.

## Union Initialization

A union initializer initializes one member, normally using a designator:

~~~c
#include <stdint.h>

union register_value {
    uint32_t word;
    uint16_t halfwords[2];
};

static const union register_value reset_value = {
    .word = 0u
};
~~~

Use a tag alongside a union when the active interpretation is program state. Do not assume initializing one member creates a portable representation for reading another.

## Compound Literals

A compound literal creates an unnamed object of a specified type:

~~~c
#include <stdint.h>

struct point {
    int16_t x;
    int16_t y;
};

void draw_point(struct point point);

void draw_origin(void)
{
    draw_point((struct point){.x = 0, .y = 0});
}
~~~

At block scope it has automatic storage duration until the end of the enclosing block; at file scope it has static storage duration. Do not return a pointer to a block-scope compound literal.

## Variable-Length Arrays And Flexible Members

A variable-length array has a runtime bound and normally automatic storage duration. Its stack budget and maximum bound must be reviewed, and it cannot be treated like a fixed-size initialized array.

A flexible array member contributes no storage to its enclosing structure. Allocate enough trailing storage and initialize the fixed members explicitly; trailing bytes need their own copy or fill operation.

## Why memset Is Not Universal Initialization

This is byte-oriented initialization:

~~~c
#include <stddef.h>
#include <stdint.h>
#include <string.h>

void clear_bytes(uint8_t *bytes, size_t length)
{
    if (bytes != NULL) {
        memset(bytes, 0, length);
    }
}
~~~

It is not a universal typed initializer:

- all-bits-zero need not be a null pointer representation;
- a floating zero and a zero byte pattern are not universally identical;
- padding bytes may remain unspecified;
- structures may contain members needing nonzero defaults;
- memory-mapped registers may have write side effects;
- memset establishes neither synchronization nor ownership.

Use typed initializers for typed objects and memset only for an explicitly byte-oriented contract.

## Initialization Order

C has no language-level constructor system that runs arbitrary code for all global objects. Static objects receive language-defined initial values before startup; runtime initialization order is chosen by the program or framework.

Do not rely on link order between modules. Use an explicit sequence:

~~~c
int clock_init(void);
int gpio_init(void);
int sensor_init(void);

int system_init(void)
{
    if (clock_init() != 0) {
        return -1;
    }
    if (gpio_init() != 0) {
        return -2;
    }
    if (sensor_init() != 0) {
        return -3;
    }
    return 0;
}
~~~

If initialization may run more than once, define whether it is idempotent, rejects repetition, or restarts the subsystem.

## Reset And Reinitialization

Power-on reset, warm reset, watchdog reset, and bootloader handoff may leave different RAM and peripheral state. Retained memory must be validated rather than treated as ordinary initialized configuration.

Make reset behavior explicit:

- store a version and integrity check with retained data;
- do not trust arbitrary RAM contents;
- reconfigure peripherals whose reset state is not guaranteed;
- clear security-sensitive state on every appropriate reset;
- test every reset class supported by the product.

## Exercises

1. Distinguish an automatic object assigned before use from one read before assignment.
2. Define a sparse lookup table and verify unspecified elements.
3. Create a configuration structure with a safe default and validation.
4. Show why an exactly sized character array is not null-terminated.
5. Replace a memset of a typed structure with a designated initializer and document the difference.
6. Trace target startup and linker map to identify data copying and bss clearing.
7. Define explicit initialization order for clock, GPIO, communication, and sensor subsystems.

### Model Answers

Try each exercise before opening its answer. Where an exercise asks you to build
something, the answer shows one good solution, not the only one.

??? note "Exercise 1: assigned before use versus read before assignment"
    <!-- c-check: skip -- deliberate uninitialized reads; GCC reports them only when it generates code, which the syntax-only check does not do -->

    ```c
    #include <stdbool.h>

    int read_sensor(void);

    int assigned_first(void)
    {
        int value;

        value = read_sensor();          /* assigned before any read */
        return value + 1;
    }

    int read_first(void)
    {
        int value;

        return value + 1;               /* read before any assignment */
    }

    int assigned_on_one_path(bool ready)
    {
        int value;

        if (ready) {
            value = read_sensor();
        }
        return value;                   /* indeterminate when ready is false */
    }
    ```

    ```text
    $ gcc -std=c17 -O0 -Wall -c uninit.c
    uninit.c:17:18: warning: ‘value’ is used uninitialized [-Wuninitialized]
    $ gcc -std=c17 -O2 -Wall -c uninit.c
    uninit.c:17:18: warning: ‘value’ is used uninitialized [-Wuninitialized]
    uninit.c:27:12: warning: ‘value’ may be used uninitialized [-Wmaybe-uninitialized]
    ```

    - `assigned_first` is fine: an automatic object needs no initializer if every path
      assigns it before reading.
    - `read_first` reads an indeterminate value. Because `value`'s address is never taken,
      the read is undefined behavior (C11 6.3.2.1p2), not merely "some garbage value".
    - `assigned_on_one_path` is the realistic bug, and GCC 13 reports it only at `-O2`: the
      `may be used uninitialized` analysis runs as part of optimization. A debug-only
      warnings build misses it. Initialize at the declaration (`int value = 0;`) when no
      better value exists, or restructure so every path assigns.

??? note "Exercise 2: a sparse lookup table"
    ```c
    #include <stddef.h>
    #include <stdint.h>
    #include <stdio.h>

    enum command { CMD_PING = 0x01, CMD_RESET = 0x05, CMD_READ = 0x10, CMD_WRITE = 0x11, CMD_LIMIT = 0x20 };

    typedef int (*command_handler)(const uint8_t *payload, size_t length);

    static int handle_ping(const uint8_t *payload, size_t length) { (void)payload; (void)length; return 0; }
    static int handle_reset(const uint8_t *payload, size_t length) { (void)payload; (void)length; return 0; }
    static int handle_read(const uint8_t *payload, size_t length) { (void)payload; (void)length; return 0; }

    static const command_handler handlers[CMD_LIMIT] = {
        [CMD_PING] = handle_ping,
        [CMD_RESET] = handle_reset,
        [CMD_READ] = handle_read,
        /* CMD_WRITE intentionally missing to show the check */
    };

    int main(void)
    {
        const enum command expected[] = { CMD_PING, CMD_RESET, CMD_READ, CMD_WRITE };
        size_t present = 0;

        for (size_t i = 0; i < CMD_LIMIT; i++) {
            present += handlers[i] != NULL;
        }
        printf("%zu of %d slots have a handler; the rest are null pointers\n", present, CMD_LIMIT);
        for (size_t i = 0; i < sizeof expected / sizeof expected[0]; i++) {
            if (handlers[expected[i]] == NULL) {
                printf("missing handler for command 0x%02x\n", (unsigned)expected[i]);
            }
        }
        return 0;
    }
    ```

    ```text
    3 of 32 slots have a handler; the rest are null pointers
    missing handler for command 0x11
    ```

    Every element without a designator is initialized as if it had static storage duration:
    null for pointers, zero for numbers (C11 6.7.9p10, p21). So the table is well defined,
    and dispatch code must treat a null entry as "unknown command". The check that matters
    is the second loop: a test that every command the protocol defines has a handler. A
    missing designator compiles silently.

??? note "Exercise 3: a configuration structure with a safe default"
    ```c
    #include <stdbool.h>
    #include <stddef.h>
    #include <stdint.h>

    struct sensor_config {
        uint32_t sample_period_ms;
        uint16_t alarm_threshold;
        uint8_t channel;
        bool filter_enabled;
    };

    /* The one place the defaults are written down. */
    static const struct sensor_config sensor_config_default = {
        .sample_period_ms = 100u,
        .alarm_threshold = 800u,
        .channel = 0u,
        .filter_enabled = true,
    };

    static bool sensor_config_valid(const struct sensor_config *config)
    {
        return config->sample_period_ms >= 10u && config->sample_period_ms <= 60000u
            && config->alarm_threshold <= 1023u
            && config->channel < 4u;
    }

    /* Use stored settings only if they are valid; otherwise fall back as a whole. */
    struct sensor_config sensor_config_load(const struct sensor_config *stored)
    {
        if (stored != NULL && sensor_config_valid(stored)) {
            return *stored;
        }
        return sensor_config_default;
    }
    ```

    - Designated initializers name every field, so reordering the struct cannot shift values
      into the wrong members, and a new field defaults to zero until it is added here.
    - Validation checks ranges, not just "non-zero": a period of 0 or 4 million ms, or a
      channel the hardware does not have, are rejected.
    - An invalid configuration is replaced *as a whole*. Fixing fields one by one can mix
      stored and default values into a combination nobody tested.

??? note "Exercise 4: an exactly sized character array"
    In `char id[4] = "ABCD";` the literal has five characters including its terminator, and
    the array has room for four. C stores the terminator only "if there is room" (C11
    6.7.9p14), so `id` holds four characters and no `'\0'`. This is not an error in C (C++
    rejects it), and GCC does not warn at default settings.

    | Declaration | `sizeof` | Terminated? |
    | --- | ---: | --- |
    | `char id[4] = "ABCD";` | 4 | no |
    | `char id[5] = "ABCD";` | 5 | yes |
    | `char id[] = "ABCD";` | 5 | yes |

    The unterminated form is right for fixed-width fields in file and protocol formats, as
    long as the code treats them as byte arrays with a known length (`memcpy`, `%.4s`) and
    never as strings. See [test-out Q8](./test-out.md#q8-an-exactly-sized-string).

??? note "Exercise 5: memset versus a designated initializer"
    Before:

    ```c
    #include <string.h>

    struct link_settings {
        const char *name;
        double timeout_s;
        unsigned int retries;
        unsigned int baud;
    };

    void link_defaults_memset(struct link_settings *settings)
    {
        memset(settings, 0, sizeof *settings);
        settings->retries = 3u;
        settings->baud = 115200u;
    }
    ```

    After:

    ```c
    struct link_settings {
        const char *name;
        double timeout_s;
        unsigned int retries;
        unsigned int baud;
    };

    void link_defaults(struct link_settings *settings)
    {
        *settings = (struct link_settings){
            .retries = 3u,
            .baud = 115200u,
        };
    }
    ```

    | | `memset` to 0, then assign | compound literal with designators |
    | --- | --- | --- |
    | unnamed members | all bits zero | zero *values*: null pointer, `0.0`, 0 |
    | null pointers and `0.0` | correct only where all-bits-zero means null and `0.0` (every mainstream ABI, but not guaranteed by C) | guaranteed |
    | padding bytes | zeroed | unspecified |
    | defaults visible in one place | no: zero first, values later | yes |
    | a new member | silently 0 | silently 0 |

    Prefer the designated initializer for typed structures. Keep `memset` (or both) where the
    padding bytes matter, for example before copying a structure to the network or to user
    space, because only `memset` zeroes padding
    (see [case study 10](../failure-case-studies/10-struct-padding-leak.md)).

??? note "Exercise 6: tracing data copying and bss clearing"
    The [linker-script session](../compilation-linking-and-abi/linker-scripts-and-memory-layout.md#link-it-and-look)
    does this on a host-built firmware image, so it can be followed without a board. The
    evidence chain:

    1. **The map file** gives `.data` two addresses: it runs in RAM and is stored in flash.

        ```text
        $ grep -A1 -E '^\.data ' firmware.map
        .data           0x0000000020000000        0x4 load address 0x000000000800010c
        ```

    2. **The symbols** give the startup code its bounds:

        ```text
        $ nm -n firmware.elf | grep -E '__data|__bss'
        000000000800010c A __data_load_start__
        0000000020000000 D __data_start__
        0000000020000004 D __data_end__
        0000000020000020 B __bss_start__
        0000000020001020 B __bss_end__
        ```

    3. **The startup code** copies 4 bytes from `0x0800010c` to `0x20000000` (the initial
       value of `boot_count`), then zeroes `0x20000020` up to `0x20001020` (`sample_buffer`).
       The loops are in the page's
       [linker-defined symbols](../compilation-linking-and-abi/linker-scripts-and-memory-layout.md#linker-defined-symbols-in-c)
       section.

    On a board, confirm the same thing with a debugger: break at the first instruction of
    `main`, then check that an initialized global holds its initializer and that a byte in
    the middle of `.bss` reads 0. If a global is wrong at `main`, the startup copy or clear,
    or the `AT>` placement in the linker script, is where to look.

??? note "Exercise 7: an explicit initialization order"
    ```c
    #include <stddef.h>

    int clock_init(void);
    void clock_deinit(void);
    int gpio_init(void);
    void gpio_deinit(void);
    int uart_init(void);
    void uart_deinit(void);
    int sensor_init(void);
    void sensor_deinit(void);

    struct init_step {
        const char *name;
        int (*init)(void);
        void (*deinit)(void);
    };

    /* Each step depends only on the steps above it. */
    static const struct init_step init_steps[] = {
        { "clock",  clock_init,  clock_deinit  },  /* bus and peripheral clocks first */
        { "gpio",   gpio_init,   gpio_deinit   },  /* pins need a clocked GPIO block */
        { "uart",   uart_init,   uart_deinit   },  /* needs clocks and its pins */
        { "sensor", sensor_init, sensor_deinit },  /* talks over the UART */
    };

    /* Returns NULL on success, or the name of the step that failed. */
    const char *system_init(void)
    {
        const size_t count = sizeof init_steps / sizeof init_steps[0];

        for (size_t i = 0; i < count; i++) {
            if (init_steps[i].init() != 0) {
                const char *failed = init_steps[i].name;

                while (i-- > 0) {                  /* undo completed steps, newest first */
                    init_steps[i].deinit();
                }
                return failed;
            }
        }
        return NULL;
    }
    ```

    - The order is data, in one table, with the dependency written next to each step. Adding
      a subsystem means choosing its position deliberately.
    - A failure unwinds the completed steps in reverse order, so the system is never left
      with a sensor powered and its UART half configured.
    - No step relies on the link order of modules or on constructors running in some
      order, which C does not define across translation units.

## Common Mistakes

- Reading an uninitialized automatic or allocated object.
- Confusing zero initialization with all-bits-zero memory.
- Assuming const data is nonvolatile without checking the linker.
- Forgetting that omitted aggregate members are zeroed only during initialization.
- Passing a non-terminated character array to string APIs.
- Returning a pointer to an automatic array or compound literal.
- Using memset to initialize pointers, floats, structures, or registers.
- Relying on unspecified cross-module initialization order.
- Assuming all reset types clear RAM and restore peripherals identically.
- Forgetting to validate retained state after warm reset.

## Debugging Checklist

1. Identify storage duration and exact point of first read.
2. Distinguish a declaration initializer from later assignment.
3. Inspect the linker map for data, bss, read-only data, and RAM copies.
4. Verify startup copies initialized data and clears zero-initialized data.
5. Compare reset classes and retained-memory behavior.
6. Enable uninitialized-use diagnostics and host sanitizers.
7. Check string capacity and termination separately from array initialization.
8. Replace byte-pattern assumptions with typed initialization or encoding.
9. Log subsystem initialization order and rollback.

## Related Topics

- [Language Fundamentals overview](./index.md)
- [Types, Values, And Objects](./types-values-and-objects.md)
- [Structures, Unions, And Enumerations](./structures-unions-and-enums.md)
- [Compilation, Linking, And ABI](../compilation-linking-and-abi/index.md)
- [Startup, Reset, And Vector Tables](../embedded-c-and-hardware/startup-reset-and-vector-tables.md)
- [Semantics And Memory Model](../semantics-and-memory/index.md)
- [C Programming](../index.md)

## References

- [ISO/IEC 9899 standards and drafts](https://open-std.org/jtc1/sc22/wg14/www/standards.html)
- [C11 public draft N1570](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n1570.pdf)
- [GCC variable attributes and storage details](https://gcc.gnu.org/onlinedocs/gcc/Variable-Attributes.html)
- [CERT C coding rules](https://wiki.sei.cmu.edu/confluence/display/c)

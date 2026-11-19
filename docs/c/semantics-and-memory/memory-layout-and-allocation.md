---
status: draft
reviewed: false
domain: c
difficulty: intermediate
last_reviewed: null
---

# Memory Layout And Allocation

C source names objects; the linker and runtime place those objects into memory. Embedded engineers must connect storage duration and object lifetime to sections, stack budgets, heaps, pools, DMA regions, retention memory, and protection boundaries.

There is no single universal C memory layout. Section names, startup behavior, heap implementation, and stack arrangement are implementation and platform contracts.

## Learning Objectives

- Relate common C objects to code, read-only data, data, bss, stack, heap, and thread-local regions.
- Distinguish storage duration from physical placement.
- Evaluate malloc for determinism, fragmentation, and failure handling.
- Design fixed pools and arenas for embedded workloads.
- Use map files and linker symbols as memory evidence.
- Budget worst-case RAM and allocation behavior.

## Typical Image Regions

A hosted or embedded image often contains:

| Region | Typical contents | Key questions |
| --- | --- | --- |
| Text | Executable code | Is it executable, cached, or execute-in-place? |
| Read-only data | Constants, strings, tables | Is it in flash or copied to RAM? |
| Data | Initialized writable statics | Who copies initial values to RAM? |
| Bss | Zero-initialized statics | Who clears it, and when? |
| No-init/retention | Reset-persistent state | How is validity checked? |
| Stack | Automatic objects and call frames | What is the worst depth and interrupt use? |
| Heap | Allocator-managed storage | What are latency, fragmentation, and failure rules? |
| TLS | Per-thread objects | What is the per-thread memory cost? |
| MMIO/DMA | Device windows and buffers | What are access, cache, and ownership rules? |

A map file is the authoritative project evidence for placement, but linker scripts and startup code define the interpretation.

## Static Storage

File-scope and local static objects have static storage duration:

~~~c
#include <stdint.h>

static const uint16_t lookup_table[] = {0u, 10u, 20u};
static uint32_t driver_state;
static uint8_t retained_flags __attribute__((section(".noinit")));
~~~

The first may be placed in read-only storage, the second in bss, and the third in a target-specific retained section. The C declaration alone does not guarantee these placements.

Static allocation is often preferable for bounded firmware because memory use is visible at link time and does not depend on runtime heap state. It still requires ownership and concurrency rules.

## Automatic Storage And Stack

Automatic objects normally use stack storage:

~~~c
#include <stddef.h>
#include <stdint.h>

int parse_header(const uint8_t *data, size_t length)
{
    uint8_t scratch[64];

    if (data == NULL || length < 4u) {
        return -1;
    }

    scratch[0] = data[0];
    return scratch[0];
}
~~~

A single local array can dominate stack use. Calculate worst-case call depth, compiler-generated spills, saved registers, interrupt nesting, RTOS task stacks, and diagnostic paths. Stack overflow may corrupt unrelated state before a fault becomes visible.

Use static analysis, linker reports, stack painting, high-water marks, and target measurement rather than estimating from source indentation.

## Dynamic Allocation

The standard allocator interface is simple:

~~~c
#include <stddef.h>
#include <stdlib.h>

void *make_block(size_t size)
{
    void *block = malloc(size);
    if (block == NULL) {
        return NULL;
    }

    return block;
}

void destroy_block(void *block)
{
    free(block);
}
~~~

Important properties are not guaranteed by this interface:

- bounded allocation and release time;
- absence of fragmentation;
- availability during early boot or interrupt context;
- DMA or cache alignment;
- a particular heap region;
- success for a size that previously succeeded;
- thread or ISR safety on a freestanding implementation.

Never allocate or free in an interrupt handler unless the allocator explicitly supports that context. Do not ignore a null result. Pair each successful allocation with one owner and one release path.

## Overflow Before Allocation

Compute sizes without wrapping:

~~~c
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

bool array_bytes(size_t count, size_t element_size, size_t *result)
{
    if (result == NULL || (element_size != 0u
                           && count > SIZE_MAX / element_size)) {
        return false;
    }

    *result = count * element_size;
    return true;
}
~~~

The multiplication guard must be evaluated before the multiplication. Also check additions for headers, alignment padding, and trailing flexible-array storage.

## Fixed Pools

A fixed pool makes capacity and allocation cost explicit:

~~~c
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum { BLOCK_COUNT = 8u, BLOCK_SIZE = 64u };

struct block {
    bool used;
    uint8_t data[BLOCK_SIZE];
};

static struct block pool[BLOCK_COUNT];

static uint8_t *pool_acquire(void)
{
    for (size_t i = 0u; i < BLOCK_COUNT; ++i) {
        if (!pool[i].used) {
            pool[i].used = true;
            return pool[i].data;
        }
    }
    return NULL;
}
~~~

A production pool also needs release validation, double-release detection, alignment policy, concurrency protection, and an owner or handle so an arbitrary pointer cannot be returned to the wrong pool.

Pools trade generality for bounded memory and predictable failure. Size classes can reduce waste; separate pools can isolate lifetimes and priorities.

## Arenas And Regions

An arena allocates many objects and releases them together:

~~~c
#include <stddef.h>
#include <stdint.h>

struct arena {
    uint8_t *storage;
    size_t capacity;
    size_t used;
};

static void *arena_alloc(struct arena *arena, size_t size)
{
    if (arena == NULL || size > arena->capacity - arena->used) {
        return NULL;
    }

    void *result = arena->storage + arena->used;
    arena->used += size;
    return result;
}

static void arena_reset(struct arena *arena)
{
    if (arena != NULL) {
        arena->used = 0u;
    }
}
~~~

This example omits alignment rounding. A real arena must align each allocation, check rounding overflow, define whether zero-size allocations are allowed, and invalidate all outstanding pointers on reset.

A packet parser or request transaction is a good arena use case when all temporary objects share one clear lifetime.

## DMA And Special Memory

DMA buffers may need:

- a specific physical memory region;
- cache-line alignment;
- non-cacheable mapping or explicit cache maintenance;
- stable lifetime until transfer completion;
- ownership handoff and memory barriers;
- restricted CPU access while hardware owns the buffer.

A general heap allocation is not automatically suitable. Use the platform allocator or linker section intended for DMA and document the ownership protocol.

## Fragmentation And Failure

Fragmentation is a property of allocation history, not just total free bytes. A heap can have enough free memory in aggregate but no sufficiently large contiguous block.

Plan failure behavior:

- reject the request with a typed status;
- use a bounded fallback buffer;
- shed optional work;
- reset or restart a subsystem;
- enter a safe state;
- record telemetry before recovery.

Do not turn allocation failure into a null dereference or an unbounded retry loop.

## Map Files And Measurement

Use the build artifacts:

~~~sh
nm -S --size-sort firmware.elf
size firmware.elf
objdump -h firmware.elf
~~~

Tool names and options vary. Combine static reports with:

- linker map review;
- stack high-water measurement;
- heap watermark and fragmentation telemetry;
- worst-case allocation traces;
- fault-injection tests;
- target debugger watchpoints and memory protection faults.

A memory budget should include startup copies, stacks, queues, pools, DMA descriptors, retained areas, and diagnostic paths.

## Exercises

1. Classify every global and large local object in a small firmware module by section and lifetime.
2. Add checked multiplication and addition helpers to a dynamic packet allocator.
3. Implement a fixed pool with handle validation and double-release detection.
4. Add alignment rounding and overflow checks to the arena allocator.
5. Measure stack high-water marks on all RTOS tasks and interrupt paths.
6. Inject allocation failures at every call site and verify recovery behavior.
7. Inspect a linker map and reconcile it with the project’s RAM budget.

### Model Answers

Try each exercise before opening its answer. Where an exercise asks you to build
something, the answer shows one good solution, not the only one.

??? note "Exercise 1: classifying objects by section and lifetime"
    For a small sensor module (the page's examples plus two typical locals):

    | Object | Section | Lifetime | Notes |
    | --- | --- | --- | --- |
    | `static const uint16_t lookup_table[]` | `.rodata` (flash) | whole program | costs no RAM |
    | `static uint32_t driver_state;` | `.bss` (RAM) | whole program | zeroed by startup code |
    | `static uint32_t retries = 3;` | `.data` (RAM, initial value in flash) | whole program | costs RAM *and* flash |
    | `retained_flags` in `.noinit` | `.noinit` (RAM) | survives warm reset | not initialized: validate before use |
    | `uint8_t frame[256]` local in `parse_header` | stack | one call | counted in the task's stack budget |
    | `static uint8_t scratch[1024]` local in a function | `.bss` | whole program | `static` moves it off the stack, and makes the function non-reentrant |
    | `malloc(n)` in `make_block` | heap | until `destroy_block` | size and failure only known at run time |

    Confirm each row with `nm -S --size-sort` (symbol, size, section letter) and the map
    file. [Storage exercise 5](storage-duration-scope-and-linkage.md#model-answers) shows
    real output for each placement. Locals have no symbol, so measure them with
    `-fstack-usage` instead.

??? note "Exercise 2: checked sizes in a packet allocator"
    ```c
    #include <assert.h>
    #include <stdbool.h>
    #include <stddef.h>
    #include <stdint.h>
    #include <stdlib.h>

    struct packet {
        size_t count;
        uint32_t samples[];                       /* flexible array member */
    };

    static bool size_mul(size_t a, size_t b, size_t *out)
    {
        if (a != 0 && b > SIZE_MAX / a) {
            return false;
        }
        *out = a * b;
        return true;
    }

    static bool size_add(size_t a, size_t b, size_t *out)
    {
        if (a > SIZE_MAX - b) {
            return false;
        }
        *out = a + b;
        return true;
    }

    struct packet *packet_alloc(size_t count)
    {
        size_t payload;
        size_t total;

        if (!size_mul(count, sizeof(uint32_t), &payload)
            || !size_add(sizeof(struct packet), payload, &total)) {
            return NULL;                          /* the size is not representable */
        }
        struct packet *packet = malloc(total);
        if (packet != NULL) {
            packet->count = count;
        }
        return packet;
    }

    int main(void)
    {
        struct packet *packet = packet_alloc(4);

        assert(packet != NULL && packet->count == 4);
        packet->samples[3] = 1;                   /* ASan checks the last element */
        free(packet);
        assert(packet_alloc(SIZE_MAX / 4 + 1) == NULL);   /* the multiplication wraps */
        assert(packet_alloc(SIZE_MAX / 4) == NULL);       /* the addition wraps */
        return 0;
    }
    ```

    Without the checks, `SIZE_MAX / 4 + 1` samples compute a total of `sizeof(struct packet)`,
    and `malloc` succeeds with a buffer that has room for no samples. Every later write to
    `samples[i]` is then out of bounds. The helpers are the same ones as in
    [Undefined Behavior exercise 5](undefined-behavior.md#model-answers).

??? note "Exercise 3: a fixed pool with release validation"
    ```c
    #include <assert.h>
    #include <stdbool.h>
    #include <stddef.h>
    #include <stdint.h>

    enum { BLOCK_COUNT = 8u, BLOCK_SIZE = 64u };

    struct block {
        bool used;
        uint8_t data[BLOCK_SIZE];
    };

    static struct block pool[BLOCK_COUNT];

    enum pool_result { POOL_OK, POOL_NOT_OURS, POOL_DOUBLE_RELEASE };

    uint8_t *pool_acquire(void)
    {
        for (size_t i = 0u; i < BLOCK_COUNT; ++i) {
            if (!pool[i].used) {
                pool[i].used = true;
                return pool[i].data;
            }
        }
        return NULL;
    }

    enum pool_result pool_release(uint8_t *data)
    {
        for (size_t i = 0u; i < BLOCK_COUNT; ++i) {
            if (data == pool[i].data) {           /* == is defined for any two pointers */
                if (!pool[i].used) {
                    return POOL_DOUBLE_RELEASE;
                }
                pool[i].used = false;
                return POOL_OK;
            }
        }
        return POOL_NOT_OURS;                     /* foreign pointer, or one into a block */
    }

    int main(void)
    {
        uint8_t other[BLOCK_SIZE];
        uint8_t *a = pool_acquire();

        assert(a != NULL);
        assert(pool_release(a + 1) == POOL_NOT_OURS);   /* interior pointer */
        assert(pool_release(other) == POOL_NOT_OURS);
        assert(pool_release(a) == POOL_OK);
        assert(pool_release(a) == POOL_DOUBLE_RELEASE);
        for (size_t i = 0; i < BLOCK_COUNT; i++) {
            assert(pool_acquire() != NULL);
        }
        assert(pool_acquire() == NULL);                 /* exhausted */
        return 0;
    }
    ```

    Release is validated by finding the block with `==`, which is defined even for unrelated
    pointers, instead of computing an index from a subtraction that is undefined for a
    foreign pointer. A linear search is fine for small pools. A double release is reported
    rather than ignored, because it means two owners believed they held the block. That
    needs fixing, not tolerating. This check cannot detect a release after the block was
    acquired again by someone else; the generation handle in
    [Memory Safety exercise 3](memory-safety-and-lifetime.md#model-answers) can.

??? note "Exercise 4: an aligned, overflow-checked arena"
    ```c
    #include <assert.h>
    #include <stdalign.h>
    #include <stddef.h>
    #include <stdint.h>

    struct arena {
        uint8_t *storage;
        size_t capacity;
        size_t used;
    };

    /* alignment must be a power of two. Returns NULL if the request does not fit. */
    void *arena_alloc(struct arena *arena, size_t size, size_t alignment)
    {
        if (arena == NULL || size == 0 || alignment == 0 || (alignment & (alignment - 1)) != 0) {
            return NULL;
        }
        uintptr_t current = (uintptr_t)(arena->storage + arena->used);
        size_t padding = (size_t)(-current & (alignment - 1));   /* bytes to the next boundary */

        if (padding > arena->capacity - arena->used
            || size > arena->capacity - arena->used - padding) {
            return NULL;                          /* subtraction form: cannot overflow */
        }
        void *result = arena->storage + arena->used + padding;
        arena->used += padding + size;
        return result;
    }

    int main(void)
    {
        alignas(16) static uint8_t storage[64];
        struct arena arena = { storage, sizeof storage, 0 };

        uint8_t *a = arena_alloc(&arena, 1, 1);
        uint32_t *b = arena_alloc(&arena, sizeof *b, alignof(uint32_t));
        double *c = arena_alloc(&arena, sizeof *c, alignof(double));

        assert(a == storage);
        assert((uintptr_t)b % alignof(uint32_t) == 0 && (uint8_t *)b == storage + 4);
        assert((uintptr_t)c % alignof(double) == 0);
        *b = 1;
        *c = 2.0;
        assert(arena_alloc(&arena, SIZE_MAX, 1) == NULL);     /* huge size: no wrap */
        assert(arena_alloc(&arena, 8, 3) == NULL);            /* not a power of two */
        assert(arena_alloc(&arena, 64, 1) == NULL);           /* does not fit */
        return 0;
    }
    ```

    The padding is computed from the actual address, because the arena's storage itself
    may not be aligned. `-current & (alignment - 1)` is the distance to the next multiple
    of `alignment`, in unsigned arithmetic. Both fit checks subtract from known-safe
    quantities, so neither `used + padding + size` nor a huge `size` can wrap. This version
    rejects zero-size requests. Allowing them is also valid, but it must be a decision.

??? note "Exercise 5: stack high-water marks on tasks and interrupts"
    Tasks: most RTOSes fill each task stack with a known pattern and report how much was
    never overwritten. In FreeRTOS:

    ```c
    #include <stdint.h>

    typedef void *TaskHandle_t;
    typedef uint32_t UBaseType_t;
    UBaseType_t uxTaskGetStackHighWaterMark(TaskHandle_t task);   /* from FreeRTOS task.h */

    void log_stack_margin(const char *name, TaskHandle_t task, UBaseType_t stack_words);
    ```

    `uxTaskGetStackHighWaterMark` returns the minimum free space ever seen, in words. Log
    it for every task after a stress test that exercises every path. The `configCHECK_FOR_STACK_OVERFLOW`
    option checks only at context switches, so it can miss overflows between them.

    Interrupts run on the main stack (MSP on Cortex-M), which the RTOS does not measure.
    Paint it yourself at startup:

    ```c
    #include <stddef.h>
    #include <stdint.h>

    #define STACK_PAINT 0xDEADBEEFu

    extern uint32_t __stack_start__[];   /* lowest address, from the linker script */
    extern uint32_t __stack_top__[];

    /* Call early, before interrupts are enabled; leaves the in-use top 64 words alone. */
    void stack_paint(void)
    {
        for (uint32_t *p = __stack_start__; p < __stack_top__ - 64; p++) {
            *p = STACK_PAINT;
        }
    }

    size_t stack_unused_bytes(void)
    {
        const uint32_t *p = __stack_start__;

        while (p < __stack_top__ && *p == STACK_PAINT) {
            p++;
        }
        return (size_t)(p - __stack_start__) * sizeof *p;
    }
    ```

    Record the results in a table per task and for the main stack: size, worst observed use,
    margin. Measured marks are a lower bound on the worst case, because the test may not have
    hit the deepest path or the worst interrupt nesting. Compare them with the static
    estimate from `-fstack-usage` plus a call-graph tool, and keep a margin (often 20–25%).

??? note "Exercise 6: injecting allocation failures"
    ```c
    #include <assert.h>
    #include <stddef.h>
    #include <stdlib.h>
    #include <string.h>

    /* Test hook: fail the Nth allocation (1-based); 0 means never fail. */
    static size_t fail_at;
    static size_t allocations;

    static void *test_malloc(size_t size)
    {
        allocations++;
        return allocations == fail_at ? NULL : malloc(size);
    }

    struct session {
        char *name;
        unsigned char *rx;
        unsigned char *tx;
    };

    /* Code under test: three allocations, each failure must clean up the others. */
    struct session *session_create(const char *name)
    {
        struct session *s = test_malloc(sizeof *s);
        if (s == NULL) {
            return NULL;
        }
        s->name = test_malloc(strlen(name) + 1);
        s->rx = test_malloc(256);
        s->tx = test_malloc(256);
        if (s->name == NULL || s->rx == NULL || s->tx == NULL) {
            free(s->name);
            free(s->rx);
            free(s->tx);
            free(s);
            return NULL;
        }
        strcpy(s->name, name);
        return s;
    }

    void session_destroy(struct session *s)
    {
        if (s != NULL) {
            free(s->name);
            free(s->rx);
            free(s->tx);
            free(s);
        }
    }

    int main(void)
    {
        /* Fail each allocation in turn, then run once with no failure. */
        for (fail_at = 1; ; fail_at++) {
            allocations = 0;
            struct session *s = session_create("uart0");
            if (fail_at <= allocations) {
                assert(s == NULL);             /* the failure was reported */
            } else {
                assert(s != NULL);             /* no injected failure left */
                session_destroy(s);
                break;
            }
        }
        return 0;                              /* LeakSanitizer checks that nothing leaked */
    }
    ```

    The loop needs no knowledge of how many allocations the function makes. It keeps moving
    the failure point until a run completes without one. Run it under ASan: LeakSanitizer
    then reports any failure path that forgot a `free`, and ASan reports any path that
    freed twice. The same hook works on the target if allocations go through one project
    wrapper.

??? note "Exercise 7: reconciling a map file with the RAM budget"
    The extended linker-script image from
    [Storage exercise 5](storage-duration-scope-and-linkage.md#model-answers):

    ```text
    $ gcc ... -Wl,--print-memory-usage ...
    Memory region         Used Size  Region Size  %age Used
               FLASH:         296 B       256 KB      0.11%
                 RAM:        6184 B        64 KB      9.44%
    ```

    | RAM contents | Bytes | Source |
    | --- | --- | --- |
    | `.data` | 4 | map: `boot_count` |
    | gap before `.bss` | 28 | GCC on x86-64 aligns the large `sample_buffer` to 32 bytes |
    | `.bss` | 4096 | map: `sample_buffer` |
    | `.noinit` | 4 | `reset_reason` |
    | `.stack` | 2052 | 4 bytes of alignment padding + the 2048-byte reservation |
    | **total** | **6184** | matches the linker's figure |

    FLASH is 220 bytes of `.text` (with `.rodata`), 68 bytes of `.note.gnu.*` sections the
    script never mentions, then the 4-byte `.data` initializer: 296. The reconciliation finds
    two things a budget spreadsheet misses: alignment gaps, and orphan sections. A real
    budget also lists the heap (if any), each RTOS task stack (usually inside `.bss`, as
    static arrays), and the measured margins from exercise 5. Fail the build when a region
    exceeds its budget, as the page's `ASSERT` in the linker script does for RAM.

## Common Mistakes

- Assuming source storage duration determines exact physical section.
- Allocating large buffers on the stack without a worst-case budget.
- Calling malloc from interrupt or hard real-time context.
- Ignoring allocator alignment and cache requirements.
- Multiplying sizes before checking for overflow.
- Resetting an arena while borrowed pointers remain in use.
- Treating a no-init section as valid without integrity checks.
- Assuming total free heap equals the largest allocatable block.
- Failing to test allocation failure.
- Measuring only average memory use.

## Debugging Checklist

1. Inspect map file, section sizes, and linker symbols.
2. Identify every stack owner and maximum call path.
3. Record allocation size, caller, lifetime, and release owner.
4. Add heap and pool watermark telemetry.
5. Check alignment and cache state for DMA buffers.
6. Run overflow and allocation-failure tests.
7. Use target memory protection or stack guards where available.
8. Compare measured high-water values with product budgets and recovery paths.

## Related Topics

- [Semantics And Memory overview](./index.md)
- [Storage Duration, Scope, And Linkage](./storage-duration-scope-and-linkage.md)
- [Object Representation, Alignment, And Padding](./object-representation-alignment-and-padding.md)
- [Memory Safety And Lifetime](./memory-safety-and-lifetime.md)
- [Compilation, Linking, And ABI](../compilation-linking-and-abi/index.md)
- [Embedded C And Hardware](../embedded-c-and-hardware/index.md)
- [C Programming](../index.md)

## References

- [ISO/IEC 9899 standards and drafts](https://open-std.org/jtc1/sc22/wg14/www/standards.html)
- [C11 public draft N1570](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n1570.pdf)
- [GCC memory allocation and built-in documentation](https://gcc.gnu.org/onlinedocs/gcc/Memory-Allocation.html)
- [GNU linker scripts](https://sourceware.org/binutils/docs/ld/Scripts.html)

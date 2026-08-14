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
    if (result == NULL || element_size != 0u
        && count > SIZE_MAX / element_size) {
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

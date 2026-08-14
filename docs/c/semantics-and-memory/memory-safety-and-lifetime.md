---
status: draft
reviewed: false
domain: c
difficulty: intermediate
last_reviewed: null
---

# Memory Safety And Lifetime

Memory safety is the discipline of ensuring that every access is within bounds, correctly aligned, made through a permitted type, and performed while the target object is alive. Lifetime safety also includes ownership, release, reuse, and concurrency.

Embedded systems may avoid a general heap, but they do not avoid lifetime problems. A queue slot, DMA buffer, register window, static workspace, or RTOS object can all be used too early, too late, or by the wrong owner.

## Learning Objectives

- Define ownership, borrowing, transfer, and release responsibilities.
- Identify use-after-scope, use-after-free, double-release, leaks, and stale aliases.
- Design bounds-aware interfaces.
- Build lifetime graphs for buffers and asynchronous operations.
- Handle memory and resource failures explicitly.
- Apply sanitizers, static analysis, and fault injection to lifetime defects.

## Ownership Models

Choose an ownership model for each pointer:

| Model | Caller responsibility | Callee responsibility |
| --- | --- | --- |
| Borrowed input | Keep object alive for call and meet mutability rules | Do not retain unless documented |
| Borrowed output | Provide writable storage and capacity | Write only within the contract |
| Owned return | Release through the documented function | Return a valid object or failure |
| Transfer | Stop using and release after transfer | Release at the new lifecycle boundary |
| Shared | Coordinate concurrent use and final release | Follow reference or owner protocol |
| Static view | Do not free or assume private ownership | Document reset and reentrancy behavior |

Write ownership into function names, parameter names, documentation, and tests. A pointer type alone cannot express all of these states in C.

## Use-After-Scope

~~~c
#include <stddef.h>

const int *make_view(void)
{
    int local[2] = {1, 2};
    return local;
}
~~~

The returned pointer outlives local. Fix it by returning a value, writing into caller-provided storage, using an owner-managed allocation, or keeping the object in a documented static lifetime.

A pointer stored in a structure can be just as stale as a pointer returned directly:

~~~c
struct span {
    const unsigned char *data;
    size_t length;
};
~~~

Review the lifetime of data, not only the lifetime of span.

## Allocation And Release Pairing

Every successful allocation needs one release path:

~~~c
#include <stdlib.h>

int make_value(int **result)
{
    if (result == NULL) {
        return -1;
    }

    int *value = malloc(sizeof *value);
    if (value == NULL) {
        return -2;
    }

    *value = 0;
    *result = value;
    return 0;
}

void destroy_value(int **value)
{
    if (value != NULL) {
        free(*value);
        *value = NULL;
    }
}
~~~

The temporary pointer prevents publishing partially initialized storage. Setting the caller’s pointer to null after release reduces one class of accidental reuse, but it does not repair aliases held elsewhere.

Pairing must match the allocator and API. Do not release stack, static, MMIO, pool, or arena storage with free.

## Double Release And Stale Aliases

A pointer copied before release remains stale afterward:

~~~c
#include <stdlib.h>

void bad_release(void)
{
    int *owner = malloc(sizeof *owner);
    int *alias = owner;

    free(owner);
    owner = NULL;
    (void)alias;
}
~~~

Clearing owner does not clear alias. Ownership transfer should invalidate the old owner by convention and remove or update aliases at the same time.

Pool and arena APIs need equivalent rules. A block returned to a pool may be reused immediately, so an old pointer can silently access a new object rather than faulting.

## Bounds-Safe Interfaces

Carry the bounds with the pointer:

~~~c
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

bool copy_prefix(uint8_t *destination, size_t destination_capacity,
                 const uint8_t *source, size_t source_length,
                 size_t count)
{
    if (destination == NULL || source == NULL || count > source_length
        || count > destination_capacity) {
        return false;
    }

    for (size_t i = 0u; i < count; ++i) {
        destination[i] = source[i];
    }
    return true;
}
~~~

If overlap is possible, use a defined overlap policy or memmove. If the destination needs a terminator, include that requirement in the capacity check instead of adding it after the copy.

Avoid APIs that accept a pointer with an undocumented assumed size. Static analysis and compiler annotations can help, but the runtime contract still needs a check for untrusted lengths.

## Asynchronous Lifetimes

An asynchronous operation extends the lifetime requirement beyond a normal call:

~~~c
enum dma_result {
    DMA_STARTED,
    DMA_BUSY,
    DMA_ERROR
};

enum dma_result start_transfer(const uint8_t *buffer, size_t length);
~~~

If the DMA engine reads buffer after start_transfer returns, the caller must keep buffer alive, unchanged, aligned, and owned by the device until completion. The completion event returns ownership to the CPU.

Document the state machine:

1. CPU owns and prepares the buffer.
2. CPU submits the descriptor and performs required cache operations.
3. Device owns the buffer; CPU does not modify it.
4. Device signals completion.
5. CPU performs required cache operations and validates length/status.
6. CPU owns the buffer again.

The same pattern applies to interrupt queues, RTOS message buffers, deferred callbacks, and zero-copy network paths.

## Resource Lifetime Graphs

Memory is only one resource. A driver may own:

- an allocated context;
- a clock or power reference;
- an interrupt registration;
- a DMA descriptor;
- a mapped register window;
- a queue slot;
- a file descriptor or Linux device handle.

Draw acquisition and release edges. A failure at step four must release resources acquired in steps one through three. A shutdown path must quiesce asynchronous users before releasing storage they may still reference.

Forward cleanup is often clearer than duplicated returns:

~~~c
int start_component(void)
{
    int result = -1;
    bool clock_enabled = false;
    bool irq_registered = false;

    if (enable_clock() != 0) {
        goto out;
    }
    clock_enabled = true;

    if (register_irq() != 0) {
        goto out;
    }
    irq_registered = true;

    result = 0;

out:
    if (irq_registered) {
        unregister_irq();
    }
    if (clock_enabled) {
        disable_clock();
    }
    return result;
}
~~~

The cleanup order is the reverse of acquisition. First stop producers and asynchronous users, then release the storage they could access.

## Failure Handling

Memory failure is part of normal behavior in a robust system:

- return a typed error;
- keep the old object valid until a replacement is ready;
- use a bounded fallback;
- shed optional work;
- retry only with a bounded policy;
- record diagnostic context;
- enter a safe state if the resource is essential.

Do not partially update an owning object before a required allocation succeeds.

## Static And Dynamic Analysis

Use layered evidence:

- compiler warnings for suspicious conversions and lifetime patterns;
- AddressSanitizer for many heap, stack, and global bounds errors;
- LeakSanitizer where supported;
- UndefinedBehaviorSanitizer for invalid operations;
- static analysis for interprocedural ownership and paths;
- fuzzing for parser boundaries;
- MPU or MMU faults and target trace for deployed behavior.

Tools have blind spots. DMA, MMIO, custom pools, and interrupt races usually need explicit tests and platform instrumentation.

## Exercises

1. Draw an ownership table for every pointer in a packet parser.
2. Implement a replacement operation that preserves the old buffer if allocation fails.
3. Add a pool handle and generation counter to detect stale block references.
4. Model a DMA buffer’s CPU/device ownership state and test illegal transitions.
5. Inject failures after each resource acquisition and verify reverse cleanup.
6. Run sanitizers on a host version and compare findings with target fault logs.

## Common Mistakes

- Returning or retaining pointers to automatic objects.
- Freeing through the wrong allocator or twice.
- Clearing one owner pointer while aliases remain.
- Publishing partially initialized storage.
- Copying more than destination capacity.
- Releasing a DMA buffer before completion.
- Resetting an arena while users retain pointers.
- Releasing resources before asynchronous producers stop.
- Treating allocator failure as impossible.
- Relying on sanitizers to cover hardware ownership and interrupt races.

## Debugging Checklist

1. Identify the object owner, borrower, and release event.
2. Draw the object lifetime from creation through invalidation.
3. Check every pointer and length at the boundary.
4. Track asynchronous ownership and cache transitions.
5. Use allocation IDs, generation counters, and poison patterns in debug builds.
6. Run sanitizers and leak checks on host-representable paths.
7. Fault-inject every allocation and initialization step.
8. Verify shutdown order with callbacks, interrupts, DMA, tasks, and queues active.

## Related Topics

- [Semantics And Memory overview](./index.md)
- [Storage Duration, Scope, And Linkage](./storage-duration-scope-and-linkage.md)
- [Pointer Fundamentals](./pointer-fundamentals.md)
- [Pointer Arithmetic And Bounds](./pointer-arithmetic-and-bounds.md)
- [Memory Layout And Allocation](./memory-layout-and-allocation.md)
- [DMA, Cache, And Memory Barriers](../embedded-c-and-hardware/dma-cache-and-memory-barriers.md)
- [C Programming](../index.md)

## References

- [ISO/IEC 9899 standards and drafts](https://open-std.org/jtc1/sc22/wg14/www/standards.html)
- [C11 public draft N1570](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n1570.pdf)
- [Clang AddressSanitizer documentation](https://clang.llvm.org/docs/AddressSanitizer.html)
- [Clang UndefinedBehaviorSanitizer](https://clang.llvm.org/docs/UndefinedBehaviorSanitizer.html)
- [CERT C memory-management rules](https://wiki.sei.cmu.edu/confluence/display/c)

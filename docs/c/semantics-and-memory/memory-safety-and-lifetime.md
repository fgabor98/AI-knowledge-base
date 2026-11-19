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

<!-- c-check: expect-warning -->

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
#include <stddef.h>

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
#include <stddef.h>
#include <stdint.h>

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
#include <stdbool.h>

int enable_clock(void);
void disable_clock(void);
int register_irq(void);
void unregister_irq(void);

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

### Model Answers

Try each exercise before opening its answer. Where an exercise asks you to build
something, the answer shows one good solution, not the only one.

??? note "Exercise 1: an ownership table for a packet parser"
    For a parser like `decode` in
    [Object Representation exercise 6](object-representation-alignment-and-padding.md#model-answers),
    called by a receive task:

    | Pointer | Points to | Owner | Borrowed by | Valid until |
    | --- | --- | --- | --- | --- |
    | `rx_buffer` (task) | static DMA buffer | UART driver | the task, after the RX-complete event | the task re-arms the DMA |
    | `in` (`decode` parameter) | `rx_buffer` | the task | `decode`, for the call | `decode` returns |
    | `out` (`decode` parameter) | the task's `struct message` | the task | `decode`, for writing | `decode` returns |
    | `message.payload` | inside `rx_buffer` | UART driver | the task | the task re-arms the DMA |
    | `reply` | arena block | the arena | the encoder | `arena_reset` |

    The table exposes the real constraint: `message.payload` is a view into the DMA buffer,
    so the task must finish with the payload, or copy it, *before* re-arming the receive.
    Each row answers who frees or reuses the memory, and when. A pointer that cannot be
    given a row is the bug.

??? note "Exercise 2: replacing a buffer without losing the old one"
    ```c
    #include <assert.h>
    #include <stdbool.h>
    #include <stddef.h>
    #include <stdint.h>
    #include <stdlib.h>
    #include <string.h>

    static bool fail_next_allocation;                 /* test hook */

    static void *checked_malloc(size_t size)
    {
        if (fail_next_allocation) {
            fail_next_allocation = false;
            return NULL;
        }
        return malloc(size);
    }

    struct blob {
        uint8_t *data;
        size_t length;
    };

    /* Replaces the contents with a copy of data. On failure the blob is unchanged. */
    bool blob_replace(struct blob *blob, const uint8_t *data, size_t length)
    {
        uint8_t *copy = checked_malloc(length != 0 ? length : 1);

        if (copy == NULL) {
            return false;                             /* old data still owned and valid */
        }
        if (length != 0) {
            memcpy(copy, data, length);
        }
        free(blob->data);                             /* release only after success */
        blob->data = copy;
        blob->length = length;
        return true;
    }

    int main(void)
    {
        struct blob blob = { NULL, 0 };

        assert(blob_replace(&blob, (const uint8_t *)"abc", 3));
        fail_next_allocation = true;
        assert(!blob_replace(&blob, (const uint8_t *)"wxyz", 4));
        assert(blob.length == 3 && memcmp(blob.data, "abc", 3) == 0);   /* unchanged */
        assert(blob_replace(&blob, (const uint8_t *)"wxyz", 4) && blob.length == 4);
        free(blob.data);
        return 0;
    }
    ```

    Allocate the new buffer, copy, and only then free the old one. The same rule applies to
    `realloc`: `p = realloc(p, n)` leaks the old block and loses the pointer when it returns
    `NULL`. Assign to a temporary first. The copy also makes it safe when `data` points into
    the blob's own old buffer.

??? note "Exercise 3: pool handles with generation counters"
    ```c
    #include <assert.h>
    #include <stdbool.h>
    #include <stddef.h>
    #include <stdint.h>

    #define SLOT_COUNT 4u

    struct handle {
        uint16_t index;
        uint16_t generation;            /* 0 is never a valid generation */
    };

    struct slot {
        uint16_t generation;            /* bumped on every release */
        bool used;
        uint8_t data[32];
    };

    static struct slot slots[SLOT_COUNT];

    bool slot_acquire(struct handle *out)
    {
        for (uint16_t i = 0; i < SLOT_COUNT; i++) {
            if (!slots[i].used) {
                slots[i].used = true;
                if (slots[i].generation == 0) {
                    slots[i].generation = 1;
                }
                *out = (struct handle){ i, slots[i].generation };
                return true;
            }
        }
        return false;
    }

    /* Returns NULL for a stale, released, or out-of-range handle. */
    uint8_t *slot_get(struct handle handle)
    {
        if (handle.index >= SLOT_COUNT || !slots[handle.index].used
            || slots[handle.index].generation != handle.generation) {
            return NULL;
        }
        return slots[handle.index].data;
    }

    bool slot_release(struct handle handle)
    {
        if (slot_get(handle) == NULL) {
            return false;                               /* double or stale release */
        }
        slots[handle.index].used = false;
        slots[handle.index].generation++;
        if (slots[handle.index].generation == 0) {
            slots[handle.index].generation = 1;         /* skip 0 on wrap */
        }
        return true;
    }

    int main(void)
    {
        struct handle old_handle;
        struct handle new_handle;

        assert(slot_acquire(&old_handle) && slot_get(old_handle) != NULL);
        assert(slot_release(old_handle));
        assert(slot_acquire(&new_handle));               /* reuses the same slot */
        assert(new_handle.index == old_handle.index);
        assert(slot_get(old_handle) == NULL);            /* stale reference detected */
        assert(!slot_release(old_handle));               /* stale release rejected */
        assert(slot_get(new_handle) != NULL);            /* the new owner is unaffected */
        return 0;
    }
    ```

    A raw pointer to a reused block cannot be told apart from a pointer held by the new
    owner. A handle can, because the generation changes on every release. The check fails
    to catch a stale handle only if the generation wraps all the way back (65,535 reuses
    of one slot) while the stale handle is still held, which is usually acceptable. Use a
    wider counter if it is not.

??? note "Exercise 4: a DMA ownership state model"
    ```c
    #include <assert.h>
    #include <stddef.h>
    #include <stdint.h>

    enum dma_owner { DMA_OWNER_CPU, DMA_OWNER_DEVICE };
    enum dma_status { DMA_OK, DMA_WRONG_OWNER };

    struct dma_buffer {
        enum dma_owner owner;
        uint8_t bytes[64];
    };

    enum dma_status dma_give_to_device(struct dma_buffer *buffer)
    {
        if (buffer->owner != DMA_OWNER_CPU) {
            return DMA_WRONG_OWNER;           /* already submitted */
        }
        /* cache clean / invalidate would go here */
        buffer->owner = DMA_OWNER_DEVICE;
        return DMA_OK;
    }

    /* Called from the transfer-complete interrupt. */
    enum dma_status dma_complete(struct dma_buffer *buffer)
    {
        if (buffer->owner != DMA_OWNER_DEVICE) {
            return DMA_WRONG_OWNER;           /* spurious or duplicate completion */
        }
        /* cache invalidate would go here */
        buffer->owner = DMA_OWNER_CPU;
        return DMA_OK;
    }

    /* The only way for the CPU to reach the bytes. */
    uint8_t *dma_cpu_bytes(struct dma_buffer *buffer)
    {
        return buffer->owner == DMA_OWNER_CPU ? buffer->bytes : NULL;
    }

    int main(void)
    {
        struct dma_buffer buffer = { .owner = DMA_OWNER_CPU };

        assert(dma_complete(&buffer) == DMA_WRONG_OWNER);         /* complete before start */
        assert(dma_give_to_device(&buffer) == DMA_OK);
        assert(dma_give_to_device(&buffer) == DMA_WRONG_OWNER);   /* double submit */
        assert(dma_cpu_bytes(&buffer) == NULL);                   /* CPU access while device owns */
        assert(dma_complete(&buffer) == DMA_OK);
        assert(dma_complete(&buffer) == DMA_WRONG_OWNER);         /* duplicate completion */
        assert(dma_cpu_bytes(&buffer) != NULL);
        return 0;
    }
    ```

    Every transition checks the current owner, and the tests try each illegal one: complete
    before start, double submit, CPU access during the transfer, and duplicate completion.
    On a real target, `owner` is written by both the task and the interrupt, so make it
    atomic or update it with interrupts masked. The cache steps belong *inside* the
    transitions, so they cannot be forgotten
    ([Arrays exercise 6](../language-fundamentals/arrays-strings-and-buffers.md#model-answers)
    has the full contract).

??? note "Exercise 5: failure injection with reverse cleanup"
    ```c
    #include <assert.h>
    #include <string.h>

    static char log_text[64];
    static int fail_step;                    /* which acquisition fails; 0 = none */

    static int acquire(int step, const char *name)
    {
        if (step == fail_step) {
            return -1;
        }
        strcat(log_text, name);              /* "+clock" etc. */
        return 0;
    }

    static void release(const char *name)
    {
        strcat(log_text, name);              /* "-clock" etc. */
    }

    int start_component(void)
    {
        if (acquire(1, "+clock ") != 0) {
            goto fail;
        }
        if (acquire(2, "+irq ") != 0) {
            goto fail_clock;
        }
        if (acquire(3, "+dma ") != 0) {
            goto fail_irq;
        }
        return 0;

    fail_irq:
        release("-irq ");
    fail_clock:
        release("-clock ");
    fail:
        return -1;
    }

    int main(void)
    {
        static const char *const expected[] = {
            "",                              /* step 1 fails: nothing to undo */
            "+clock -clock ",
            "+clock +irq -irq -clock ",      /* reverse order */
        };

        for (fail_step = 1; fail_step <= 3; fail_step++) {
            log_text[0] = '\0';
            assert(start_component() == -1);
            assert(strcmp(log_text, expected[fail_step - 1]) == 0);
        }
        fail_step = 0;
        log_text[0] = '\0';
        assert(start_component() == 0 && strcmp(log_text, "+clock +irq +dma ") == 0);
        return 0;
    }
    ```

    The fake resources record a log, so each test checks the *order* of the cleanup, not
    only that something was released. Failing each step in turn is the only way to execute
    every cleanup path; normal tests run none of them. A missing or misordered label shows
    up as a log mismatch.

??? note "Exercise 6: host sanitizer findings versus target faults"
    The same bug looks different on the host and on a Cortex-M target without an MMU:

    | Host finding (ASan / UBSan) | Typical target symptom | Target evidence to look for |
    | --- | --- | --- |
    | heap- or stack-buffer-overflow | usually **nothing**: the write lands in a neighbouring object in RAM | corrupted variables, a stack canary failure, a watchpoint on the neighbour |
    | access to unmapped memory, NULL dereference | BusFault or HardFault; address 0 often *reads* successfully (the vector table) | `CFSR` `PRECISERR` with `BFAR` set; writes to flash fault |
    | stack-use-after-return | stale data, sometimes a fault much later | nothing direct; reproduce on the host |
    | use-after-free, double free | allocator corruption, failure inside `malloc` | heap integrity check, pool double-release detection |
    | misaligned load | UsageFault on M0/M0+ or with `UNALIGN_TRP`; silent elsewhere | `CFSR` `UNALIGNED` bit |
    | signed overflow, invalid shift | wrong values; no fault | only the host run finds it |
    | stack overflow | HardFault, or silent corruption of `.bss` below the stack | stack paint, MPU guard region, `MSPLIM` on Armv8-M |

    The comparison shows why both are needed. The host run finds memory bugs the target
    silently tolerates, and the target finds faults that depend on real addresses, MPU
    settings, and hardware. When a target fault log and a host finding point at the same
    function, fix the host finding first, then check that the target fault disappears.

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
- [Case study 09: Pointer Into A Reallocated Array](../failure-case-studies/09-pointer-into-reallocated-array.md)

## References

- [ISO/IEC 9899 standards and drafts](https://open-std.org/jtc1/sc22/wg14/www/standards.html)
- [C11 public draft N1570](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n1570.pdf)
- [Clang AddressSanitizer documentation](https://clang.llvm.org/docs/AddressSanitizer.html)
- [Clang UndefinedBehaviorSanitizer](https://clang.llvm.org/docs/UndefinedBehaviorSanitizer.html)
- [CERT C memory-management rules](https://wiki.sei.cmu.edu/confluence/display/c)

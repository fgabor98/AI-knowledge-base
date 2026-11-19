---
status: draft
reviewed: false
domain: c
difficulty: intermediate
last_reviewed: null
---

# Const, Volatile, And Restrict

Type qualifiers communicate different contracts. Const constrains modification through an access path. Volatile makes accesses observable to the abstract machine. Restrict promises a non-overlapping access relationship for a computation. Atomic types provide synchronization and indivisible operations.

These qualifiers are not interchangeable. In particular, volatile is not a lock, a memory barrier, or a guarantee that a multi-byte access is atomic.

## Learning Objectives

- Apply const to data and pointers intentionally.
- Explain what volatile does and does not guarantee.
- Use restrict only when the caller and implementation satisfy its aliasing contract.
- Distinguish volatile from atomic synchronization.
- Combine qualifiers for MMIO and read-only hardware state.
- Recognize qualifier casts that remove a safety contract.

## const Correctness

These declarations qualify different objects:

~~~c
#include <stddef.h>
#include <stdint.h>

const uint32_t *data;
uint32_t *const address = NULL;
const uint32_t *const fixed_data = NULL;
~~~

- data is a modifiable pointer to const uint32_t;
- address is a const pointer to modifiable uint32_t;
- fixed_data is a const pointer to const uint32_t.

Const prevents modification through that particular lvalue. It does not make the underlying storage immutable if another permitted alias can modify it, and it does not make access thread-safe.

Propagate const through read-only interfaces:

~~~c
#include <stddef.h>
#include <stdint.h>

uint32_t checksum(const uint8_t *data, size_t length)
{
    uint32_t result = 0u;
    for (size_t i = 0u; i < length; ++i) {
        result = (result << 5) ^ data[i];
    }
    return result;
}
~~~

Do not cast away const to call a legacy API unless you have proved that the API does not modify the object and the object itself is writable.

## volatile Semantics

Volatile tells the implementation that an access through the volatile-qualified lvalue is observable and must be performed according to the implementation’s rules. It is appropriate for memory-mapped registers, status changed by an interrupt handler, special signal-handler objects under platform rules, and compiler-defined communication areas.

~~~c
#include <stdint.h>

struct timer_registers {
    volatile uint32_t status;
    volatile uint32_t control;
};

static struct timer_registers *const timer =
    (struct timer_registers *)0x40000000u;

uint32_t timer_status(void)
{
    return timer->status;
}
~~~

The address, structure layout, access width, and required barriers are target-specific. Volatile does not make a read-modify-write sequence atomic, does not order unrelated non-volatile memory for a multicore system, and does not make a register safe to access from every context.

A volatile access can still be optimized around other operations as permitted by the implementation. Use the platform’s barrier, atomic, or device-access primitive when hardware requires ordering.

## Interrupt Flags

A volatile flag may be enough for a simple single-core polling handoff when the platform guarantees the access width and interrupt behavior:

~~~c
#include <stdbool.h>

static volatile bool event_pending;

void interrupt_handler(void)
{
    event_pending = true;
}

bool take_event(void)
{
    if (!event_pending) {
        return false;
    }

    event_pending = false;
    return true;
}
~~~

This is not a general producer-consumer queue. An interrupt can arrive between the test and clear, and multiple events can collapse into one. For multicore or task synchronization, use C atomic facilities or an RTOS event and define the memory-ordering protocol.

## restrict

Restrict applies to pointer expressions and promises that, for a relevant execution, accesses to an object through that restricted pointer are not also made through an unrelated pointer expression:

~~~c
#include <stddef.h>

void add_arrays(size_t length,
                int *restrict destination,
                const int *restrict left,
                const int *restrict right)
{
    for (size_t i = 0u; i < length; ++i) {
        destination[i] = left[i] + right[i];
    }
}
~~~

The intended caller must not pass overlapping regions in a way that violates the contract. If destination aliases left, behavior may be undefined and the compiler may vectorize or reorder accesses.

Restrict is a promise, not a runtime check. Use it when non-aliasing is part of the API and tests enforce it. Omit it when overlap is valid or ownership is uncertain.

## Atomic Types And Synchronization

C11 atomic types and operations provide a language-level synchronization model:

~~~c
#include <stdatomic.h>

static _Atomic unsigned int sequence;

void publish(void)
{
    atomic_fetch_add_explicit(&sequence, 1u, memory_order_release);
}

unsigned int observe(void)
{
    return atomic_load_explicit(&sequence, memory_order_acquire);
}
~~~

Atomicity, visibility, and ordering are distinct properties. An atomic counter does not make the data it counts automatically visible unless the chosen ordering establishes that relationship.

Volatile may be combined with an atomic type for a device-specific reason, but do not add volatile as a substitute for understanding atomic operations.

## Qualifier Casts

A cast can remove qualifiers:

~~~c
void legacy_write(char *text);

void call_legacy(const char *text)
{
    legacy_write((char *)text);
}
~~~

This is safe only if legacy_write never writes and the pointed-to object is writable. If text points to a string literal or read-only flash, a write is invalid. Prefer changing the legacy prototype or adding a correctly typed adapter.

A cast that adds volatile or const changes the access path, not the underlying object’s lifetime or hardware behavior.

## Embedded Patterns

Common patterns include:

~~~c
#include <stdint.h>

#define REG32(address) (*(volatile uint32_t *)(address))

static inline void set_bits(uintptr_t address, uint32_t mask)
{
    REG32(address) |= mask;
}
~~~

This pattern is target-specific and may be wrong for write-one-to-clear registers, side-effectful reads, or non-atomic register updates. A vendor HAL or reviewed accessor should encode those rules.

For a read-only status register, use const volatile when the CPU must not write but hardware may change the value:

~~~c
#include <stdint.h>

static const volatile uint32_t *status_register =
    (const volatile uint32_t *)0x40000000u;
~~~

## Exercises

1. Rewrite a mutable input API with const-correct parameters and fix all callers.
2. Construct a test where overlapping restrict arguments violate the contract; compare optimized and unoptimized builds only as evidence, not as a definition.
3. Replace a volatile task flag with a C11 atomic or RTOS event and document the memory ordering.
4. Read a peripheral manual and identify which registers need volatile, barriers, write-one-to-clear handling, or atomic set/clear aliases.
5. Review every cast that removes const or adds volatile in a driver.
6. Measure whether a volatile polling loop meets its timeout and power budget.

### Model Answers

Try each exercise before opening its answer. Where an exercise asks you to build
something, the answer shows one good solution, not the only one.

??? note "Exercise 1: const-correct parameters and callers"
    Before:

    ```c
    #include <stddef.h>

    size_t count_char(char *text, char wanted);

    size_t count_spaces(void)
    {
        return count_char("a b c", ' ');
    }
    ```

    This compiles silently, because a string literal has type `char[]` in C, even though
    writing to it is undefined. With `-Wwrite-strings`, GCC 13 reports the call:
    `passing argument 1 of 'count_char' discards 'const' qualifier from pointer target type`.

    After:

    ```c
    #include <stddef.h>

    size_t count_char(const char *text, char wanted)
    {
        size_t count = 0;

        for (; *text != '\0'; text++) {
            count += *text == wanted;
        }
        return count;
    }

    size_t count_spaces(void)
    {
        return count_char("a b c", ' ');
    }
    ```

    Callers passing `char *` need no change, since adding `const` is implicit. The callers
    to fix are those that had a cast such as `(char *)name`, or that passed `const` data
    and were warned. Removing those casts is the point of the exercise. If a callee
    genuinely writes, the parameter stays non-`const`, and the caller that passed read-only
    data had a real bug. See also
    [Declarations exercise 4](../language-fundamentals/declarations-and-declarators.md#model-answers).

??? note "Exercise 2: violating restrict on purpose"
    <!-- c-check: expect-warning -->

    ```c
    #include <stdio.h>

    void add_twice(int *restrict total, const int *restrict step)
    {
        *total += *step;
        *total += *step;
    }

    int main(void)
    {
        int x = 1;

        add_twice(&x, &x);          /* violates restrict: both point at x */
        printf("x = %d\n", x);
        return 0;
    }
    ```

    ```text
    rs.c:13:15: warning: passing argument 1 to 'restrict'-qualified parameter aliases with argument 2 [-Wrestrict]
    -O0: x = 4
    -O2: x = 3
    ```

    At `-O0` each `*step` is reloaded after `*total` changes: 1 + 1 = 2, then 2 + 2 = 4. At
    `-O2`, `restrict` lets GCC load `*step` once: 1 + 1 + 1 = 3. Neither result is "the
    right one". The call is undefined, and the two builds are only evidence that the
    compiler used the promise. GCC caught this case because the aliasing is visible at the
    call. Through pointers computed at run time it cannot, so restrict APIs need a
    documented precondition, and ideally a debug check (see
    [Conversions exercise 4](conversions-promotions-and-aliasing.md#model-answers)).

??? note "Exercise 3: replacing a volatile flag with an atomic"
    ```c
    #include <pthread.h>
    #include <stdatomic.h>
    #include <stdbool.h>
    #include <stdio.h>

    static int sample;                              /* the data being published */
    static atomic_bool ready;

    static void *producer(void *arg)
    {
        (void)arg;
        sample = 42;                                /* 1: write the data */
        atomic_store_explicit(&ready, true, memory_order_release);   /* 2: publish */
        return NULL;
    }

    int main(void)
    {
        pthread_t thread;

        pthread_create(&thread, NULL, producer, NULL);
        while (!atomic_load_explicit(&ready, memory_order_acquire)) {
        }
        printf("%d\n", sample);                     /* guaranteed to see 42 */
        pthread_join(thread, NULL);
        return 0;
    }
    ```

    The ordering contract: the *release* store makes every write before it (`sample = 42`)
    visible to a thread whose *acquire* load reads `true`. With `static volatile bool ready`
    instead, ThreadSanitizer reports `WARNING: ThreadSanitizer: data race` on `ready`, and
    nothing orders `sample` with respect to the flag. The atomic version runs clean under
    TSan. On an RTOS, an event flag or semaphore gives the same guarantee and lets the
    consumer block instead of spinning. An ISR-to-task flag on a single core still needs
    the atomic, or an RTOS primitive, so the compiler cannot reorder the data write past
    the flag.

??? note "Exercise 4: classifying registers from a peripheral manual"
    For a typical UART or timer chapter, sort each register by what its description says,
    not by its name:

    | Register (example) | Description says | Needs |
    | --- | --- | --- |
    | `DR` data | read pops the RX FIFO; write pushes TX | `volatile`; read exactly once per byte; never read "just to check" |
    | `SR` status | bits set by hardware | `volatile` reads; re-read in every loop iteration |
    | `ICR` interrupt clear | "write 1 to clear", reads as 0 | write only the bits to clear (`ICR = FLAG`), never `ICR \|= FLAG` |
    | `CR1` control | reserved bits "must be kept at reset value" | read-modify-write that preserves unknown bits |
    | `BSRR` bit set/reset | separate set and reset halves | the atomic alternative to read-modify-write of `ODR`; safe against interrupts |
    | `CR1` enable before a DMA start | "the buffer must be written before enabling" | a barrier (`__DSB()` on Arm) between the buffer write and the enable |
    | any register after clock enable | "wait 2 cycles after enabling the clock" | a dummy read-back or the documented delay |

    `volatile` is needed for all of them, and it is never sufficient on its own. The other
    columns come only from reading the register text, which is why this review is done
    against the manual and not against the header.

??? note "Exercise 5: reviewing qualifier casts"
    Find them with `grep -nE '\((const )?(volatile )?[a-z_0-9]+ \*\)' drivers/foo/*.c`, and
    compile once with `-Wcast-qual`, which flags every cast that drops a qualifier. Then
    classify:

    | Cast | Verdict |
    | --- | --- |
    | `legacy_write((char *)text)` where `legacy_write` only reads | acceptable as a temporary wrapper; fix `legacy_write`'s prototype instead |
    | `memset((void *)cfg, 0, n)` where `cfg` is `const` | bug: writes a `const` object, which may be in flash |
    | `(uint8_t *)rx_dma_buffer` dropping `volatile` for `memcpy` | acceptable *after* the ownership switch and cache invalidate; comment why |
    | `(volatile uint32_t *)&shared_flag` to "make it work" | bug: `volatile` is not synchronization; use an atomic |
    | `(volatile uint32_t *)(BASE + OFF)` | correct: this is how an MMIO address becomes a register pointer |

    Removing `const` is defined only if the object was not defined `const` and is not written
    through the result. Adding `volatile` at the point of access never makes a race safe.

??? note "Exercise 6: measuring a volatile polling loop"
    ```c
    #include <stdbool.h>
    #include <stdint.h>

    #define DWT_CYCCNT (*(volatile uint32_t *)0xE0001004u)   /* Cortex-M cycle counter */
    #define STATUS_READY 0x1u

    extern volatile uint32_t *const status_register;

    /* Returns the cycles spent waiting, or UINT32_MAX on timeout. */
    uint32_t wait_ready_measured(uint32_t timeout_cycles)
    {
        uint32_t start = DWT_CYCCNT;

        while ((*status_register & STATUS_READY) == 0u) {
            if ((uint32_t)(DWT_CYCCNT - start) >= timeout_cycles) {
                return UINT32_MAX;
            }
        }
        return (uint32_t)(DWT_CYCCNT - start);
    }
    ```

    The method, on the board:

    1. **Timeout:** log the returned cycle count over many runs, and force the device to
       stay busy to test the timeout path. The worst-case overshoot is one loop iteration
       past the deadline. Read it from the disassembly or measure it.
    2. **Cross-check:** toggle a GPIO around the call and measure the pulse with a scope or
       logic analyzer. This catches a cycle counter that is not enabled, or one that stops in
       sleep modes.
    3. **Power:** measure supply current while polling. A busy poll keeps the core at full
       run current for the whole wait. If the waits are long (more than tens of
       microseconds, typically), use the device's ready interrupt and `WFI`, then measure
       again.

    The `DWT` address is the standard Cortex-M one; the counter must be enabled
    (`DEMCR.TRCENA` and `DWT_CTRL.CYCCNTENA`) first, and Cortex-M0/M0+ cores do not have it.

## Common Mistakes

- Treating const as deep immutability or thread safety.
- Treating volatile as atomicity, locking, or a memory barrier.
- Marking every pointer restrict without proving non-aliasing.
- Casting away const from literal or read-only storage.
- Reading a volatile register multiple times when it has side effects.
- Using read-modify-write on a register with write-one-to-clear or reserved bits.
- Making a volatile flag do the work of a queue.
- Assuming atomic operations automatically make related non-atomic data safe.
- Hiding qualifier changes in typedefs.

## Debugging Checklist

1. Identify which object each qualifier applies to.
2. Inspect generated code for required volatile accesses, but do not infer synchronization from assembly alone.
3. Check register access width, barriers, and side effects in the hardware manual.
4. Test aliasing contracts with overlapping and non-overlapping inputs.
5. Run ThreadSanitizer or an RTOS race tool for host-representable synchronization.
6. Verify that a const-removing cast cannot reach read-only storage.
7. Check atomic memory order against the producer-consumer proof.
8. Document every target-specific qualifier macro.

## Related Topics

- [Semantics And Memory overview](./index.md)
- [Conversions, Promotions, And Aliasing](./conversions-promotions-and-aliasing.md)
- [Object Representation, Alignment, And Padding](./object-representation-alignment-and-padding.md)
- [C Memory Model And Concurrency](../advanced-c/c-memory-model-and-concurrency.md)
- [Embedded C And Hardware](../embedded-c-and-hardware/index.md)
- [C Programming](../index.md)

## References

- [ISO/IEC 9899 standards and drafts](https://open-std.org/jtc1/sc22/wg14/www/standards.html)
- [C11 public draft N1570](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n1570.pdf)
- [GCC volatile access documentation](https://gcc.gnu.org/onlinedocs/gcc/Volatiles.html)
- [GCC restrict and aliasing optimization documentation](https://gcc.gnu.org/onlinedocs/gcc/Optimize-Options.html)

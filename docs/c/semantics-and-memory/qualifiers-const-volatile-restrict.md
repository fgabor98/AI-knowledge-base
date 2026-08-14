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

static inline void set_bits(uint32_t address, uint32_t mask)
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

---
status: draft
reviewed: false
domain: c
difficulty: advanced
last_reviewed: null
---

# Memory-Mapped I/O

Memory-mapped I/O (MMIO) exposes peripheral registers in an address range accessed by load and store instructions. The C syntax may resemble ordinary memory, but registers have hardware-defined width, access, side effects, reset values, ordering, privilege, and clock requirements. Correct MMIO code begins with the device reference manual, not with a convenient cast.

## Learning Objectives

- represent register addresses, widths, masks, and fields safely;
- use `volatile` correctly without confusing it with atomicity or ordering;
- handle read-modify-write, write-one-to-clear, reserved, and side-effect bits;
- sequence clocks, resets, pin mux, status, and data operations;
- isolate vendor headers and create testable register access layers;
- diagnose bus faults and behavior caused by wrong access width or ordering.

## Register Contract

For each register, record:

- address and bus region;
- access width and alignment;
- read/write/clear/set behavior;
- reset value and power-domain behavior;
- reserved bits and required write values;
- side effects of reads and writes;
- clock, reset, privilege, and synchronization requirements;
- ordering relative to other registers and memory;
- interrupt/DMA interaction;
- timeout and error behavior.

The C type should reflect the access width, but the type alone cannot encode side effects:

~~~c
#include <stdint.h>

#define REG32(address) (*(volatile uint32_t *)(address))

#define TIMER_BASE      0x40010000u
#define TIMER_CONTROL   REG32(TIMER_BASE + 0x00u)
#define TIMER_STATUS    REG32(TIMER_BASE + 0x04u)
#define TIMER_CLEAR     REG32(TIMER_BASE + 0x08u)

#define TIMER_ENABLE    (1u << 0)
#define TIMER_IRQ_EN    (1u << 1)
#define TIMER_DONE      (1u << 0)
~~~

This style is illustrative. Production code should use the device header or a project wrapper that validates address, width, and target identity. A register pointer must not be dereferenced on a host unless a simulation maps the access intentionally.

## `volatile` Is Not Synchronization

`volatile` tells the C implementation that accesses are observable and must not be optimized away or merged in ways forbidden by the volatile rules. It does not:

- make a read-modify-write atomic;
- order normal memory relative to a device;
- flush CPU caches;
- synchronize two threads;
- make a multiword register access indivisible;
- guarantee that a peripheral has completed a write.

Use C atomics for language-level shared-memory synchronization, compiler barriers for compiler ordering, architecture/device barriers for hardware ordering, and vendor/OS primitives for cache and peripheral completion.

## Access Width And Alignment

Writing a 32-bit register through an 8-bit pointer can trigger a different bus transaction or be ignored. A misaligned access can fault or be split. Follow the manual’s access width and alignment exactly. Avoid C bit-fields for MMIO unless the compiler, ABI, endian, and access behavior are explicitly controlled; masks and shifts are more visible.

## Bit Fields And Masks

Use named masks and shifts:

~~~c
#include <stdint.h>

#define CLOCK_DIV_SHIFT  4u
#define CLOCK_DIV_MASK   (0x0fu << CLOCK_DIV_SHIFT)

static uint32_t field_prep(uint32_t mask, unsigned shift, uint32_t value)
{
    return (value << shift) & mask;
}

static uint32_t field_get(uint32_t mask, unsigned shift, uint32_t value)
{
    return (value & mask) >> shift;
}
~~~

Validate that values fit before shifting when an out-of-range value must be rejected rather than truncated. Keep masks typed consistently with the register width.

## Read-Modify-Write Hazards

This is unsafe when a register contains write-one-to-clear, write-only, read-to-clear, or concurrently changing bits:

~~~c
#include <stdint.h>

static volatile uint32_t timer_control;
#define TIMER_CONTROL timer_control
#define TIMER_ENABLE  (1u << 0)

static void enable_timer(void)
{
    TIMER_CONTROL |= TIMER_ENABLE;
}
~~~

The compiler emits a read followed by a write. The read may have side effects, and the write may repeat bits that should not be written. Prefer a dedicated set/clear register, a vendor-provided atomic bit operation, or a carefully documented sequence:

~~~c
#include <stdint.h>

static volatile uint32_t timer_control;
#define TIMER_CONTROL timer_control
#define TIMER_IRQ_EN  (1u << 1)
#define TIMER_ENABLE  (1u << 0)

static void configure_timer(void)
{
    uint32_t control = TIMER_CONTROL;
    control &= ~TIMER_IRQ_EN;
    control |= TIMER_ENABLE;
    TIMER_CONTROL = control;
}
~~~

This is safe only if the manual says the bits are stable/readable and the update is atomic enough. If an interrupt or another bus master can change the register, use a hardware atomic register or a critical section appropriate to the device.

## Write-One-To-Clear And Reserved Bits

For a write-one-to-clear status bit, writing the value read from the register may clear every currently set event, including one another subsystem has not handled. Clear only the intended bit:

~~~c
#include <stdint.h>

static volatile uint32_t timer_clear;
#define TIMER_CLEAR timer_clear
#define TIMER_DONE  (1u << 0)

static void clear_timer_done(void)
{
    TIMER_CLEAR = TIMER_DONE;
}
~~~

Reserved bits may need to be written zero or preserved. Do not write a guessed all-ones value. Some peripherals require a key, unlock sequence, or write/readback delay for protected registers.

## Ordering And Completion

A sequence such as “write descriptor, then set doorbell” requires the peripheral to observe the descriptor before the notification. The necessary ordering may include:

- compiler barrier;
- CPU memory barrier;
- cache clean/flush;
- peripheral write buffer completion;
- readback of a safe register;
- device-specific synchronization bit.

Use the architecture/vendor primitive and document which agents are ordered. A C `volatile` store is not a universal device barrier.

## Peripheral Structures

Struct overlays can improve readability when the vendor ABI and offsets are verified:

~~~c
#include <stdint.h>

typedef struct {
    volatile uint32_t control;  /* 0x00 */
    volatile uint32_t status;   /* 0x04 */
    volatile uint32_t data;     /* 0x08 */
    uint32_t reserved[5];       /* 0x0c..0x1c */
} timer_registers_t;

_Static_assert(sizeof(timer_registers_t) == 0x20u,
               "timer register block size");

#define TIMER ((timer_registers_t *)0x40010000u)
~~~

Use `offsetof` assertions for important fields and verify the compiler’s layout. Do not assume a struct overlay handles endian, bus width, access side effects, or secure/non-secure aliasing.

## Host Testing

Separate the register policy from the access mechanism:

~~~c
#include <stddef.h>
#include <stdint.h>

#define TIMER_ENABLE (1u << 0)

struct timer_io {
    uint32_t (*read_control)(void *context);
    void (*write_control)(void *context, uint32_t value);
    void (*write_clear)(void *context, uint32_t value);
    void *context;
};

int timer_enable(struct timer_io *io)
{
    uint32_t value;
    if (io == NULL || io->read_control == NULL || io->write_control == NULL) {
        return -1;
    }
    value = io->read_control(io->context);
    io->write_control(io->context, value | TIMER_ENABLE);
    return 0;
}
~~~

A fake can model changing status, write-one-to-clear behavior, illegal widths, and reset values. The hardware test must still verify the actual bus transaction and peripheral response.

## Exercises

1. Build a register contract table from a peripheral manual.
2. Implement field encode/decode helpers with range checks.
3. Model a write-one-to-clear register and test accidental event loss.
4. Compare read-modify-write with dedicated set/clear hardware registers.
5. Add register offset and block-size assertions to a device header.
6. Use a logic analyzer or trace to verify access width and ordering.
7. Create a host fake that models reset, status side effects, and illegal access order.

## Common Mistakes

- using `volatile` as a replacement for atomicity, cache maintenance, or barriers;
- reading registers whose reads clear or acknowledge events;
- using read-modify-write on write-one-to-clear or write-only registers;
- writing reserved bits with guessed values;
- using the wrong width or unaligned address;
- assuming struct overlays define hardware behavior by themselves;
- forgetting clocks, pin mux, reset release, privilege, or secure aliases;
- testing only a memory fake and never verifying the bus transaction;
- placing MMIO access in portable policy code.

## Related Topics

- [Interrupts, Exceptions, And Faults](./interrupts-exceptions-and-faults.md)
- [DMA, Cache, And Memory Barriers](./dma-cache-and-memory-barriers.md)
- [Peripheral Drivers](./peripheral-drivers.md)
- [Qualifiers: const, volatile, restrict](../semantics-and-memory/qualifiers-const-volatile-restrict.md)
- [Compiler Modes, Warnings, And Optimization](../compilation-linking-and-abi/compiler-modes-warnings-and-optimization.md)

## References

- [CMSIS device header guidance](https://arm-software.github.io/CMSIS_5/Core/html/device_h_pg.html)
- [CMSIS compiler control and volatile access](https://arm-software.github.io/CMSIS_5/develop/Core/html/group__compiler__conntrol__gr.html)
- [C11 public draft N1570, volatile and atomics](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n1570.pdf)

---
status: draft
reviewed: false
domain: c
difficulty: advanced
last_reviewed: null
---

# Microcontroller Platforms

Microcontrollers (MCUs) integrate a processor core, nonvolatile program storage,
working memory, clocks, reset logic, timers, communication peripherals, and often
analog circuitry in one device. C code for an MCU is usually freestanding: there may
be no operating system, no process isolation, no virtual memory, and only a small
subset of the hosted C library. The hardware and the startup/linker environment are
therefore part of the program's effective runtime.

## Learning Objectives

- Distinguish an MCU from an application processor and choose an appropriate software
  architecture for each.
- Relate C objects and linker sections to flash, SRAM, TCM, backup RAM, and MMIO.
- Understand reset, clock, interrupt, protection, low-power, and watchdog concerns.
- Select a HAL/BSP boundary that preserves portability without hiding timing or error
  behavior.
- Validate platform assumptions with map files, register documentation, fault data, and
  hardware measurements.

## MCU And MPU Resource Models

An MCU generally optimizes for deterministic control, low power, low cost, and direct
peripheral access. An application processor (MPU) generally provides a richer memory
hierarchy, MMU, caches, multiple privilege levels, and an operating system. The border
is not absolute: a high-end MCU can have caches and multiple cores, while a small MPU
can run bare metal.

| Concern | Typical MCU | Typical MPU |
| --- | --- | --- |
| Program storage | On-chip flash or ROM | External flash/storage loaded into RAM or mapped through a controller |
| Working memory | SRAM, optional TCM/backup RAM | DRAM with cache hierarchy |
| Protection | MPU, secure extensions, board policy | MMU, process isolation, hypervisor/secure monitor |
| Startup | Vector table, clock/reset setup, data copy, zeroing | Boot ROM, bootloader, firmware stages, kernel, root filesystem |
| Timing | Often bounded and interrupt-driven | Caches, MMU, scheduling, and contention complicate bounds |
| C library | Freestanding runtime or small libc | Hosted libc plus OS services |

This distinction changes design decisions. A pointer to an MCU peripheral is commonly
an address in a fixed memory map. A pointer in an MPU userspace process is usually a
virtual address whose validity and permissions depend on the MMU and operating system.

## The MCU Memory Map

The reference manual—not the C standard—defines the meaning of a peripheral address.
Typical regions include:

- **Code flash/ROM:** executable and usually read-only at runtime; instruction fetches
  may stall while wait states are inserted.
- **Main SRAM:** ordinary read/write data, stacks, heaps, and often DMA buffers.
- **Tightly coupled memory (TCM):** core-local memory with predictable latency, when
  present; it may not be visible to every bus master.
- **Backup/retention RAM:** survives selected reset or low-power modes, but has a
  separate initialization and validity protocol.
- **External memory:** mapped through a controller and sensitive to timing, pin mux,
  cacheability, and boot configuration.
- **MMIO windows:** registers with access widths, side effects, reset values, and
  ordering requirements.

The linker script maps sections to these regions. A declaration such as
`__attribute__((section(".fast_code")))` is not enough: the linker must define the
section, place it in a region, and possibly arrange a load address plus a runtime
execution address. Startup code must copy or initialize it before use.

### Representing device registers

Use vendor-provided register definitions only after checking that their types and
access widths match the reference manual. A `volatile` qualified member tells the
compiler that an access is observable and must not be removed or freely merged with
other volatile accesses. It does not by itself make a multi-register operation
atomic, make a peripheral ready, or order a DMA transaction.

```c
#include <stdint.h>

struct timer_registers {
    volatile uint32_t control;
    volatile uint32_t status;
    volatile uint32_t count;
    volatile uint32_t compare;
};

static struct timer_registers *const timer0 =
    (struct timer_registers *)(uintptr_t)0x40000000u;

enum { TIMER_ENABLE = 1u << 0, TIMER_STATUS_READY = 1u << 0 };

static void timer_start(uint32_t compare)
{
    timer0->compare = compare;
    timer0->control = TIMER_ENABLE;
}

static int timer_ready(void)
{
    return (timer0->status & TIMER_STATUS_READY) != 0u;
}
```

The example expresses an MMIO contract but cannot be tested safely on a host because
the address is target-specific. Production code should put such definitions in a
small platform module and test the higher-level timer policy against a fake interface.
Document whether a read clears a bit, whether writes require a key sequence, and
whether a register is safe to access from an interrupt or DMA callback.

## Reset And Startup

The reset path commonly performs these steps:

1. Reset logic selects a vector/entry address and an initial stack or boot mode.
2. Assembly or compiler startup establishes the stack and minimal CPU state.
3. Runtime data sections are copied from a load image to RAM; zero-initialized sections
   are cleared.
4. Clocks, flash wait states, pin multiplexing, protection, and memory attributes are
   configured in a safe order.
5. C runtime initialization runs, if supplied; this may include constructors in a
   C/C++ image, libc setup, and hooks.
6. `main` starts the application, which may create tasks and enable interrupts.

Do not enable an interrupt before its vector, stack, clock, peripheral reset state, and
handler-visible data are ready. If a watchdog is active from reset, the startup budget
must include the time spent copying and clearing sections. A bootloader hand-off also
needs a documented contract for vector location, clocks, image validity, stack, and
reset reason.

## Clocks, Flash, And Determinism

Changing the CPU or bus clock is a system operation, not just writing a divider. The
sequence may require a clock source to stabilize, a voltage range change, flash wait
state configuration, peripheral divider updates, and status verification. Running
flash faster than its supported access time can produce intermittent instruction or
data corruption.

Even without caches, flash wait states and bus arbitration affect latency. Place only
latency-critical code or tables in RAM/TCM when measurement justifies it. Marking a
function “fast” without checking its literal pools, called functions, interrupt
vectors, and data dependencies can leave a hidden flash access on the critical path.

## Interrupts, Exceptions, And Protection

MCU interrupt controllers differ in vector representation, pending behavior, priority
encoding, nesting, and context stacking. A handler must be short enough for the
system's latency budget and must acknowledge the device according to its protocol.
Use an interrupt to capture state and signal deferred work when the operation can
block, take an unbounded lock, or perform a long transfer.

An MPU can divide memory into regions with permissions and attributes such as
read-only, execute-never, device, or shareable. Region priority, alignment, subregions,
and default-map behavior are platform-specific. Protection is useful for catching
stack overflow and isolating tasks, but only if every task stack, DMA buffer, and
peripheral access is represented in the policy.

## Low Power And Reset Domains

Sleep, stop, standby, and backup modes can change clock availability, SRAM retention,
register state, interrupt wake sources, and debugger behavior. Treat wake-up as a
partial reboot of some hardware domains:

- Save or reconstruct peripheral configuration that is not retained.
- Re-establish clocks before using peripherals or time bases.
- Revalidate RAM markers with a version, checksum, and reset-cause check.
- Reinitialize communication links whose peer may not have slept with you.
- Define which timers and watchdogs continue running.

Reset is also partitioned. A peripheral reset, system reset, brownout reset, watchdog
reset, and debug reset may leave different state behind. Record the reset reason early,
before later initialization clears or overwrites it.

## HAL, BSP, And Direct Register Access

A useful layering is:

```text
application policy
        |
portable driver/protocol logic
        |
BSP/HAL: clocks, pins, IRQ hookup, register access, DMA/cache operations
        |
vendor SDK and startup + linker script
        |
silicon
```

Keep timing, ownership, error, and interrupt-context behavior visible at the boundary.
A HAL that turns every action into a blocking call can destroy a real-time guarantee;
a HAL that exposes every register can make portability and review difficult. Prefer
small interfaces with explicit state transitions, timeouts, and buffer ownership.

## Exercises And Diagnostics

1. Draw the target memory map and annotate every linker section, stack, heap, DMA pool,
   vector table, and bootloader boundary.
2. Trace reset to `main` with a debugger or GPIO marker. Measure the time before the
   first interrupt can be serviced.
3. Deliberately overflow a protected task stack in a test image and capture the fault
   address, stacked registers, and reset behavior.
4. Compare a flash-resident and RAM-resident implementation with a cycle counter or
   trace tool; verify that called code and constants are where expected.
5. Replace a direct-register driver with a fake register block on the host and test
   policy, timeout, and error paths without touching hardware.

## Common Mistakes

- Assuming every RAM address is equally visible to the CPU, DMA, debugger, and other
  cores.
- Using `volatile` as a substitute for atomicity, barriers, cache maintenance, or a
  peripheral protocol.
- Enabling interrupts before vector tables, clocks, flags, and handler state are ready.
- Treating a vendor HAL's default clock or pin configuration as a guaranteed product
  configuration.
- Forgetting flash wait states or peripheral clock domains when changing frequency.
- Placing a buffer in a linker section without checking startup initialization and DMA
  reachability.
- Reusing retained RAM without validating its reset and power-domain assumptions.

## Related Topics

- [Platform-Specific C overview](./index.md)
- [ARM Cortex-M](./arm-cortex-m.md)
- [Embedded C And Hardware](../embedded-c-and-hardware/index.md)
- [Memory And Object Semantics](../semantics-and-memory/index.md)
- [Compilation, Linking, And ABI](../compilation-linking-and-abi/index.md)

## References

- [Arm Cortex-M documentation](https://developer.arm.com/Processors/Cortex-M)
- [CMSIS-Core documentation](https://arm-software.github.io/CMSIS_5/Core/html/index.html)
- [CMSIS startup file documentation](https://arm-software.github.io/CMSIS_5/5.8.0/Core/html/startup_c_pg.html)
- [CMSIS NVIC documentation](https://arm-software.github.io/CMSIS_5/Core/html/group__NVIC__gr.html)
- The target MCU datasheet, reference manual, errata, memory map, and vendor SDK

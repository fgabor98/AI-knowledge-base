---
status: draft
reviewed: false
domain: c
difficulty: advanced
last_reviewed: null
---

# Startup, Reset, And Vector Tables

Startup is the bridge between a processor reset state and a valid C execution environment. It establishes the stack, selects or verifies the memory map, initializes data and bss, configures clocks and protection, installs vector tables, and eventually enters the application or RTOS. A fault here occurs before normal logging and often before the C library is ready.

## Learning Objectives

- trace cold, warm, watchdog, brownout, and software reset paths;
- understand vector-table placement, initial stack, reset handler, and weak defaults;
- order clocks, memory, C runtime, security, and interrupt initialization correctly;
- perform data/bss initialization with linker-defined boundaries;
- define bootloader handoff state and returned-`main` behavior;
- debug pre-`main` faults with minimal evidence.

## Reset Classes

Do not treat “reset” as one event:

| Reset | Possible preserved state | Questions |
| --- | --- | --- |
| power-on/brownout | little or retention-dependent | are clocks, RAM, and peripherals in reset defaults? |
| external pin | product-dependent | what boot mode and pins are sampled? |
| watchdog | retention RAM may survive | what evidence identifies the previous failure? |
| software/system | many registers may persist | which drivers must reinitialize fully? |
| bootloader handoff | bootloader-configured state | what stack, vector, cache, clock, and interrupt state is promised? |

Record reset cause before later initialization can clear or overwrite it. Make repeated reset behavior safe and bounded.

## Vector Table

On Cortex-M, the vector table begins with the initial main stack value and reset handler, followed by processor exceptions and device interrupts. It is normally placed in program memory and may be relocated to RAM through the vector-table register when the architecture/device supports it. Other architectures use different formats; always use the processor and device manual.

A conceptual C table:

~~~c
#include <stdint.h>

typedef void (*handler_t)(void);

extern unsigned char __initial_sp__;
void Reset_Handler(void);
void Default_Handler(void);

__attribute__((section(".isr_vector"), used, aligned(256)))
const uintptr_t vector_table[] = {
    (uintptr_t)&__initial_sp__,
    (uintptr_t)&Reset_Handler,
    (uintptr_t)&Default_Handler,
    (uintptr_t)&Default_Handler,
};
~~~

The alignment, pointer encoding, mode bits, number of entries, and section retention are target-specific. CMSIS startup templates also commonly provide weak device handlers aliased to a default handler. Verify that the vector order matches the device’s IRQ enumeration and startup file.

## Minimal Reset Sequence

A typical sequence is:

1. processor loads the initial stack and reset entry from the vector table;
2. reset handler establishes CPU mode, stack limit, fault behavior, and early clock state;
3. boot metadata/reset cause is captured;
4. memory controller and external RAM are initialized if needed;
5. `.data` is copied from its load address to its run address;
6. `.bss` and other zero sections are cleared;
7. clocks, pin mux, MPU/TrustZone, caches, and low-level services are configured;
8. C runtime and initialization arrays are processed if enabled;
9. `main` or the RTOS entry is called;
10. interrupts are enabled according to a documented policy.

The ordering changes by device. For example, an external RAM-backed `.data` region cannot be copied before the memory controller works, and enabling interrupts before the vector table and handlers are valid can create an immediate fault.

## Data And BSS Initialization

The linker provides boundaries for initialized and zero-initialized regions. A startup implementation may use a small loop or a trusted runtime helper:

~~~c
#include <stddef.h>

extern unsigned char __data_load__[];
extern unsigned char __data_start__[];
extern unsigned char __data_end__[];
extern unsigned char __bss_start__[];
extern unsigned char __bss_end__[];

static void initialize_c_memory(void)
{
    size_t data_size = (size_t)(__data_end__ - __data_start__);
    for (size_t i = 0u; i < data_size; ++i) {
        __data_start__[i] = __data_load__[i];
    }
    for (unsigned char *p = __bss_start__; p != __bss_end__; ++p) {
        *p = 0u;
    }
}
~~~

Test empty ranges and verify the load/run addresses in the map. If memory protection, cache, ECC initialization, or external RAM setup is required, it belongs before the corresponding region is touched.

## Clocks, Pins, And Memory Protection

Clock configuration affects every later timing assumption. Establish:

- oscillator/PLL source and lock status;
- CPU and bus frequencies;
- flash wait states;
- peripheral clock gates;
- pin mux and electrical configuration;
- watchdog window and refresh policy;
- MPU/SAU regions and default fault policy;
- cache and branch predictor state where applicable.

Validate that the configured clock matches the value used by delay, baud, RTOS tick, timeout, and peripheral drivers. A wrong `SystemCoreClock`-like variable can make the system appear intermittently faulty.

## Interrupt Enablement

Keep interrupts disabled until:

- vector table location and entries are valid;
- interrupt status is cleared or intentionally preserved;
- handlers and their stack have been initialized;
- driver state and RTOS objects exist;
- priorities and masking rules are configured.

Enable sources one at a time during bring-up. An interrupt pending from before initialization can otherwise execute code against zeroed or uninitialized state.

## C Runtime And RTOS Entry

The startup file may call a runtime entry that initializes constructors, libc, TLS, heap, or other support before `main`. If a project bypasses that entry, it must decide which features are intentionally unavailable. An RTOS may start from `main`, or startup may create the scheduler directly; document the chosen model.

Never start the scheduler before the system clock, interrupt priority configuration, allocator, and idle/timer resources satisfy the RTOS port’s requirements. Do not use RTOS APIs from early interrupts unless the port explicitly permits it.

## Bootloader Handoff

Define an application-entry contract:

- application vector base and image address;
- stack pointer and ABI alignment;
- reset reason and boot metadata registers;
- interrupt enable, pending, and priority state;
- cache, MPU/TrustZone, clocks, and peripheral ownership;
- watchdog deadline;
- image authenticity and rollback result;
- whether the application must relocate vectors or reinitialize everything.

The safest handoff is usually one that leaves a small, documented state and lets the application reset/reinitialize what it owns. Do not rely on accidental bootloader state.

## Pre-`main` Debugging

Use a minimal fault record or debugger breakpoint at reset:

1. verify image bytes at the vector location;
2. inspect initial SP and reset PC;
3. single-step the first memory and clock operations;
4. check linker symbols and region boundaries;
5. verify `.data` source and destination;
6. check bss clearing does not overwrite stack, retention, or boot data;
7. inspect fault status and reset cause;
8. enable one interrupt only after its driver state is ready.

Avoid semihosting or complex formatted I/O as the only early diagnostic. A GPIO pattern, retention record, or minimal polled UART is more reliable.

## Exercises

1. Draw cold, watchdog, and bootloader-handoff startup state machines.
2. Inspect a CMSIS-style vector table and match each entry to its IRQ number.
3. Test data-copy and bss-clear loops with empty and non-empty sections.
4. Intentionally enable an interrupt before its state is ready and diagnose the failure.
5. Measure clock configuration against a GPIO or timer reference.
6. Implement a pre-`main` fault record and symbolize the recorded PC.
7. Write and test a bootloader/application handoff contract.

## Common Mistakes

- assuming all reset causes leave identical register and RAM state;
- placing or aligning the vector table incorrectly;
- using a vector order that does not match the device IRQ enumeration;
- enabling interrupts before handlers and driver state exist;
- copying `.data` before external memory is initialized;
- clearing retention, boot metadata, or stack memory as bss;
- using the wrong CPU clock in timeout and baud calculations;
- bypassing required C runtime initialization;
- depending on undocumented bootloader state;
- debugging early faults only through a service that is initialized later.

## Related Topics

- [Startup, Runtime, And `main`](../compilation-linking-and-abi/startup-runtime-and-main.md)
- [Linker Scripts And Memory Layout](../compilation-linking-and-abi/linker-scripts-and-memory-layout.md)
- [Interrupts, Exceptions, And Faults](./interrupts-exceptions-and-faults.md)
- [Bootloaders And Firmware Images](./bootloaders-and-firmware-images.md)
- [Freestanding C](./freestanding-c.md)

## References

- [CMSIS startup file](https://arm-software.github.io/CMSIS_5/5.8.0/Core/html/startup_c_pg.html)
- [CMSIS vector table and NVIC](https://arm-software.github.io/CMSIS_5/5.7.0/Core/html/group__NVIC__gr.html)
- [Using CMSIS in embedded applications](https://arm-software.github.io/CMSIS_5/Core/html/using_pg.html)
- [Arm Application Binary Interface repository](https://github.com/ARM-software/abi-aa)

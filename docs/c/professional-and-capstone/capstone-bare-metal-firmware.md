---
status: draft
reviewed: false
domain: c
difficulty: advanced
last_reviewed: null
---

# Capstone: Bare-Metal Firmware

Build a small bare-metal firmware image for a documented Cortex-M or comparable MCU.
The project must start from reset, initialize the memory image and clocks, use a timer
interrupt to drive bounded work, expose diagnostic output, survive a deliberate fault,
and prove the final image fits the linker-defined memory map.

This capstone is not complete when an LED blinks. The deliverable is an explainable
boot/runtime system with startup, linker, interrupt, watchdog, fault, and release
contracts.

## Project Brief

Choose a specific board/MCU and record:

- core revision and instruction/FPU options;
- flash, SRAM, retention, TCM, and peripheral map;
- clock/reset/watchdog behavior;
- vector-table and interrupt-controller rules;
- debugger/programmer and bootloader assumptions;
- compiler, assembler, linker, libc, SDK, and C dialect;
- board revision, power supply, and external connections.

Implement a firmware service that:

- boots from the documented reset entry;
- copies/clears runtime sections correctly;
- configures clocks and flash wait states safely;
- configures a GPIO heartbeat and UART/event log;
- uses a periodic timer interrupt to schedule bounded work;
- services a watchdog from a health policy, not an unconditional loop;
- records reset causes and fault context;
- has a tested flash image layout and versioned metadata;
- can be debugged from the map file, symbols, and fault record.

## Startup Contract

Trace and document:

1. reset/vector fetch and initial stack;
2. reset handler and CPU state;
3. data load-to-run copy and `.bss` clearing;
4. clock/power/flash setup;
5. vector relocation, MPU/security setup, and fault enables;
6. C runtime/libc prerequisites;
7. board/peripheral initialization;
8. `main`, scheduler/loop entry, and first interrupt enable.

Before each step, state which memory and peripherals are safe to access. Do not use a
UART, heap, or formatted logging function before its clock, storage, and runtime
contracts are ready. Do not enable an interrupt before the vector, handler state,
stack, clock, and interrupt source are prepared.

## Linker Script And Memory Layout

The linker script must define memory regions and placement policy. Review:

- vector table location and alignment;
- executable/read-only code and constants;
- initialized data load/run addresses;
- zero-initialized data and no-init/retained sections;
- main and interrupt stack bounds/guards;
- heap boundaries or the deliberate absence of a heap;
- DMA/non-cacheable/device buffers;
- bootloader/application/update slots;
- image metadata, checksum, signature, and reserved gaps.

A simplified linker concept is:

```text
FLASH:  vector + text + rodata + image metadata
RAM:    data + bss + heap/pools + stacks
NOINIT: reset-retained diagnostics or validated boot state
```

The actual script must account for alignment, load addresses, overlays, startup symbols,
garbage collection, and post-link image tools. Add linker assertions for region length,
stack/heap collision, metadata placement, and reserved boundaries. Treat the map file
as a release artifact, not a temporary build log.

## Vector Table And Interrupts

Verify every vector entry, weak default, handler spelling, priority, and source-clear
sequence. A default handler should capture the interrupt identity or stop visibly; a
silent infinite loop can disguise a wrong symbol.

For the periodic timer:

- choose a clock source and calculate the reload value with checked arithmetic;
- define drift, wraparound, and missed-tick policy;
- acknowledge/clear the source in the correct order;
- capture minimal state in the ISR;
- defer work that can block, allocate, or run too long;
- measure entry-to-exit and worst-case preemption latency.

The ISR/task boundary needs an explicit handoff. A volatile counter may be sufficient
for one target-specific observation, but a queue, atomic, or interrupt-safe primitive
is needed when multiple fields or ownership are involved.

## GPIO And UART Diagnostics

Use GPIO markers for timing evidence and a UART/event transport for human-readable or
binary diagnostics. Define:

- register access width and required barriers;
- pin mux and clock initialization;
- output buffering and backpressure;
- behavior when the transport is disconnected or full;
- whether logging is safe from the ISR/fault context;
- format version, build ID, timestamp source, and reset reason.

Fault and interrupt paths should use a fixed-size binary record or minimal polled sink,
not `printf` through an uninitialized or blocking stack. Decode records on the host.

## Watchdog Policy

A watchdog is useful only when it detects loss of required progress. Define health
conditions such as:

- main loop/task heartbeat;
- timer/interrupt progress;
- peripheral service progress;
- queue drain/backpressure state;
- memory/invariant checks;
- last successful checkpoint.

Feed the watchdog only after those conditions are met. Record the watchdog reset cause
and last health state in retained/backup storage with a version, length, checksum, and
validity marker. Test a stuck loop, blocked peripheral, interrupt storm, corrupted
record, and watchdog-disabled debug mode. Ensure debug halt behavior is deliberate.

## Fault Handler And Crash Record

Enable configurable fault classes early and capture:

- active stack pointer and stacked core registers;
- return/context information and faulting PC;
- configurable/hard/bus/memory/usage fault status;
- fault address when valid;
- active interrupt and reset cause;
- build ID, image slot, and a monotonic crash counter.

The handler must be safe when the heap, scheduler, clocks, and filesystem are unusable.
Guard against recursive faults, validate retained storage, and choose a product action:
halt for debug, persist-and-reset, enter a recovery image, or report to a bootloader.

Provide a host decoder that maps the saved PC to symbols and source using the exact ELF
and compiler artifacts. Test a deliberate undefined instruction, invalid access,
stack overflow/protection fault, and bad vector entry in a controlled image.

## Flash And Update Layout

Define how the image is programmed, authenticated, selected, and rolled back. Include:

- immutable boot region and version policy;
- application slot(s) and metadata;
- image length, checksum/signature, and compatibility fields;
- power-loss behavior during programming;
- downgrade prevention or authorized recovery;
- vector remap/jump hand-off;
- application/bootloader ownership of clocks, interrupts, and watchdog.

Test power interruption at every update stage and verify that at least one bootable
image remains. Do not let application code erase its own executing flash region without
a documented memory/controller procedure.

## Timing And Resource Budget

Measure and report:

- reset-to-main and reset-to-first-service time;
- interrupt period/jitter and maximum masked interval;
- ISR and deferred-work worst-case duration;
- stack watermarks and guard behavior;
- static RAM/flash map and runtime pool use;
- watchdog margin under load;
- power/sleep behavior if applicable.

Use the exact release optimization, linker, and clock configuration. State cache/flash
wait-state, DMA, interrupt, debugger, and temperature conditions. A debug build with
semihosting is not evidence for the production timing budget.

## Test Plan

### Host and build tests

- compile register-independent policy with host warnings/sanitizers;
- test clock/reload arithmetic and state machines against a reference;
- validate image metadata and CRC/signature parsing;
- test fault-record decoding with synthetic frames;
- assert linker symbols and generated configuration.

### Target tests

- cold, warm, watchdog, brownout, software, and bootloader reset;
- every interrupt/vector and default-handler path;
- timer drift, missed ticks, and interrupt load;
- UART full/disconnected/error behavior;
- deliberate fault capture and reboot/recovery;
- stack/MPU/guard faults and retained-memory validation;
- flash image boundary, update interruption, and rollback;
- timing, power, and temperature corners.

## Deliverables

- source, linker script, startup/vector files, board configuration, and build commands;
- map file, symbols, disassembly, image metadata, checksum/signature, and build manifest;
- reset/boot sequence and memory-map diagrams;
- fault record format and host decoder;
- timing/resource/power report;
- hardware test procedure and results;
- recovery/update instructions and known limitations;
- review record covering startup, interrupts, watchdog, fault, and linker assumptions.

## Milestones

1. Board/toolchain contract, memory map, and boot design.
2. Reset-to-`main` image with verified linker sections.
3. GPIO/UART diagnostics and timer interrupt.
4. Watchdog health policy and reset-cause record.
5. Fault capture/decoder and deliberate-fault tests.
6. Image metadata/update/recovery path.
7. Release timing, map, artifact, and hardware evidence review.

## Common Mistakes

- Treating startup and linker files as vendor boilerplate that does not need review.
- Enabling interrupts before vectors, clocks, stack, and handler state are ready.
- Feeding the watchdog unconditionally instead of proving system health.
- Using formatted logging or heap allocation in a fault/ISR path.
- Assuming the stacked PC always identifies a precise faulting instruction.
- Failing to account for load/run addresses, `.noinit`, DMA, or retained reset domains.
- Measuring timing only in a debug build or with no interrupt/DMA contention.
- Updating flash without a power-loss and rollback design.

## Related Topics

- [Professional Practice And Capstones overview](./index.md)
- [Embedded C And Hardware](../embedded-c-and-hardware/index.md)
- [Startup, Runtime, And main](../compilation-linking-and-abi/startup-runtime-and-main.md)
- [Linker Scripts And Memory Layout](../compilation-linking-and-abi/linker-scripts-and-memory-layout.md)
- [ARM Cortex-M](../platform-specific-c/arm-cortex-m.md)
- [Debugging With GDB](../correctness-quality-and-security/debugging-with-gdb.md)

## References

- [CMSIS startup code](https://arm-software.github.io/CMSIS_5/5.8.0/Core/html/startup_c_pg.html)
- [CMSIS NVIC documentation](https://arm-software.github.io/CMSIS_5/Core/html/group__NVIC__gr.html)
- [GNU ld linker script documentation](https://sourceware.org/binutils/docs/ld/Scripts.html)
- [OpenOCD documentation](https://openocd.org/doc-release/html/index.html)
- The exact MCU reference manual, errata, bootloader/update specification, linker
  script, debugger configuration, and board schematic

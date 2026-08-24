---
status: draft
reviewed: false
domain: c
difficulty: advanced
last_reviewed: null
---

# ARM Cortex-M

Cortex-M processors are designed around exception-driven embedded control. The core
profiles share a programmer model and the NVIC concept, but differ in instruction
sets, optional floating-point/DSP support, security extensions, debug features, and
memory-system behavior. Always combine the generic Arm architecture documentation
with the exact core revision and the MCU's integration manual.

## Learning Objectives

- Explain Cortex-M reset, exception entry, stacking, return, and priority behavior.
- Use CMSIS and vendor startup code without confusing convenience interfaces with the
  hardware contract.
- Diagnose HardFault, MemManage, BusFault, and UsageFault failures from saved context.
- Design safe vector-table, MPU, TrustZone-M, FPU, and low-power integration.
- Understand which code may execute in an interrupt and how to measure latency.

## Core Families And The Thumb Execution Model

Cortex-M implementations execute Thumb instructions; the exact Thumb subset depends
on the core. Older and smaller profiles have fewer instructions and architectural
features. Cortex-M4/M7-class designs commonly add DSP instructions and an optional
single-precision FPU. Cortex-M23/M33-class designs can add Armv8-M security features,
including TrustZone-M. Cortex-M55-class designs add newer Helium/vector capabilities.
Do not infer support from a product name: inspect the compiler target flags, CPUID
information, vendor header, and core documentation.

The compiler's `-mcpu`, `-mthumb`, floating-point ABI, and FPU options must agree with
the actual core and with every linked object. A wrong floating-point ABI can fail at
link time, while a wrong instruction or erratum setting can fail only on hardware.
Record these options in the build metadata and test that all libraries use the same
contract.

## Reset And The Vector Table

A typical Cortex-M vector table contains an initial main stack pointer followed by the
reset handler, core exception entries, and device interrupt entries. Entries are
function pointers with the architectural Thumb-state encoding expected by the core.
The table may reside at the default address or be relocated by the System Control Block
`VTOR` register when the MCU and security configuration support it.

CMSIS startup files usually provide:

- the vector table and weak default handlers;
- reset code that initializes data and calls system initialization;
- a stack symbol and optional heap boundary;
- weak aliases that let an application replace a handler by defining the same symbol.

Weak defaults are convenient but can hide a misspelled handler name. Treat the vector
table and interrupt symbols as a build-time interface. Add a map-file check or test
that confirms every safety-critical interrupt resolves to an intended implementation.

## Exception Entry And Return

On exception entry, the core saves an architectural stack frame containing registers
such as `r0-r3`, `r12`, `lr`, `pc`, and `xPSR`; floating-point state may add an extended
frame. The processor can use the main stack pointer (MSP) or process stack pointer
(PSP), and the exception return value in `LR` describes which stack and mode to resume.
The exact frame and lazy-FPU behavior matter when an RTOS switches context or a fault
handler decodes a stack frame.

An exception handler should preserve the ABI and avoid treating an arbitrary `LR` as a
normal return address. A common diagnostic pattern is to use a small assembly wrapper
to select the active stack and pass its address to a C function. The wrapper is
architecture-specific; keep it in one reviewed file and keep the actual decoding logic
in ordinary C.

The stacked program counter is evidence, not automatically the exact faulting
instruction. For some faults it points to the instruction after a completed operation;
for a precise or imprecise bus fault the relationship differs. Inspect fault status,
the instruction bytes, and the core documentation together.

## NVIC Priorities And Interrupt Design

The Nested Vectored Interrupt Controller provides exception enable, pending, active,
priority, and software-trigger operations. Priority fields are often implemented with
fewer bits than the register width, and the priority grouping can split preemption and
subpriority fields. A numerically smaller priority commonly means higher urgency, but
verify the target and RTOS convention before relying on it.

Important design questions include:

- Can this interrupt preempt the code it calls?
- Which priorities are allowed to call RTOS APIs?
- Does acknowledging the peripheral clear the source, and can it race with a new event?
- Is a pending interrupt level- or edge-sensitive?
- Can the handler run during clock, power, or peripheral reconfiguration?
- What is the maximum time with interrupts masked?

Keep the top-half deterministic: capture status, move or mark data, clear the source,
and signal deferred work. If a flag can be set by both an ISR and task, use an atomic
or interrupt-safe protocol; a plain read-modify-write can lose an event.

## System Control Block And Faults

The System Control Block exposes core identification, vector relocation, reset
control, fault configuration, and fault-status registers. Enable the configurable fault
handlers early in development so a memory-management, bus, or usage error does not
collapse into an opaque HardFault.

Capture at least:

- the active stack pointer and the stacked `r0-r3`, `r12`, `LR`, `PC`, and `xPSR`;
- Configurable Fault Status Register (CFSR) subfields;
- HardFault Status Register (HFSR);
- fault address registers when their validity bits are set;
- active interrupt number, reset reason, and build identifier.

The C handler should be safe when the heap, scheduler, clocks, or semihosting are not
available. Store a compact record in retained RAM or a crash transport, guard against
recursive faults, and reset or halt according to the product policy.

## Stack Selection And RTOS Contexts

Bare-metal code often uses MSP for thread mode. An RTOS commonly uses PSP for tasks and
MSP for handlers. This gives a separate interrupt stack and makes task stack bounds
measurable. The RTOS context switch must preserve the registers required by the ABI
and the architectural lazy-FPU state if floating-point code is allowed in tasks.

Do not call a function from an ISR merely because its C prototype is available. Check
whether it can block, allocate, take a non-ISR lock, access a task-only peripheral
context, or depend on thread-local state. The interrupt priority ceiling and RTOS API
rules are part of the platform contract.

## Memory Ordering And Barriers

The `__DMB`, `__DSB`, and `__ISB` operations exposed by CMSIS correspond to different
ordering and pipeline effects. In broad terms:

- **DMB** orders explicit memory accesses around a synchronization boundary.
- **DSB** waits for relevant memory effects to complete before continuing.
- **ISB** flushes the instruction pipeline so subsequent execution observes a changed
  execution context, such as control-register or memory-attribute changes.

Use the barrier required by the architectural operation and the peripheral protocol,
not a blanket barrier after every volatile access. C11 atomics express inter-thread
ordering for C objects; they do not automatically perform the cache maintenance or
device-specific sequence required by every DMA or MMIO block.

## MPU And TrustZone-M

The optional MPU can enforce read/write/execute permissions and memory attributes for
regions. A robust configuration defines a deliberate default map, marks stacks and
code appropriately, and gives DMA buffers the attributes required by the bus fabric.
Reprogramming regions is itself a critical section and must not leave a task or ISR
running with a partially updated policy.

TrustZone-M partitions the system into Secure and Non-secure states. A Non-secure call
into Secure code crosses a controlled gateway and must validate pointers, lengths,
handles, and timing expectations. Do not pass a raw Non-secure pointer into Secure
code and assume it remains valid; validate its range and permissions, and consider
concurrent modification during the call.

## CMSIS As A Portability Boundary

CMSIS-Core standardizes names and access patterns for core registers, NVIC operations,
intrinsics, compiler controls, and device startup conventions. It is useful as a
shared vocabulary, but it does not erase device differences. The device header,
startup file, linker script, clock tree, interrupt names, and errata remain vendor- and
part-specific.

Keep application code away from raw CMSIS register operations where possible. A small
board module can expose operations such as `board_timer_init()` or
`board_enter_sleep()`, while a test double models their result and timing policy.

## Debugging A Cortex-M Failure

1. Stop at the fault handler before state is overwritten.
2. Determine which stack was active and decode the correct frame shape, including FPU
   state when relevant.
3. Check validity bits before trusting fault address registers.
4. Disassemble around the stacked `PC` using the exact image and symbols.
5. Check vector-table location, handler address, stack bounds, and EXC_RETURN validity.
6. Compare clock, MPU, privilege, and peripheral state with the reset/startup timeline.
7. Reproduce with optimization and link-time options close to the production image.

## Exercises And Examples

1. Implement a crash record structure and a host decoder for a captured Cortex-M stack
   frame. Test basic and extended-FPU frame layouts with synthetic records.
2. Create a timer ISR that records a timestamp and signals a worker. Measure worst-case
   latency while another interrupt and a DMA transfer are active.
3. Relocate a vector table in a test image, trigger a software interrupt, and verify
   that the expected table and handler are used.
4. Configure an MPU region for a task stack and provoke an out-of-bounds access in a
   controlled test; document the observed fault registers.
5. Compare Secure and Non-secure API wrappers and list every pointer, length, and
   handle validation at the boundary.

## Common Mistakes

- Assuming all Cortex-M profiles have the same instructions, fault types, FPU, or
  TrustZone support.
- Replacing an interrupt handler without verifying the exact vector symbol and weak
  alias behavior.
- Decoding every fault as a precise data abort at the stacked `PC`.
- Calling blocking or task-only APIs from an ISR.
- Changing priority values without checking implemented bits, grouping, and RTOS rules.
- Using `DSB` or `volatile` as a universal replacement for a documented synchronization
  protocol.
- Enabling an FPU or compiling with the wrong floating-point ABI for the image.
- Passing unchecked pointers across a Secure/Non-secure boundary.

## Related Topics

- [Platform-Specific C overview](./index.md)
- [Microcontroller Platforms](./microcontroller-platforms.md)
- [C Memory Model And Concurrency](../advanced-c/c-memory-model-and-concurrency.md)
- [Embedded C And Hardware](../embedded-c-and-hardware/index.md)
- [Correctness, Quality, And Security](../correctness-quality-and-security/index.md)

## References

- [Arm Cortex-M processors](https://developer.arm.com/Processors/Cortex-M)
- [CMSIS-Core NVIC documentation](https://arm-software.github.io/CMSIS_5/Core/html/group__NVIC__gr.html)
- [CMSIS startup code documentation](https://arm-software.github.io/CMSIS_5/5.8.0/Core/html/startup_c_pg.html)
- [CMSIS compiler control documentation](https://arm-software.github.io/CMSIS_5/develop/Core/html/group__compiler__conntrol__gr.html)
- The exact core programming manual, MCU reference manual, errata, and RTOS port guide

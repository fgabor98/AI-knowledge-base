---
status: draft
reviewed: false
domain: c
difficulty: advanced
last_reviewed: null
---

# ARM Cortex-A And AArch64

Cortex-A processors are application processors: they are designed for virtual memory,
rich operating systems, multiple privilege levels, caches, and often multiple cores.
AArch64 is the 64-bit execution state of Armv8-A and later architectures. C remains
the main implementation language for firmware, kernels, drivers, runtimes, and
applications, but its behavior is shaped by the AArch64 ABI, page tables, cache
attributes, exception levels, and the boot software that establishes them.

## Learning Objectives

- Read the AArch64 programmer model and explain how C calls map onto registers and the
  stack.
- Distinguish EL0 userspace, EL1 operating-system code, and higher secure/virtualization
  levels.
- Understand MMU translation, cacheability, memory attributes, and device mappings.
- Connect C drivers to GIC interrupts, Device Tree, PSCI, and firmware interfaces.
- Diagnose faults using exception syndrome, fault address, page tables, and disassembly.

## AArch64 Programmer Model And ABI

AArch64 provides general-purpose registers `X0` through `X30`, a stack pointer, a
program counter, and processor state. The lower 32-bit view of an `X` register is a `W`
register. The architecture also has SIMD/FP registers and system registers accessible
only under the appropriate privilege rules.

The AArch64 Procedure Call Standard normally uses `X0-X7` for integer/pointer
arguments and return values, with additional arguments passed on the stack. `X19-X28`
are callee-saved; `X29` is conventionally the frame pointer and `X30` the link
register. The stack pointer must obey the ABI's alignment rule at public interfaces.
Floating-point and vector arguments use the SIMD/FP register convention when the
function prototype and ABI permit it.

These are ABI rules, not C syntax. Violating them in inline assembly, a hand-written
context switch, an exception wrapper, or an FFI boundary can corrupt a caller without
an obvious source-level error. Use compiler-generated prologues where possible and
describe every clobbered register and memory effect when assembly is unavoidable.

## Exception Levels And Security States

The architecture defines exception levels:

- **EL0:** ordinary applications or other least-privileged code.
- **EL1:** operating-system kernel or a bare-metal supervisor.
- **EL2:** hypervisor/virtualization control, when enabled.
- **EL3:** secure monitor and firmware transition code, when implemented.

Arm TrustZone divides the system into Secure and Non-secure states. A platform may run
secure firmware at EL3, a secure OS, a non-secure hypervisor, or another arrangement.
The exact boot path is platform policy. C code crossing an exception level or security
boundary should use a specified firmware ABI (for example, PSCI or an SMC service),
not reach into another level's private data structures.

An exception vector selects an entry based on the current level, stack, and exception
class. The handler must preserve the ABI or switch deliberately to a context-save
format. For synchronous exceptions, inspect the exception syndrome register and fault
address before returning or terminating the context.

## MMU And Address Spaces

In a normal Linux application, a C pointer is a virtual address in the process's EL0
address space. The MMU translates it through translation tables to a physical address,
subject to access permissions and memory attributes. Kernel and firmware code may use
different translation regimes and may temporarily disable or replace them.

Each mapping needs an intentional choice of:

- read/write/execute permission and privilege access;
- normal memory versus device memory;
- shareability and cacheability;
- granule/page size and alignment;
- access-fault behavior and lifetime;
- visibility to DMA and other masters.

Never map a device register block as ordinary cacheable memory. Never assume a physical
address can be cast and dereferenced from userspace. A driver must arrange a mapping
through the OS, validate offsets and lengths, and use the accessors and barriers
specified for that device.

## Caches, DMA, And Device Memory

A cache-coherent CPU can still have a coherency problem when a peripheral, another
coherency domain, or a non-coherent interconnect participates in the transfer. The
driver contract must state whether buffers are:

- coherent and safe to share directly;
- non-coherent and require clean/invalidate operations;
- streaming (ownership transferred for one operation);
- permanently mapped with a restricted memory attribute.

Cache maintenance is about visibility and ownership, not just ordering. A barrier can
order CPU operations while stale cache lines remain invisible to DMA. Conversely,
cleaning a buffer without a protocol can expose partially written data. Use the OS DMA
API in kernel code and the platform's documented cache operations in firmware.

## GIC And Interrupt Delivery

The Generic Interrupt Controller (GIC) separates interrupt routing and prioritization
from the peripheral source. Interrupts can be private to a core, shared, or routed to
a selected affinity. A driver must coordinate source acknowledgement, GIC end-of-
interrupt handling, affinity, and device-specific masking.

For an SMP system, decide whether a handler may run on any CPU and whether its data
structures are per-CPU or shared. Per-CPU state avoids locks for fast paths but requires
explicit CPU selection during initialization and teardown. Shared interrupt lines need
careful status checking so one device does not clear or starve another.

## Boot And Firmware Interfaces

A typical embedded A-class boot sequence may include ROM code, a first-stage loader,
secure firmware, a bootloader, a hypervisor, a kernel, and userspace. Each stage can
change exception level, MMU state, cache state, vector base, CPU release state, and
memory ownership.

Important standardized or commonly specified interfaces include:

- **PSCI:** power, reset, and CPU hotplug operations for an operating system.
- **SMC/HVC calls:** transitions to secure firmware or a hypervisor according to a
  defined calling convention.
- **Device Tree:** hardware description passed to firmware, bootloader, and kernel.
- **ACPI or platform tables:** alternative hardware discovery on some systems.

Write down the hand-off contract: register arguments, memory locations, cache state,
address width, exception level, and who owns each buffer. “It works after the
bootloader” is not a sufficient contract.

## Device Tree And C Drivers

Device Tree describes hardware as nodes and properties. A Linux driver matches a
compatible string, then obtains resources such as register ranges, interrupts, clocks,
resets, regulators, and DMA constraints through kernel APIs. The driver should not
hard-code a board address just because the address is stable on one product.

The Device Tree binding is an interface. Document required properties, units, allowed
values, reset behavior, and compatibility guarantees. A driver should fail clearly when
required resources are absent or inconsistent, rather than silently using a default
that may be unsafe.

## SMP And Memory Ordering

AArch64 provides weakly ordered memory. The compiler's C memory model and the
architecture's memory model are separate layers. Use C11 atomics for communication
between C threads when the implementation supports them, and use kernel/firmware
primitives for device or cross-domain synchronization. A `volatile` object is not a
portable lock or inter-core publication mechanism.

For a publication protocol, the producer must finish writing the payload before making
the ready state visible, and the consumer must observe the state before reading the
payload. The corresponding acquire/release operations are clearer than sprinkling
architecture barriers into application code. For MMIO or page-table changes, use the
architecture/OS-required barrier and instruction synchronization sequence.

## Debugging AArch64 Failures

Collect the exception level, vector, exception syndrome, fault address, link/return
state, general registers, and relevant page-table or mapping information. Then:

1. Determine whether the fault is an instruction abort, data abort, alignment fault,
   permission fault, translation fault, or undefined instruction.
2. Translate the faulting virtual address in the crashing context, not in a different
   process or after teardown.
3. Check memory attributes and whether the access was normal memory, device memory, or
   an unmapped address.
4. Inspect the exact instruction and register values in the matching ELF image.
5. Check whether a DMA engine or another core modified the memory concurrently.
6. Compare the failure with boot stage, CPU affinity, power state, and firmware calls.

## Exercises And Examples

1. Compile a small C API for AArch64 and annotate every argument, return value, saved
   register, stack adjustment, and frame-pointer choice in its disassembly.
2. Build a minimal Device Tree binding and list the validation a driver must perform
   before mapping its registers.
3. Create a cache/DMA ownership table for a video or network buffer and state exactly
   when data becomes CPU-owned or device-owned.
4. Decode a synthetic data-abort record: classify the syndrome, identify the access
   width and direction, and determine which translation-table evidence is needed.
5. Compare a bare-metal EL1 image with a Linux userspace process and enumerate which
   operations must become syscalls or driver APIs.

## Common Mistakes

- Assuming a C pointer is a physical address or that a physical address can be mapped
  safely by casting it.
- Treating caches as an implementation detail when DMA or another core observes memory.
- Mixing AArch64 objects with incompatible ABI, endianness, or floating-point settings.
- Entering an exception or SMC handler without a deliberate register and stack contract.
- Hard-coding board resources instead of using the platform's discovery interface.
- Using a C volatile flag as an SMP synchronization primitive without atomic ordering.
- Mapping device registers as cacheable normal memory.

## Related Topics

- [Platform-Specific C overview](./index.md)
- [Embedded Linux](./embedded-linux.md)
- [Multicore And Heterogeneous Systems](./multicore-and-heterogeneous-systems.md)
- [Compilation, Linking, And ABI](../compilation-linking-and-abi/index.md)
- [C Memory Model And Concurrency](../advanced-c/c-memory-model-and-concurrency.md)

## References

- [Arm Cortex-A processors](https://developer.arm.com/Processors/Cortex-A)
- [CMSIS-Core(A) documentation](https://arm-software.github.io/CMSIS_5/latest/Core_A/html/index.html)
- [Arm Application Binary Interface documentation](https://github.com/ARM-software/abi-aa)
- [Linux Device Tree usage model](https://docs.kernel.org/6.3/devicetree/usage-model.html)
- [Linux Device Tree documentation](https://docs.kernel.org/next/devicetree/index.html)
- The exact Arm Architecture Reference Manual, core TRM, SoC manual, boot protocol, and
  GIC documentation

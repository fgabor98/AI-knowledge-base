---
status: draft
reviewed: false
domain: c
difficulty: advanced
last_reviewed: null
---

# RISC-V

RISC-V is an open ISA built from a base integer instruction set plus optional standard
and non-standard extensions. That modularity is powerful for embedded work, but it
means that “RISC-V” does not identify one complete processor or one memory map. The
compiler target, ISA string, ABI, privilege implementation, interrupt controller,
debug module, and SoC documentation must all be known.

## Learning Objectives

- Read a RISC-V ISA string and relate it to generated C code and available operations.
- Explain the integer register convention and RISC-V ELF psABI responsibilities.
- Use privilege levels, CSRs, traps, PMP, and firmware interfaces safely.
- Understand `FENCE`, atomics, compressed instructions, and toolchain relaxation.
- Port low-level C across RISC-V microcontrollers and application processors without
  assuming non-standard hardware behavior.

## ISA Composition And Target Selection

A target is described by a base such as `RV32I` or `RV64I` plus extensions. Common
extensions include:

- `M`: integer multiply/divide;
- `A`: atomic memory operations;
- `F` and `D`: single- and double-precision floating point;
- `C`: compressed 16-bit encodings;
- `B`: bit-manipulation facilities in implementations that support the ratified form;
- vector extensions and other optional standard extensions;
- vendor or experimental extensions identified by the toolchain and platform.

An ISA string such as `rv32imac` is a build contract. `-march` controls instruction
availability; `-mabi` controls data representation and calling convention. A program
that compiles with `-march=rv64gc` may fail on a core without the floating-point or
atomic extensions, or may trap if the OS has not enabled the expected execution state.
Keep target flags in one build definition and inspect the ELF attributes in release
artifacts.

The compressed extension changes code size and instruction alignment but does not
change the C abstract machine. It can, however, affect branch ranges, linker
relaxation, instruction-level debugging, and assumptions in hand-written patching or
boot code.

## Integer Register And Calling Convention

The standard integer register names are:

| Registers | Conventional role |
| --- | --- |
| `x0` (`zero`) | Constant zero |
| `x1` (`ra`) | Return address |
| `x2` (`sp`) | Stack pointer |
| `x3` (`gp`) | Global pointer |
| `x4` (`tp`) | Thread pointer |
| `x5-x7`, `x28-x31` | Temporaries |
| `x8-x9`, `x18-x27` | Callee-saved (`s0/fp` through `s11`) |
| `x10-x17` | Argument/return registers (`a0-a7`) |

The psABI normally passes integer and pointer arguments in `a0-a7`, returns scalar
results in `a0` and `a1`, and requires callees to preserve the `s` registers, `sp`, and
the relevant platform conventions for `gp` and `tp`. Variants cover RV32/RV64,
embedded register sets, floating-point ABIs, and vector calling conventions.

An interrupt, trap, context switch, or inline-assembly wrapper must preserve the
registers required by the context contract. A trap handler cannot blindly use the
ordinary function ABI until it has saved the interrupted state and established a valid
stack.

## Privilege Levels And CSRs

RISC-V systems commonly implement Machine mode (M-mode), Supervisor mode (S-mode),
and User mode (U-mode), though a small MCU may implement only a subset. Hypervisor
extensions add virtualization-related state. Control and Status Registers (CSRs)
configure status, trap vectors, interrupt enables/pending state, address translation,
delegation, counters, and implementation features.

CSR numbers and writable fields are architectural or platform-specific. Accessing an
unsupported or read-only CSR can trap. Keep CSR access in small, target-guarded
functions and define what happens when a feature is absent. Do not use a compiler
macro alone as proof that the underlying CSR exists on a particular chip.

For a trap, record the cause, the faulting program counter, the trap value, privilege
state, and relevant status/delegation registers. The meaning of `mtval`/`stval` is
cause-dependent; it may contain a fault address or instruction bits, and may be zero.

## Interrupt Architecture

RISC-V defines interrupt causes and delegation mechanisms, but the external interrupt
controller is often platform-specific. A small MCU may have a simple local controller;
an application processor may use a PLIC or a newer interrupt architecture. Timer and
software interrupts may be provided by CLINT-like blocks, ACLINT-style components, or
vendor logic.

The handler must coordinate three levels:

1. The RISC-V interrupt enable/pending state and trap vector.
2. The platform interrupt controller's claim/complete or acknowledge protocol.
3. The peripheral's source status and clear/mask behavior.

Enabling an external interrupt at only one level is not enough. Conversely, clearing a
source before capturing its status can lose information. Specify interrupt affinity,
nesting, priority, and delegation separately for each privilege/runtime environment.

## CSR Access In A Portability Wrapper

Target-specific assembly should be isolated behind a C interface. This example has a
host-safe fallback and uses the architectural `rdcycle` instruction only when compiling
for RISC-V:

```c
#include <stdint.h>

static inline uintptr_t platform_cycle_counter(void)
{
#if defined(__riscv)
    uintptr_t value;
    __asm__ volatile ("rdcycle %0" : "=r"(value));
    return value;
#else
    return 0u;
#endif
}
```

The wrapper does not promise that the counter is available, stable across privilege
levels, synchronized across cores, or suitable for a wall clock. The platform layer
must document those properties and provide a frequency conversion or a different time
source when required.

## Memory Ordering And Atomics

RISC-V has an explicit memory-ordering model. `FENCE` orders selected classes of
memory operations; the `A` extension supplies atomic read-modify-write and load/store
operations. C11 atomics map to suitable instructions or library routines when the
compiler and runtime support them. A target without native atomics may require a lock
or may not support a particular lock-free width.

Use release publication and acquire consumption for shared data structures. Use the
stronger architecture or OS primitive required for MMIO, interrupt-controller state,
DMA ownership, or a device-specific protocol. A C atomic operation on ordinary memory
does not automatically make a peripheral register transaction valid.

## PMP And Protection

The Physical Memory Protection (PMP) mechanism, when implemented, restricts access by
privilege mode to physical address ranges. Region encoding, priority, lock behavior,
and execute/read/write permissions need careful setup before lower-privilege code runs.
PMP is not an MMU and does not by itself provide virtual address spaces or process
isolation. A firmware design should define a default-deny or least-privilege policy,
protect executable and stack regions, and include access for interrupt and DMA paths.

## SBI, Boot, And Runtime Boundaries

On systems with an operating system, Supervisor Binary Interface (SBI) services can
provide timer, inter-processor interrupt, reset, and hart-management operations below
S-mode. On a bare-metal MCU, firmware may instead own the machine-mode trap vector and
peripherals directly. Do not call an SBI service or assume a firmware-owned CSR unless
the boot contract says the service is present.

Boot code must establish the stack, trap vector, delegation, memory protection,
per-hart state, clocks, and device access before handing control to the next runtime.
For multicore startup, record which hart is the boot hart, how secondary harts are
released, and how their stacks and interrupt state are initialized.

## Toolchain And ABI Verification

For every image, record:

- `-march` and `-mabi` values and the compiler target triple;
- whether linker relaxation and compressed instruction generation are enabled;
- code model and position-independent-code settings;
- floating-point and vector ABI choices;
- startup objects, linker script, libraries, and firmware calls;
- the exact core/SoC extensions and errata workarounds.

Inspect disassembly for unexpected emulation calls, floating-point instructions,
mis-sized atomic operations, or relaxation assumptions. A function that looks portable
in C can introduce a library call whose implementation needs an extension or a runtime
service absent from a freestanding image.

## Exercises And Examples

1. Given `rv32imac` and `rv64gc` targets, compare the size and calling convention of a
   structure-returning function and document the ABI differences.
2. Write a trap-record decoder that classifies cause, privilege, and `tval` without
   assuming every trap value is an address.
3. Implement a release/acquire single-producer ring buffer, then inspect whether the
   target emits native atomics or calls a helper.
4. Map every interrupt source through CSR enable, external controller, and peripheral
   status; test each failure mode independently.
5. Build the same driver with and without `C` and compare code size, alignment, and
   debug stepping behavior.

## Common Mistakes

- Treating “RISC-V” as a complete, uniform hardware platform.
- Enabling compiler extensions without confirming the silicon and firmware support them.
- Assuming the presence of an ISA extension implies a particular interrupt controller,
  CSR, memory map, or privilege implementation.
- Saving only caller-saved registers in a trap path that must resume arbitrary code.
- Using `FENCE` or a C atomic without defining whether the memory is ordinary, MMIO, or
  DMA-visible.
- Ignoring linker relaxation, code model, or `gp`/`tp` conventions in hand-written code.
- Calling an SBI service from a bare-metal image with no SBI implementation.

## Related Topics

- [Platform-Specific C overview](./index.md)
- [Microcontroller Platforms](./microcontroller-platforms.md)
- [Multicore And Heterogeneous Systems](./multicore-and-heterogeneous-systems.md)
- [Compilation, Linking, And ABI](../compilation-linking-and-abi/index.md)
- [C Memory Model And Concurrency](../advanced-c/c-memory-model-and-concurrency.md)

## References

- [RISC-V ISA manual](https://github.com/riscv/riscv-isa-manual)
- [RISC-V ELF psABI specification](https://github.com/riscv-non-isa/riscv-elf-psabi-doc)
- [RISC-V calling convention](https://github.com/riscv-non-isa/riscv-elf-psabi-doc/blob/master/riscv-cc.adoc)
- [RISC-V ELF specification](https://github.com/riscv-non-isa/riscv-elf-psabi-doc/blob/master/riscv-elf.adoc)
- The exact core manual, privileged architecture version, interrupt-controller spec,
  SoC manual, boot contract, and vendor extension documentation

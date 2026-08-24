---
status: draft
reviewed: false
domain: c
difficulty: advanced
last_reviewed: null
---

# Platform-Specific C

Portable C is a language specification. A production C program also lives inside an
instruction-set architecture (ISA), an ABI, a startup/runtime contract, an operating
system, a linker script, and a board-level memory map. Platform-specific C is the
discipline of making those extra contracts explicit instead of accidentally depending
on them.

This chapter is aimed at embedded engineers who need to move confidently between a
small freestanding microcontroller image, an RTOS application, a 64-bit application
processor, and embedded Linux. The goal is not to memorize every register. It is to
learn which questions to ask, where a guarantee comes from, and how to isolate the
platform-dependent part of a design.

## Learning Objectives

By the end of this chapter, you should be able to:

- Separate ISO C guarantees from compiler, ABI, operating-system, silicon, and board
  guarantees.
- Read a target's ABI and use it to explain generated calls, stack frames, argument
  passing, object representation, and binary compatibility.
- Understand the boot-to-`main` path on both microcontrollers and application
  processors.
- Map C objects and accesses onto flash, SRAM, TCM, MMIO, DMA buffers, caches, and
  protected regions without confusing addressability with safe access.
- Design interrupt, exception, multicore, and device-driver boundaries that respect the
  target's execution and memory-ordering rules.
- Make compiler extensions, intrinsics, inline assembly, and vendor SDK types visible,
  reviewable, and replaceable.
- Debug a platform failure using disassembly, map files, fault state, trace, and
  hardware documentation rather than guessing from source code alone.

## The Contract Stack

When a behavior is in doubt, identify the layer that defines it:

| Layer | Questions it answers | Typical evidence |
| --- | --- | --- |
| ISO C | What does the language and standard library guarantee? | C standard, library specification, compiler diagnostics |
| Implementation | What are the sizes, representations, extensions, and startup assumptions? | Compiler manual, predefined macros, `sizeof`, ABI options |
| ABI | How are functions, objects, exceptions, registers, and binaries represented? | ABI document, object-file inspection, calling-convention tests |
| ISA and microarchitecture | Which instructions, privilege operations, atomics, caches, and faults exist? | ISA manual, core reference manual, errata |
| Operating system or RTOS | What are the syscall, scheduling, interrupt, driver, and protection rules? | Kernel/RTOS API documentation, configuration, source |
| Board and SoC | Where are devices, memory, clocks, resets, and security boundaries? | Datasheet, reference manual, schematic, Device Tree |
| Build and release system | Which exact compiler, flags, linker script, libraries, and image steps are used? | Build logs, map file, linker script, reproducible build metadata |

The lower layers do not override undefined behavior in ISO C. For example, a CPU may
perform an unaligned load successfully, but dereferencing a misaligned pointer can
still violate the C implementation's alignment requirements. Conversely, `volatile`
can describe an observable MMIO access to the compiler, but it does not automatically
provide atomicity, cache maintenance, or a device-protocol transaction.

## How to Study This Chapter

Work from a concrete target whenever possible. Record the following before writing
low-level code:

1. ISA, core revision, supported optional extensions, endianness, and word size.
2. ABI, data model (`ILP32`, `LP64`, and so on), floating-point ABI, and calling
   convention.
3. Privilege levels, exception entry rules, interrupt controller, and debug access.
4. Memory map, reset state, cacheability, DMA visibility, protection regions, and boot
   stages.
5. Compiler/binutils versions, target flags, C dialect, standard library, linker
   script, startup objects, and post-link image tools.
6. Concurrency model: interrupt preemption, RTOS tasks, SMP, AMP, DMA, and external
   bus masters.

Then verify each assumption with a tiny experiment. Useful experiments include a
calling-convention probe, an object-layout assertion, a reset-to-`main` trace, a
fault-handler dump, a cache/DMA coherency test, and a disassembly comparison at the
optimization levels used in production.

## Chapter Map

### Foundations and microcontrollers

- [Microcontroller Platforms](./microcontroller-platforms.md) compares MCU resource
  models, memory regions, startup code, HALs, and vendor differences.
- [ARM Cortex-M](./arm-cortex-m.md) covers the exception-centric profile used by many
  real-time systems.

### Application processors and operating systems

- [ARM Cortex-A And AArch64](./arm-cortex-a-and-aarch64.md) connects AArch64 C to
  privilege levels, MMU, caches, GIC, SMP, and secure boot worlds.
- [Embedded Linux](./embedded-linux.md) explains the user/kernel, syscall, device,
  Device Tree, driver, and deployment boundaries.

### Other architectures and system composition

- [RISC-V](./risc-v.md) covers ISA composition, CSRs, privilege, traps, the psABI,
  and toolchain-selected targets.
- [x86-64](./x86-64.md) uses the familiar desktop/server architecture to teach ABI,
  generated code, SIMD, ordering, and OS boundary differences.
- [Multicore And Heterogeneous Systems](./multicore-and-heterogeneous-systems.md)
  covers shared memory, cache coherence, synchronization, SMP, AMP, and remote
  processors.
- [Compiler And Vendor Extensions](./compiler-and-vendor-extensions.md) shows how to
  use non-ISO facilities without allowing them to leak through every interface.

## A Platform Porting Checklist

Before calling a port complete, review:

- [ ] The exact target triple, architecture flags, ABI, and data model are recorded.
- [ ] Startup initializes the stack, vector/exception entry, data, zeroed data, clocks,
  protection, and runtime prerequisites in the documented order.
- [ ] Linker sections match the memory map, and the map file is checked for overflow,
  unexpected placement, and retained/dead-stripped symbols.
- [ ] MMIO types, access widths, ordering requirements, reset values, and side effects
  are documented at the driver boundary.
- [ ] Interrupt and exception handlers obey the ABI and execution-context restrictions.
- [ ] Cache, DMA, MPU/MMU, and shared-memory assumptions are tested on hardware.
- [ ] Toolchain extensions have feature detection, a fallback or an explicit target
  requirement, and a test that protects the assumption.
- [ ] Host tests exercise policy and algorithms separately from target access code.
- [ ] Debug builds preserve enough symbols and fault state to diagnose field failures.

## Capstone Exercises

1. **Port a register driver.** Keep the protocol logic in portable C and implement
   only the register access, interrupt hookup, and memory placement per target.
2. **Explain a call.** Compile one function for two ABIs, inspect the assembly, and
   annotate argument registers, stack alignment, saved registers, and return values.
3. **Trace startup.** Follow reset through the vector or bootloader, runtime
   initialization, constructors if present, and `main`; record every memory region
   touched before the scheduler starts.
4. **Prove a coherency boundary.** Build a DMA or inter-core ring buffer with explicit
   ownership transitions, cache maintenance where required, and a stress test.
5. **Make an extension portable.** Wrap one compiler attribute or intrinsic behind a
   project macro, provide a safe fallback, and test both paths in CI.

## Related Topics

- [C Programming](../index.md)
- [Compilation, Linking, And ABI](../compilation-linking-and-abi/index.md)
- [Embedded C And Hardware](../embedded-c-and-hardware/index.md)
- [C Memory Model And Concurrency](../advanced-c/c-memory-model-and-concurrency.md)
- [Correctness, Quality, And Security](../correctness-quality-and-security/index.md)
- [Topic Map](../../topic-map.md)

## References

- [Arm CMSIS documentation](https://arm-software.github.io/CMSIS_5/Core/html/index.html)
- [Arm Application Binary Interface documentation](https://github.com/ARM-software/abi-aa)
- [RISC-V ISA manual](https://github.com/riscv/riscv-isa-manual)
- [RISC-V ELF psABI](https://github.com/riscv-non-isa/riscv-elf-psabi-doc)
- [x86-64 System V ABI](https://gitlab.com/x86-psABIs/x86-64-ABI)
- [Linux kernel documentation](https://docs.kernel.org/)
- [GCC developer documentation](https://gcc.gnu.org/onlinedocs/gcc/)
- [Clang language extensions](https://clang.llvm.org/docs/LanguageExtensions.html)

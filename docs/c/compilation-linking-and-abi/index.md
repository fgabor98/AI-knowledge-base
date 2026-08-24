---
status: draft
reviewed: false
domain: c
difficulty: intermediate
last_reviewed: null
---

# Compilation, Linking, And ABI

This chapter explains how C source becomes a hosted executable, shared object, or embedded firmware image. The central idea is that several contracts meet at the build boundary:

1. The C standard describes source-language meaning and translation requirements.
2. The compiler implements a language dialect and emits machine code for a target.
3. The object format records sections, symbols, relocations, and debug information.
4. The linker resolves references and maps the result into an executable image.
5. The ABI defines how separately compiled code agrees on data layout, calls, registers, and binary interfaces.
6. Startup code and the operating system or boot ROM turn the image into a running program.

The same C statement can therefore be valid ISO C while still producing the wrong binary for a target if the selected ABI, linker script, startup code, or library does not match the system.

## What This Chapter Teaches

- the difference between a translation unit, object file, executable, shared object, and firmware image;
- what preprocessing, compilation, assembly, linking, loading, and startup each contribute;
- how compiler modes, warnings, optimization, and target flags change the generated program;
- how ELF sections, segments, symbols, archives, and relocations fit together;
- how static and dynamic linking differ and why bare-metal systems normally use a different model;
- how linker scripts express flash, RAM, section placement, symbols, and image invariants;
- how reset handlers, C runtime initialization, `main`, constructors, and termination are connected;
- how cross-compilers, target triples, sysroots, multilibs, and SDKs prevent host/target contamination;
- how an ABI controls calls, register use, alignment, struct layout, variadic arguments, and FFI;
- how to inspect the produced binary instead of trusting a successful build.

## The Source-To-System Model

```text
source files + headers + macros
              |
              v
       preprocessed translation unit
              |
              v
     compiler: parse, analyze, optimize
              |
              v
       assembly / machine code
              |
              v
       relocatable object (.o)
              |
     archives, runtime objects, libraries
              v
     linker: symbols + relocations + layout
              |
              v
 executable / shared object / firmware ELF
              |
       loader or boot image conversion
              v
   startup code -> C runtime -> application
```

The compiler driver often hides several of these stages. A command such as `cc app.c -o app` may invoke the preprocessor, compiler, assembler, linker, startup objects, and standard libraries. Senior C work requires knowing when that default orchestration is correct and how to expose it when diagnosing a failure.

## Recommended Progression

1. [Translation Pipeline](./translation-pipeline.md) — observe each stage and its artifacts.
2. [Compiler Modes, Warnings, And Optimization](./compiler-modes-warnings-and-optimization.md) — make the compiler contract explicit.
3. [Object Files, Symbols, And Relocations](./object-files-symbols-and-relocations.md) — read what separate compilation records.
4. [Static And Dynamic Linking](./static-and-dynamic-linking.md) — understand library selection and runtime loading.
5. [Linker Scripts And Memory Layout](./linker-scripts-and-memory-layout.md) — place code and data in a real embedded memory map.
6. [Startup, Runtime, And `main`](./startup-runtime-and-main.md) — follow reset or process creation into C.
7. [Cross-Compilation And Sysroots](./cross-compilation-and-sysroots.md) — build for a target without mixing host artifacts.
8. [ABI, Calling Conventions, And FFI](./abi-calling-conventions-and-ffi.md) — make binary interfaces deliberate.
9. [Debug Information And Binary Inspection](./debug-information-and-binary-inspection.md) — verify the final image and resolve failures.

## ISO C Versus Toolchain Contracts

Keep these questions separate:

| Question | Contract to inspect |
| --- | --- |
| Is the source construct valid and what does it mean? | Selected ISO C edition and implementation-defined choices |
| Which extensions and built-ins are accepted? | Compiler dialect and target documentation |
| How are objects represented in a file? | Object format, usually ELF on embedded GNU systems |
| How does one function call another? | Platform ABI and procedure call standard |
| Where does `.text` or `.data` live? | Linker script, memory map, and image format |
| Which library supplies `memcpy` or `__aeabi_*`? | libc, compiler runtime, and link command |
| Who initializes RAM and clocks? | ROM, bootloader, reset handler, C runtime, or OS |
| How is a crash address converted to source? | Debug format, symbols, load address, and exact build |

Do not describe all of these as “the compiler.” The compiler may emit an unresolved relocation; the linker may choose a library member; the loader may apply a dynamic relocation; and startup code may copy data before C code runs.

## Build Variants To Understand

Maintain a small matrix rather than one magical build:

| Variant | Typical purpose | Important evidence |
| --- | --- | --- |
| Hosted debug | Fast local diagnosis | warnings, `-g`, sanitizers, symbols |
| Hosted optimized | Production behavior on a host | optimization, hardening, reproducibility |
| Freestanding debug | Bare-metal bring-up | map, sections, startup trace, semihosting policy |
| Freestanding release | Product image | flash/RAM budget, stack, timing, reset behavior |
| Cross-host test | Run target-independent modules on a host | boundary adapters, ABI isolation |
| Size/timing experiment | Validate a suspected cost | disassembly, map, cycle measurement |

Every artifact should retain the compiler version, target flags, linker script, libc configuration, source revision, and build configuration. A binary without this provenance is difficult to debug and impossible to compare reliably.

## Evidence-Driven Workflow

For a build or runtime failure, collect evidence in this order:

1. Print the complete compiler and linker commands.
2. Confirm the compiler target, language mode, sysroot, and selected libraries.
3. Keep preprocessed output for an include or macro problem.
4. Inspect the first failing object file and its undefined symbols.
5. Inspect the linker map, section table, program headers, and entry point.
6. Resolve the runtime address against the exact unstripped image and load address.
7. Compare the generated assembly when behavior differs under optimization.
8. Record the smallest change that changes the artifact.

Successful linking is not proof of a correct firmware image. A section can fit the linker layout and still be inaccessible at runtime; a symbol can resolve to a compatible name with an incompatible calling convention; and a release image can be too large, too slow, or missing debug provenance.

## Running Project

Build a small three-module program and produce both a hosted executable and a bare-metal-style ELF:

1. Put the public API in a header and implement it in a separate translation unit.
2. Compile with explicit C mode, warnings, target options, and debug information.
3. Keep an object-only build so symbols and relocations can be inspected.
4. Put a constant table in a named section and retain it through section garbage collection.
5. Add a custom linker script with flash and RAM regions and an `ASSERT` for capacity.
6. Inspect the map, ELF headers, section sizes, symbols, relocations, and disassembly.
7. Write a reset path that initializes data and bss before calling application code.
8. Expose one C ABI function to a second language or a separately compiled client.
9. Resolve an intentionally recorded crash address using the exact build artifacts.

## Chapter Outcomes

After completing this chapter, you should be able to:

- explain every major artifact between `.c` and a running program;
- reproduce a compiler-driver command as explicit preprocessing, compilation, assembly, and link steps;
- distinguish source errors, compile errors, assembler errors, link errors, loader errors, and startup faults;
- inspect symbols and relocations to explain an undefined reference or wrong implementation;
- design a linker layout with verifiable memory, alignment, retention, and overflow rules;
- integrate startup code and libc without accidentally using uninitialized C runtime state;
- configure a cross-build whose headers, libraries, ABI, and runtime all belong to the target;
- define a stable C-facing ABI for plugins, bootloader interfaces, and foreign languages;
- use maps, DWARF, disassembly, and build provenance to debug optimized production images.

## Related Topics

- [Semantics And Memory](../semantics-and-memory/index.md)
- [Modular Design And APIs](../modular-design-and-apis/index.md)
- [Standard Library And Ecosystem](../standard-library-and-ecosystem/index.md)
- [Correctness, Quality, And Security](../correctness-quality-and-security/index.md)
- [Embedded C And Hardware](../embedded-c-and-hardware/index.md)
- [Platform-Specific C](../platform-specific-c/index.md)
- [C Programming](../index.md)
- [Topic Map](../../topic-map.md)

## References

- [GCC C dialect options](https://gcc.gnu.org/onlinedocs/gcc/C-Dialect-Options.html)
- [GCC optimization options](https://gcc.gnu.org/onlinedocs/gcc/Optimize-Options.html)
- [GCC link options](https://gcc.gnu.org/onlinedocs/gcc/Link-Options.html)
- [GNU ld overview](https://sourceware.org/binutils/docs/ld/Overview.html)
- [GNU ld linker scripts](https://sourceware.org/binutils/docs/ld/Scripts.html)
- [GNU Binary Utilities documentation](https://sourceware.org/binutils/docs/binutils.html)
- [System V ABI and ELF specification](https://refspecs.linuxfoundation.org/elf/)
- [Arm Application Binary Interface repository](https://github.com/ARM-software/abi-aa)

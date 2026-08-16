---
status: draft
reviewed: false
domain: c
difficulty: advanced
last_reviewed: null
---

# Debug Information And Binary Inspection

The final ELF, map file, debug records, disassembly, and image manifest are the ground truth for what was built. Source code and build logs are necessary context, but they cannot answer where a function landed, which library member was selected, whether a vector was retained, or which instruction faulted.

## Learning Objectives

- generate and preserve useful debug information for optimized C;
- inspect ELF headers, sections, segments, symbols, relocations, and sizes;
- resolve a program counter to a source location with the exact image;
- use GDB, `addr2line`, `objdump`, `readelf`, `nm`, and map files together;
- separate debug artifacts from deployable images without losing provenance;
- diagnose embedded faults, stripped production images, and address relocation correctly.

## Debug Formats And Compilation

GCC-family compilers commonly emit DWARF debug information with `-g`. The debug records describe source files, line tables, types, variables, scopes, inline call sites, and machine locations. They usually live in `.debug_*` sections that are not loaded into the target’s runtime memory.

Useful build choices include:

~~~sh
cc -std=c17 -Og -g3 -fno-omit-frame-pointer -c app.c -o app.o
cc -Og -g3 app.o -o app
~~~

`-g` can be combined with optimization. Optimized code may have variables unavailable at a breakpoint, statements reordered, inlined functions represented as nested call sites, and prologues split across ranges. Use `-Og` for interactive diagnosis when it reproduces the issue, but keep a production-like optimized build for optimization-sensitive failures.

Keep compile and link debug settings consistent enough for the debugger to associate the final image with all objects. Store the unstripped ELF even if the device receives a `.bin` or `.hex`.

## Binary Inspection Toolkit

| Tool | Questions it answers |
| --- | --- |
| `readelf -h` | class, machine, entry point, ABI flags |
| `readelf -S` | sections, addresses, sizes, flags, alignments |
| `readelf -l` | loadable segments, permissions, file-to-memory mapping |
| `readelf -s` | symbol binding, type, visibility, size, section |
| `readelf -r` | relocations still present |
| `readelf -d` | dynamic dependencies and loader metadata |
| `objdump -d` | disassembly and instruction addresses |
| `objdump -dr` | disassembly with relocation annotations |
| `nm` | concise symbol presence and address view |
| `size` | text/data/bss summary |
| `strings` | embedded text and unexpected paths |
| `addr2line` | address to file/line/function using debug information |
| `gdb` | interactive state, registers, memory, symbols, and backtraces |

Commands are GNU-family examples; options and output differ by target.

## Map Files

Ask the linker for a map file where supported:

~~~sh
cc objects.o -Wl,-Map=app.map,--cref -o app.elf
~~~

A useful map review checks:

- entry point and vector address;
- output section start, end, size, and alignment;
- flash load addresses and RAM run addresses;
- largest functions and data objects;
- archive members pulled into the image;
- discarded sections;
- duplicate or unexpected symbols;
- linker-defined boundaries and assertions;
- memory-region utilization and unexplained holes.

Make map review partly automated. A CI script can fail on flash/RAM limits, missing metadata, unexpected dependencies, or a changed entry point, while a human reviews meaningful deltas.

## Resolving Addresses

For a hosted non-PIE or fixed-address firmware image:

~~~sh
addr2line -e app.elf -f -C 0x08001234
arm-none-eabi-addr2line -e firmware.elf -f -C 0x08001234
~~~

For an address inside a shared object or PIE, subtract the object’s runtime load base to obtain the link-time-relative address, or let GDB use the loaded mappings. If the target has an exception PC, first determine whether it points to the faulting instruction, the following instruction, a return address, or an architecture-specific frame value.

The address is meaningful only with:

- the exact ELF that ran;
- the exact load base or relocation slide;
- matching debug information;
- the correct instruction-set state and address masking rules;
- knowledge of whether the crash record was captured before or after stack unwinding.

Never resolve a field crash against the newest build because the source line “looks close.” Preserve a build ID and artifact mapping.

## Separate Debug Files And Stripping

Deployment often strips debug sections while retaining a symbol-bearing artifact offline:

~~~sh
objcopy --only-keep-debug app.elf app.elf.debug
strip --strip-debug --strip-unneeded app.elf
objcopy --add-gnu-debuglink=app.elf.debug app.elf
~~~

The exact strip policy depends on the target and whether symbols are needed for field diagnostics. A separate debug file must match the executable; a name match alone is not enough. Use a build ID, debug link, artifact registry, and checksum policy.

For firmware, keep at least:

- unstripped ELF with DWARF;
- stripped or programmer image;
- linker map;
- compiler/linker command manifest;
- source and toolchain revisions;
- post-link checksum/signature inputs;
- memory and symbol reports.

## GDB Workflow

A generic hosted session:

~~~text
gdb ./app
(gdb) set pagination off
(gdb) break main
(gdb) run
(gdb) info files
(gdb) info registers
(gdb) bt full
(gdb) disassemble /m function_name
(gdb) x/16wx address
~~~

For a remote embedded target, load the exact ELF for symbols, connect through the debug server, reset and halt according to the board policy, then inspect registers, fault status, stack, and memory. Do not assume a debugger reset is equivalent to a power-on reset; peripherals, retention RAM, watchdogs, and boot ROM state can differ.

When a backtrace is corrupt, inspect:

- stack pointer alignment and bounds;
- saved link/return address encoding;
- stack overflow or overwrite evidence;
- frame-pointer and unwind metadata;
- interrupt nesting and exception frames;
- compiler optimization and tail-call transformations;
- whether the loaded ELF matches the target image.

## Disassembly As A Contract Check

Disassembly can answer whether:

- a function was inlined or eliminated;
- a branch was range-expanded;
- a supposedly constant access became a load;
- a volatile register access was emitted;
- a stack frame fits the ABI alignment rule;
- an unexpected floating-point helper or library call was pulled in;
- a security or memory barrier remains in the generated code;
- code was placed in the intended flash/RAM section.

Use it to test a specific hypothesis, not to replace source-level reasoning. A machine instruction sequence is target-specific and may change with compiler version, flags, link order, or profile data.

## Size And Resource Analysis

Use multiple views:

~~~sh
size firmware.elf
arm-none-eabi-size -A firmware.elf
arm-none-eabi-nm -S --size-sort firmware.elf | tail -n 30
arm-none-eabi-objdump -h firmware.elf
readelf -SW firmware.elf
~~~

Track flash file payload, flash execution region, RAM allocation, zero-fill RAM, retained RAM, stack reservation, heap reservation, and external memory independently. A single “binary size” number hides the resource that may actually fail.

## Embedded Fault Evidence

A production fault record should include, where available:

- reset cause and watchdog state;
- fault status registers;
- program counter and link register;
- stack pointer and a bounded stack snapshot;
- active exception/interrupt number;
- image version and build ID;
- task/thread identifier;
- recent event or trace records;
- memory protection or bus fault address;
- whether caches, DMA, and clocks were configured.

The fault handler must use only initialized, reentrant, nonblocking facilities that are safe in that context. Store a compact record and defer formatting or transport until the next healthy boot.

## Reproducibility And Provenance

A debug artifact is useful only if its provenance is trustworthy. Include:

- source revision and dirty-tree state;
- compiler, assembler, linker, libc, and binutils versions;
- target triple and complete options;
- linker script and generated headers;
- LTO/PGO/profile inputs;
- binary conversion, signing, and flashing steps;
- build ID and hashes of deployable and debug files.

Reproducible builds make it possible to compare a changed instruction or section and to prove that a debug file belongs to the deployed image.

## Exercises

1. Build an optimized program with DWARF, strip it, and resolve an address using a separate debug file.
2. Generate a map and write a report of the ten largest code and data symbols.
3. Introduce a known fault, capture its PC and stack, and resolve it offline.
4. Compare disassembly with and without inlining, LTO, frame pointers, and section garbage collection.
5. Create a PIE/shared-object address-resolution exercise using a runtime load base.
6. Add a build ID and verify that a mismatched debug file is rejected.
7. Define a field-fault artifact retention policy and test it with two intentionally similar builds.

## Common Mistakes

- debugging a stripped image without preserving matching DWARF;
- using the source revision rather than the exact ELF for address resolution;
- forgetting a PIE/shared-object relocation slide;
- treating a source line as an exact instruction boundary under optimization;
- trusting a corrupt backtrace without inspecting stack and ABI state;
- measuring only `.text` and ignoring bss, stack, heap, or alignment padding;
- relying on `ldd` or one tool’s summary instead of inspecting the ELF;
- letting a fault handler call malloc, printf, locks, or a driver that is not fault-safe;
- omitting linker scripts, generated headers, or post-link tools from provenance;
- assuming debugger reset, watchdog reset, and power-on reset have identical state.

## Related Topics

- [Compiler Modes, Warnings, And Optimization](./compiler-modes-warnings-and-optimization.md)
- [Object Files, Symbols, And Relocations](./object-files-symbols-and-relocations.md)
- [Static And Dynamic Linking](./static-and-dynamic-linking.md)
- [Startup, Runtime, And `main`](./startup-runtime-and-main.md)
- [Debugging With GDB](../correctness-quality-and-security/debugging-with-gdb.md)
- [Bootloaders And Firmware Images](../embedded-c-and-hardware/bootloaders-and-firmware-images.md)

## References

- [GCC debugging options](https://gcc.gnu.org/onlinedocs/gcc/Debugging-Options.html)
- [GDB documentation](https://sourceware.org/gdb/current/onlinedocs/gdb.html/)
- [GDB separate debug files](https://www.sourceware.org/gdb/current/onlinedocs/gdb.html/Separate-Debug-Files.html)
- [GNU Binary Utilities documentation](https://sourceware.org/binutils/docs/binutils.html)
- [GNU `addr2line` documentation](https://sourceware.org/binutils/docs/binutils/addr2line.html)
- [GNU `objdump` documentation](https://sourceware.org/binutils/docs/binutils/objdump.html)

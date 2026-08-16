---
status: draft
reviewed: false
domain: c
difficulty: intermediate
last_reviewed: null
---

# Compiler Modes, Warnings, And Optimization

Compiler options are part of a C program’s effective contract. They select the language dialect, target machine, ABI, diagnostics, optimization assumptions, debug format, and runtime integration. A senior engineer treats the complete command line as versioned source material.

## Learning Objectives

- choose an explicit C dialect and understand extension boundaries;
- turn warnings into actionable quality gates without hiding important diagnostics;
- distinguish debugability, optimization, size, speed, and hardening goals;
- understand target options, attributes, built-ins, pragmas, and inline assembly as implementation contracts;
- use LTO and profile-guided optimization without losing ABI or diagnostic control;
- compare generated behavior using assembly, symbols, maps, and measurements.

## Language And Target Modes

Use an explicit standard mode in project builds. GCC-family examples include `-std=c17`, `-std=c11`, and implementation dialects such as `-std=gnu17`. A base ISO mode restricts extensions that conflict with the selected standard; a GNU mode enables additional implementation features. The exact supported modes and defaults vary by compiler version.

For embedded work, distinguish at least:

- **language mode** — C11, C17, a newer supported edition, or a project-approved GNU dialect;
- **target architecture** — for example `-mcpu`, `-march`, `-mthumb`, or a RISC-V architecture string;
- **ABI and floating-point convention** — hard/soft floating point, register conventions, data model, and alignment;
- **runtime model** — hosted, freestanding, RTOS, or operating-system process;
- **predefined feature macros** — compiler and platform facts consumed by headers;
- **extension policy** — atomics, packed layouts, section attributes, interrupt attributes, and assembly.

Record these settings in the build system and expose them in a generated manifest. Never infer them from the compiler executable name alone.

## Warnings As Design Feedback

A useful baseline for GCC/Clang-like compilers might be:

~~~sh
cc -std=c17 -Wall -Wextra -Wpedantic \
   -Wconversion -Wsign-conversion -Wshadow \
   -Wformat=2 -Wundef -Wdouble-promotion \
   -Werror=implicit-function-declaration \
   -c sensor.c -o sensor.o
~~~

Warning groups differ by compiler and version. Begin with a supported, documented baseline, then add warnings with a clean-up and ownership plan. A warning policy should answer:

- which warnings are errors in new code;
- which legacy warnings are tracked exceptions;
- how generated code is handled;
- whether host and target compilers have equivalent coverage;
- how warning changes are reviewed across compiler upgrades;
- which diagnostics are required for security, portability, and safety.

Do not use `-w` as a release strategy. Do not blanket-disable a warning because one line is inconvenient. Prefer a narrow, documented suppression around a reviewed construct and test the intended behavior.

Warnings cannot prove absence of undefined behavior. They are evidence generated from the compiler’s model; tests, static analysis, sanitizers, code review, and runtime instrumentation cover different failure classes.

## Optimization Levels And the As-If Rule

The compiler may transform a program as long as observable behavior remains consistent with the language and selected implementation contract. This is often summarized as the as-if rule. Undefined behavior, data races, invalid aliasing assumptions, and incorrect `volatile` use remove the guarantees engineers expect.

Typical optimization choices are:

| Choice | Use | Caution |
| --- | --- | --- |
| `-O0` | simple stepping and bring-up | poor timing and large code; can hide optimization-sensitive bugs |
| `-Og` | debug-oriented optimization | still changes control flow and variable lifetime |
| `-O1`/`-O2` | balanced product builds | verify timing, size, and tool support |
| `-O3` | aggressive speed experiments | may increase size and expose target-specific tradeoffs |
| `-Os`/`-Oz` | code-size constrained images | measure speed and alignment effects |
| `-Ofast` | domain-specific numerical experiments | may violate strict language or floating-point expectations |

The name of an optimization level is not a timing guarantee. Measure interrupt latency, worst-case execution time, stack depth, flash wait-state effects, cache behavior, and energy on the real target.

## Debug Information And Optimized Code

Compile with debug information at the stage where source-to-machine mapping is created:

~~~sh
cc -std=c17 -Og -g3 -fno-omit-frame-pointer -c sensor.c -o sensor.o
cc -Og -g3 sensor.o -o sensor
~~~

`-g` does not turn optimization off. At optimized levels, variables may be merged, removed, moved, or represented only briefly. A source line can map to multiple instruction ranges or no instruction at all. Use disassembly and watchpoints to understand the generated code rather than concluding that the debugger is lying.

Keep production optimization representative when diagnosing a production-only problem. Reproducing it with `-O0` can remove the failure.

## Built-Ins, Attributes, And Pragmas

Compilers provide implementation facilities such as:

- `__builtin_expect` and branch prediction hints;
- checked or specialized memory built-ins;
- `__attribute__((section(".fastcode")))`;
- `__attribute__((used))`, `retain`, `weak`, `alias`, and visibility controls;
- alignment and packing attributes;
- `#pragma` controls for diagnostics, packing, optimization, or target features;
- compiler barriers and target-specific memory barriers.

These are useful at a platform boundary, but they are not portable C. Wrap them in project macros with a documented fallback:

~~~c
#if defined(__GNUC__) || defined(__clang__)
#define FW_USED __attribute__((used))
#define FW_SECTION(name) __attribute__((section(name)))
#else
#define FW_USED
#define FW_SECTION(name)
#endif

FW_USED FW_SECTION(".firmware_metadata")
const unsigned char firmware_format_version = 1u;
~~~

The fallback must not silently produce a binary missing a required object. Pair attributes with linker retention rules, map-file checks, and a build-time test.

## `volatile`, Atomics, And Barriers

`volatile` tells the compiler that an access is observable and must not be removed or merged in ways forbidden by the volatile rules. It does not make an operation atomic, establish inter-thread ordering, flush a cache, or replace a hardware barrier. Use the mechanism that matches the contract:

- `volatile` for memory-mapped registers or externally changing objects where the platform requires it;
- C atomics for inter-thread synchronization on objects supported by the implementation;
- compiler barriers for compiler reordering constraints;
- hardware or architecture barriers for device and memory-system ordering;
- RTOS primitives for task synchronization, waiting, and ownership.

Document the boundary because an apparently redundant access can be required for hardware, while an unnecessary volatile qualifier can prevent useful optimization.

## Inline Assembly

Inline assembly is a constrained interface between C and the compiler. Specify inputs, outputs, clobbers, and memory effects accurately:

~~~c
static inline void cpu_pause(void)
{
#if defined(__x86_64__) || defined(__i386__)
    __asm__ volatile ("pause" ::: "memory");
#else
    /* A target-specific implementation belongs here. */
#endif
}
~~~

The `memory` clobber is a compiler-ordering statement, not necessarily a CPU fence. Inline assembly must be reviewed against the target ABI, register allocator, interrupt model, and assembler syntax. Prefer compiler intrinsics or a small assembly translation unit when they express the operation more clearly.

## Link-Time Optimization

With GCC, `-flto` is normally supplied while compiling and during the final link:

~~~sh
cc -std=c17 -O2 -flto -c a.c -o a.o
cc -std=c17 -O2 -flto -c b.c -o b.o
cc -O2 -flto a.o b.o -o app
~~~

LTO can inline across translation units, remove unused code, propagate constants, and expose whole-program diagnostics. It can also:

- change symbol retention and section garbage collection;
- make debug stepping less direct;
- require linker plugin support;
- make inconsistent target options dangerous;
- remove functions referenced only by a bootloader, debugger, script, or vector table unless retained;
- complicate incremental builds and artifact provenance.

Use explicit exported/retained interfaces and post-link checks. Treat any externally discovered symbol as a linker contract, not as an accidental implementation detail.

## Profile-Guided Optimization

PGO uses representative execution profiles to guide branch layout, inlining, and code placement. A profile from a desktop workload may be actively misleading for a microcontroller. If using PGO:

1. define the target workload and profile collection environment;
2. build an instrumented artifact with the same relevant ABI and feature set;
3. collect normal, boundary, failure, and worst-case paths;
4. merge and validate profile data;
5. build the optimized image;
6. re-measure timing, size, power, and safety behavior on the target;
7. keep profile provenance with the binary.

## Reproducibility And Hardening

Build options affect both security and reproducibility. Consider:

- deterministic archives and stable file ordering;
- `-ffile-prefix-map` or equivalent source-path normalization;
- build IDs and exact compiler versions;
- stack protection and control-flow hardening where supported;
- position-independent code or PIE where required;
- read-only relocation and non-executable memory policy on hosted systems;
- signing and measured hashes after final image generation.

Hardening options are target and ABI dependent. Verify the resulting ELF program headers and runtime behavior instead of assuming an option was accepted and effective.

## Exercises

1. Compare `-std=c17` and a GNU dialect for a small extension-using file.
2. Turn the warning baseline into CI gates and document each exception.
3. Compare `-Og`, `-O2`, and size optimization using `size`, disassembly, and a target timing harness.
4. Add a section attribute and prove that the section is retained in an LTO plus garbage-collection build.
5. Write an incorrect inline-assembly constraint, observe the failure, then correct it.
6. Build with and without frame pointers and compare unwinding and stack evidence.
7. Run representative PGO workloads and compare them with a workload that exercises error paths.

## Common Mistakes

- relying on the compiler’s default C dialect;
- treating warnings as proof of correctness or disabling them globally;
- comparing timing at `-O0` with production behavior;
- using `volatile` as a universal concurrency primitive;
- writing inline assembly without complete clobbers;
- enabling LTO without linker retention and symbol checks;
- using desktop PGO data for a different target workload;
- changing optimization and target flags independently across objects;
- stripping symbols before preserving a matching debug artifact;
- assuming accepted options are supported or meaningful on every compiler.

## Related Topics

- [Translation Pipeline](./translation-pipeline.md)
- [Object Files, Symbols, And Relocations](./object-files-symbols-and-relocations.md)
- [ABI, Calling Conventions, And FFI](./abi-calling-conventions-and-ffi.md)
- [Undefined Behavior](../semantics-and-memory/undefined-behavior.md)
- [Correctness, Quality, And Security](../correctness-quality-and-security/index.md)

## References

- [GCC C dialect options](https://gcc.gnu.org/onlinedocs/gcc/C-Dialect-Options.html)
- [GCC warning options](https://gcc.gnu.org/onlinedocs/gcc/Warning-Options.html)
- [GCC optimization options](https://gcc.gnu.org/onlinedocs/gcc/Optimize-Options.html)
- [GCC debugging options](https://gcc.gnu.org/onlinedocs/gcc/Debugging-Options.html)
- [GCC link-time optimization](https://gcc.gnu.org/onlinedocs/gcc/Optimize-Options.html#Optimize-Options)

---
status: draft
reviewed: false
domain: c
difficulty: intermediate
last_reviewed: null
---

# Translation Pipeline

A C program is translated through several conceptual stages. A compiler driver may combine them into one command, but each stage has a different input, output, failure mode, and debugging evidence. Knowing the boundaries is the fastest way to stop guessing about a build.

## Learning Objectives

- distinguish source files, headers, translation units, assembly, objects, and final images;
- inspect preprocessing, compiler, assembler, and linker output;
- understand which parts are required by ISO C and which are implementation choices;
- explain the difference between compile-time, link-time, load-time, and startup failures;
- compare a hosted process pipeline with a freestanding firmware pipeline.

## The Conceptual Stages

The C standard describes translation in abstract phases, including source character handling, comment replacement, preprocessing directives, adjacent string literal concatenation, and syntax/semantic translation. Real compilers implement these activities with more internal passes and may optimize across stage boundaries. The useful engineering model is:

1. **Source discovery** — the driver selects language mode, include paths, predefined macros, and target options.
2. **Preprocessing** — headers are included, macros expand, conditional regions are selected, and `#line` information is generated.
3. **Parsing and semantic analysis** — tokens become declarations and expressions; types, conversions, constraints, and control-flow rules are checked.
4. **Intermediate representation and optimization** — the compiler lowers the program into internal forms and applies transformations justified by the language and selected flags.
5. **Assembly generation** — target instructions, data, directives, debug records, and relocation sites are emitted.
6. **Assembly** — the assembler turns assembly into a relocatable object with sections, symbols, and relocations.
7. **Linking** — the linker combines objects and libraries, resolves symbols, applies relocations, and lays out sections.
8. **Loading or image conversion** — an OS loader maps program segments, or a post-link tool emits a raw binary, Intel HEX, UF2, or vendor image.
9. **Runtime startup** — reset/loader code establishes the machine state, initializes C storage, and enters the application.

The boundaries are not all one-to-one. LTO can preserve compiler IR in object files for a later optimization pass; a JIT or dynamic loader can perform later code generation or relocation; and embedded post-link tools can add headers, hashes, signatures, or checksums.

## A Small Experiment

Create these files:

~~~c
/* message.h */
#ifndef MESSAGE_H
#define MESSAGE_H

int message_length(const char *text);

#endif
~~~

~~~c
/* message.c */
#include "message.h"

int message_length(const char *text)
{
    int length = 0;
    while (text[length] != '\0') {
        ++length;
    }
    return length;
}
~~~

~~~c
/* main.c */
#include "message.h"

int main(void)
{
    return message_length("hello") == 5 ? 0 : 1;
}
~~~

Observe the artifacts with GCC or a compatible driver:

~~~sh
cc -std=c17 -Wall -Wextra -Wpedantic -E main.c -o main.i
cc -std=c17 -Wall -Wextra -Wpedantic -S message.c -o message.s
cc -std=c17 -Wall -Wextra -Wpedantic -g -c message.c -o message.o
cc -std=c17 -Wall -Wextra -Wpedantic -g -c main.c -o main.o
cc -g main.o message.o -o message
~~~

Compare `main.i`, `message.s`, `message.o`, and `message`. The preprocessed file shows what the compiler actually sees; the assembly shows the target instructions before final addresses are known; the object retains unresolved references; and the executable has final symbols, segments, and an entry path.

Useful driver diagnostics include:

~~~sh
cc -### main.c                 # print commands without executing them
cc -v -### main.c              # include search and toolchain details
cc -E -dM - < /dev/null        # predefined macros
cc -print-search-dirs          # compiler and library search paths
cc -print-sysroot              # selected logical target root
~~~

The exact options differ by compiler. Treat these commands as GCC-family examples, not ISO C facilities.

## Translation Unit Boundaries

A translation unit is a source file after preprocessing. A declaration in a header is copied into every translation unit that includes it; the function body in another `.c` file is not visible to the compiler unless the implementation uses a whole-program mechanism such as LTO. This explains why:

- a declaration mismatch can compile in one file and fail at link time;
- `static` at file scope gives internal linkage and keeps a symbol local to one translation unit;
- macros can silently change different translation units in different ways;
- an inline definition, an external definition, and a declaration have different linkage rules;
- changing a header can require rebuilding all dependent translation units.

Use dependency generation to make the build graph explicit:

~~~sh
cc -std=c17 -MMD -MP -c message.c -o message.o
cat message.d
~~~

Do not treat a successful incremental build as evidence that every affected object was rebuilt. Stale objects are a common source of impossible-looking ABI and link behavior.

## Hosted And Freestanding Pipelines

In a hosted build, the driver usually adds startup objects, the C library, compiler runtime support, and a platform-specific dynamic linker or loader contract. `main` is called by a runtime entry such as `__libc_start_main` on common GNU/Linux systems, although the exact arrangement is implementation-specific.

In a freestanding build, the project commonly supplies:

- a reset or ROM entry point;
- a vector table and initial stack value;
- a linker script;
- startup code to copy `.data` and clear `.bss`;
- a selected subset or replacement of libc;
- the application entry and a policy for returning from it.

The compiler still emits objects and relocations, but there may be no process loader, filesystem, environment, dynamic linker, or standard `main` startup. A `main` function can be a project convention rather than the first C function executed.

## Failure Classification

| Failure | Typical stage | First evidence |
| --- | --- | --- |
| Missing header | preprocessing | include search and preprocessed output |
| Macro changes declaration | preprocessing | `-E`, `-dD`, compiler predefined macros |
| Invalid expression or type | semantic compilation | diagnostic with source location |
| Unsupported instruction | assembly or compiler | target architecture flags and assembly |
| Undefined reference | linking | object symbol tables and link order |
| Region overflow | linking | map file and linker `ASSERT` |
| Missing shared object | loading | interpreter and dynamic dependency inspection |
| Fault before `main` | startup | reset trace, vector table, data/bss checks |
| Wrong source line in crash | debug/provenance | exact ELF, DWARF, load address, build ID |

Always identify the stage before changing source code. Adding a declaration cannot repair a wrong linker script; changing optimization cannot supply a missing target library; and rebuilding without checking the command can leave the same misconfiguration intact.

## Optimization And LTO Boundaries

Without LTO, the compiler generally optimizes each translation unit independently. The linker sees machine-code objects and cannot freely inline an ordinary function across them. With LTO, object files may carry compiler IR and the final link can perform whole-program analysis. This changes the meaning of “compile” and “link” as performance stages, but not the need for a correct ABI at externally visible boundaries.

LTO increases the importance of:

- consistent compiler and target options;
- complete and correct declarations;
- visibility and retention attributes;
- linker plugin support;
- reproducible build inputs;
- preserving symbols needed by bootloaders, debuggers, or external tools.

## Exercises

1. Generate preprocessed, assembly, object, and executable artifacts for the three-file example.
2. Remove the definition of `message_length` and diagnose the undefined symbol with `nm`.
3. Change the declaration to `size_t message_length(const char *)` in one file only and explain the compiler and linker behavior.
4. Add a macro that changes a structure layout in one translation unit and observe the ABI failure.
5. Build with and without LTO and compare symbol tables and disassembly.
6. Build once with `-g` and once stripped; keep the debug file and resolve an address from both.
7. Repeat the experiment with a cross-compiler and record every target-specific difference.

## Common Mistakes

- thinking a `.c` file is compiled with the headers as separate input rather than after preprocessing;
- debugging the source without first checking the preprocessor output;
- assuming the compiler emits final addresses before linking;
- forgetting that libraries are selected by the linker and often extracted on demand;
- mixing objects produced with incompatible target, ABI, or structure-packing options;
- assuming `main` is the first function in every C system;
- stripping the only copy of debug information;
- enabling LTO without checking retention, symbol visibility, or linker-plugin support;
- allowing stale generated headers or objects into an incremental build.

## Related Topics

- [Compilation, Linking, And ABI overview](./index.md)
- [Compiler Modes, Warnings, And Optimization](./compiler-modes-warnings-and-optimization.md)
- [Object Files, Symbols, And Relocations](./object-files-symbols-and-relocations.md)
- [Startup, Runtime, And `main`](./startup-runtime-and-main.md)
- [C Programming](../index.md)

## References

- [GCC overall options](https://gcc.gnu.org/onlinedocs/gcc/Overall-Options.html)
- [GCC preprocessor options](https://gcc.gnu.org/onlinedocs/gcc/Preprocessor-Options.html)
- [GCC link options](https://gcc.gnu.org/onlinedocs/gcc/Link-Options.html)
- [C11 public draft N1570, translation phases](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n1570.pdf)

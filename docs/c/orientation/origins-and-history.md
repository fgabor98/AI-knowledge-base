---
status: draft
reviewed: false
domain: c
difficulty: beginner
last_reviewed: null
---

# Origins And History

C is easiest to understand when its history is treated as an engineering history rather than a list of release dates. The language grew out of operating-system development, small machines, limited memory, separate compilation, and the need to write software that could be moved between related machines without giving up control of the hardware.

Understanding that background explains why C has:

- a small core language
- explicit pointers
- arrays that are closely related to pointers
- manual storage management
- separate compilation
- a standard library that is deliberately smaller than an operating system
- implementation-defined details around data representation and execution
- no built-in object model, garbage collector, or mandatory runtime

## The Prehistory: BCPL And B

### BCPL

BCPL was designed by Martin Richards in the 1960s. It was intended for systems programming and compiler implementation. Its design emphasized portability across machines and a compact implementation. BCPL did not have C's type system, but it established part of the lineage that led to C.

Important historical lessons:

- C did not begin as a high-level application language.
- Portability was important from the beginning.
- The language was expected to map reasonably directly onto real machine operations.
- A small compiler and runtime were valuable constraints.

### B

Ken Thompson developed B at Bell Laboratories around 1969–1970. B was derived from BCPL and was used in the early development of Unix. B retained a compact syntax but did not provide the typed data model that later made C more suitable for larger systems software.

The transition from B to C was not a clean-sheet redesign. It was an incremental response to practical programming needs, especially the need to describe data types and machine objects more precisely.

## The Birth Of C

Dennis Ritchie developed C at Bell Laboratories during 1971–1973. The work began by extending B and continued through several iterations of the language and compiler.

The major additions included:

- typed variables
- structure types
- pointers
- richer expressions
- a more capable compiler
- a better fit for writing operating-system components and utilities

The development was closely connected to Unix. In 1973, much of Unix was rewritten in C for the PDP-11. This was a decisive demonstration: a language that was close enough to the machine for systems work could also be portable enough to move a substantial operating system.

This combination still defines C's niche:

- high-level enough to organize large programs
- low-level enough to describe memory and device-facing operations
- small enough to support constrained implementations
- abstract enough to move between processor families

Ritchie's historical account describes the language's relationship to real machines, small compilers, operating systems, and portability in detail. It is one of the best primary sources for understanding the design pressures behind C.

## Timeline

| Period | Development | Engineering significance |
|---|---|---|
| 1960s | Martin Richards develops BCPL | Establishes a compact systems-language lineage |
| 1969–1970 | Ken Thompson develops B | Provides the immediate predecessor used in early Unix work |
| 1971–1973 | Dennis Ritchie develops C from B | Adds types and machine-oriented structure |
| 1973 | Unix is substantially rewritten in C on the PDP-11 | Demonstrates that a systems language can be both practical and portable |
| 1978 | Kernighan and Ritchie publish The C Programming Language | Provides the widely used informal language definition of the period |
| 1983 | ANSI forms the X3J11 committee | Begins formal standardization of C |
| 1989 | ANSI C, X3.159-1989 | First major standardized C language specification |
| 1990 | ISO C90, ISO/IEC 9899:1990 | International publication of the first ISO C standard |
| 1995 | C95 amendment | Adds corrections and library-related additions to C90 |
| 1999 | C99, ISO/IEC 9899:1999 | Adds major language and library capabilities used by modern C |
| 2011 | C11, ISO/IEC 9899:2011 | Adds atomics, threads, alignment, and other modern facilities |
| 2018 | C17, ISO/IEC 9899:2018 | Primarily consolidates corrections and clarifications |
| 2024 | C23, ISO/IEC 9899:2024 | Current ISO C edition, with language cleanup and new facilities |

The standards timeline is maintained by [WG14](https://open-std.org/jtc1/sc22/wg14/www/projects.html). The published C23 standard is listed by [ISO](https://www.iso.org/standard/82075.html).

## What The History Explains

### Arrays and pointers

C was designed in a world where contiguous machine memory and address arithmetic mattered. Arrays and pointers are not identical types, but the language makes them interact closely. This is powerful for buffers and hardware interfaces, but it also creates bounds and lifetime hazards.

### Manual storage

C assumes that the programmer and the surrounding runtime can decide how storage is obtained and released. A hosted program may use malloc and free. A firmware project may use only static storage and linker-defined regions. Neither choice is imposed by the language.

### Separate compilation

Large systems were built from separately compiled source files and libraries. C therefore has a strong model of declarations, definitions, translation units, external linkage, object files, and linkers. These topics are not accidental build-system details; they are part of how C projects scale.

### Small runtime assumptions

C was intended to help build operating systems and tools. It could not assume that an operating system already existed. This is why freestanding implementations are possible and why startup need not begin at main.

### Machine-near but not machine-identical code

C exposes many concepts that map well to machines, but it is not portable assembly language. The standard can leave representation, alignment, evaluation order, or behavior categories open to the implementation. Correct low-level C requires understanding both the abstract language and the target machine.

## Standardization Changed The Meaning Of “C”

Before formal standardization, “C” often meant a compiler family, a version of the K&R book, or local practice. Standardization established a shared vocabulary for:

- syntax
- declarations
- types
- expressions
- libraries
- diagnostics
- implementation-defined behavior
- translation limits

Standardization did not eliminate dialects. Modern projects still distinguish among:

- strict ISO C90, C99, C11, C17, or C23
- GNU dialects such as GNU C17
- vendor extensions
- operating-system-specific APIs
- compiler intrinsics
- architecture-specific assembly
- project coding standards

The practical lesson is to ask which C is meant:

1. Which ISO edition?
2. Which compiler?
3. Which C library?
4. Which ABI?
5. Which operating system or startup model?
6. Which vendor and architecture extensions?
7. Which safety, security, or product rules?

## Historical Dialects Still Seen In Embedded Work

- K&R-style declarations in old firmware
- C90 restrictions in legacy safety projects
- C99 features in modern but conservative embedded code
- C11 atomics and static assertions where toolchains support them
- C17 as a stable compatibility target
- C23 in newer host environments and selected modern toolchains
- GNU C extensions in Linux kernel and toolchain-oriented code
- vendor-specific attributes, address spaces, pragmas, and intrinsics
- compiler-specific startup and interrupt declarations

A legacy codebase may use an older dialect for reasons that are not purely technical:

- the certified compiler is fixed
- the vendor SDK assumes a particular compiler
- the target library lacks newer headers
- the code must build on multiple old toolchains
- qualification evidence is tied to a particular build configuration

Do not modernize syntax by instinct. First identify the compatibility and evidence constraints.

## Learning Exercises

### Exercise 1: Build a project timeline

Create a one-page timeline containing:

- BCPL
- B
- early C
- Unix on the PDP-11
- K&R C
- ANSI C
- C90
- C99
- C11
- C17
- C23

For each milestone, record one language or engineering consequence.

### Exercise 2: Compare dialects

Compile the same small program under:

- c90
- c99
- c11
- c17
- c23, if supported by the compiler

Record:

- accepted syntax
- warnings
- predefined standard-version macros
- library availability
- extensions enabled by the selected mode

### Exercise 3: Identify the historical assumption

For each feature, explain the historical problem it helps solve:

- pointers
- structures
- separate compilation
- static storage
- function pointers
- arrays and byte access
- compiler-defined extensions

## Common Misconceptions

- C was not designed as a portable replacement for every use of assembly.
- C is not defined by the K&R book anymore, although the book remains historically important.
- C23 does not mean every compiler, libc, vendor SDK, or embedded project supports C23.
- A compiler accepting a construct does not make the construct ISO C.
- “Low-level” does not mean “outside the language standard.”
- “Portable” does not mean “independent of all platform behavior”; it means the required platform assumptions are identified and controlled.
- Old code is not automatically bad code, and new syntax is not automatically safer code.

## Related Topics

- [Standards And Conformance](./standards-and-conformance.md)
- [Hosted And Freestanding C](./hosted-and-freestanding.md)
- [Declarations And Declarators](../language-fundamentals/declarations-and-declarators.md)
- [Translation Pipeline](../compilation-linking-and-abi/translation-pipeline.md)

## References

- [Dennis Ritchie, The Development of the C Language](https://www.bell-labs.com/usr/dmr/www/chist.pdf)
- [WG14 project status and C standards history](https://open-std.org/jtc1/sc22/wg14/www/projects.html)
- [WG14 approved standards and public drafts](https://open-std.org/jtc1/sc22/wg14/www/standards.html)
- [ISO/IEC 9899:2024](https://www.iso.org/standard/82075.html)
- [GCC C dialect options](https://gcc.gnu.org/onlinedocs/gcc/C-Dialect-Options.html)

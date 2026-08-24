---
status: draft
reviewed: false
domain: c
difficulty: beginner
last_reviewed: null
---

# Standards And Conformance

A C standard is a contract for the language and its standard library. It is not a complete description of a compiler command, an executable file, an operating system, a board, or a product.

Standards knowledge is a practical engineering skill. It lets you answer:

- Is this behavior guaranteed?
- If not, who chooses the behavior?
- Is the choice documented?
- Is the program invalid and required to produce a diagnostic?
- Is the compiler accepting a non-standard extension?
- Does the target environment provide the required library or startup behavior?
- Can the assumption be recorded and tested?

## The Standards Family

The ISO C standard is revised periodically. The important editions for current engineering work are:

| Common name | ISO publication | Practical significance |
|---|---|---|
| C89 | ANSI X3.159-1989 | First ANSI standardized C; still encountered in legacy code |
| C90 | ISO/IEC 9899:1990 | First ISO C edition |
| C95 | ISO/IEC 9899:1990/Amd 1:1995 | Amendment to C90 |
| C99 | ISO/IEC 9899:1999 | Adds major language and library features used by modern C |
| C11 | ISO/IEC 9899:2011 | Adds atomics, threads, alignment, and other facilities |
| C17 | ISO/IEC 9899:2018 | Maintenance release with corrections and clarifications |
| C23 | ISO/IEC 9899:2024 | Current ISO C edition |

The current standard and the revision history are maintained through [ISO](https://www.iso.org/standard/82075.html) and [WG14](https://open-std.org/jtc1/sc22/wg14/www/projects.html). Published ISO standards are normally licensed documents. WG14 also publishes public working documents and older public drafts that are useful for study.

### Public drafts and final standards

A public draft is not automatically the final standard. For example:

- WG14 N1570 is the well-known public draft corresponding to C11.
- WG14 N1256 is the consolidated public C99 draft with technical corrigenda.
- C23 has a final ISO publication and earlier public working drafts.
- Compiler documentation may describe implementation support more accurately than a draft for a particular toolchain.

Use public drafts for learning and navigation, but label them correctly in notes and reviews.

## What The Standard Defines

The standard defines the language and library at an abstract level, including:

- source representation
- lexical elements
- syntax and constraints
- types and objects
- conversions
- expressions and statements
- declarations and definitions
- program execution
- input and output abstractions
- standard-library interfaces
- implementation limits
- required diagnostics
- portability-related behavior categories

The standard deliberately does not define every transformation detail. It does not prescribe:

- a particular compiler architecture
- an intermediate representation
- an assembler
- an object-file format
- a linker
- a loader
- a processor instruction set
- a board memory map
- a vendor peripheral register
- a filesystem
- a shell
- a debugger

Those belong to the implementation, ABI, operating-system, and platform contracts.

## Conforming Implementations And Programs

### Hosted implementation

A hosted implementation provides the complete standard library and the hosted program startup model. Ordinary desktop and server C environments are usually hosted.

A hosted program normally enters through a function named main and can use facilities such as:

- streams
- files
- dynamic allocation
- formatted I/O
- time functions
- locale support
- mathematical functions
- operating-system interfaces supplied separately by POSIX or the platform

“Hosted” does not mean “safe,” “portable across operating systems,” or “free from compiler extensions.” It only describes the language execution environment.

### Freestanding implementation

A freestanding implementation is required to provide a smaller environment. Startup and termination are implementation-defined, and the complete hosted library is not required.

Typical examples include:

- operating-system kernels
- bootloaders
- microcontroller firmware
- firmware libraries
- hypervisor components
- early platform initialization code

Freestanding does not mean that no libraries exist. A freestanding project may provide:

- a vendor HAL
- a board-support package
- an RTOS
- a small libc
- compiler runtime helpers
- a custom allocator
- a logging library
- a protocol stack

Those facilities are project or implementation contracts, not automatically ISO C guarantees.

### Strictly conforming program

A strictly conforming program is written so that its behavior does not depend on unspecified, undefined, or implementation-defined behavior and does not exceed the minimum implementation limits.

This is a useful ideal for portable library code, but it is not the only legitimate engineering target. Embedded and operating-system code often depends on documented implementation or platform behavior. The important requirement is to make that dependency explicit and controlled.

### Conforming program

A conforming program may use implementation-defined or implementation-supported behavior, provided it is accepted by a conforming implementation and its required behavior remains within the standard's rules.

In practice, a project profile usually looks like:

- a selected ISO dialect
- a selected compiler version
- a selected C library
- a selected ABI
- documented extensions
- a target processor
- a build configuration
- a coding and verification policy

## Behavior Categories

### Fully specified behavior

The standard defines the required result or effect. Portable code can rely on it within the standard's stated conditions.

### Implementation-defined behavior

The implementation may choose among permitted alternatives, but it must document the choice.

Examples can include:

- the signedness of plain char
- the size and representation of some types
- aspects of integer conversion
- implementation limits
- ABI and object layout choices

The right response is not necessarily to avoid all implementation-defined behavior. It is to:

1. identify it,
2. verify the target choice,
3. document it,
4. isolate it behind an interface when useful,
5. test it in the supported configuration.

### Unspecified behavior

The implementation may choose among permitted alternatives and does not have to document which alternative it selected.

Do not write code that depends on one unspecified ordering or result.

### Undefined behavior

The standard imposes no requirements after the program reaches the undefined operation. The compiler may assume that valid programs do not reach it and optimize accordingly.

Common sources include:

- out-of-bounds access
- signed overflow
- invalid shifts
- use-after-free
- invalid alignment
- data races
- invalid object lifetime
- unsequenced conflicting side effects

Undefined behavior is not a normal runtime error category. It is a failure of the program's assumptions.

### Constraint violation

A constraint violation is a rule for which the implementation must issue a diagnostic. After a diagnostic, the implementation may still translate the program as an extension.

A compiler warning or error is therefore not the same thing as a runtime behavior guarantee. Always ask whether the source is:

- valid ISO C,
- valid only under an extension,
- invalid but accepted as an extension,
- or invalid and rejected.

## Compiler Dialects

Compilers commonly provide both standard-oriented and extension-oriented modes.

For GCC, examples include:

- -std=c17
- -std=gnu17
- -std=c23
- -std=gnu23
- -pedantic
- -Wpedantic
- -ffreestanding
- -fhosted

The exact accepted values and support level depend on the compiler version. A base ISO mode generally limits extensions that conflict with the selected standard. A GNU mode enables GNU extensions in addition to the base standard.

A project should not leave the dialect implicit. Record it in:

- the build system
- the compiler command
- CI configuration
- generated build metadata
- release manifests
- developer documentation

## Feature-Test Macros

The implementation can expose standard-version and environment information through predefined macros. A small probe can make the selected environment visible:

~~~c
#include <stdio.h>

int main(void)
{
#if defined(__STDC_VERSION__)
    printf("__STDC_VERSION__ = %ld\n", (long)__STDC_VERSION__);
#else
    puts("__STDC_VERSION__ is not defined");
#endif

#if defined(__STDC_HOSTED__)
    printf("__STDC_HOSTED__ = %d\n", __STDC_HOSTED__);
#else
    puts("__STDC_HOSTED__ is not defined");
#endif

    return 0;
}
~~~

These macros are useful evidence, but they are not a complete feature test. A compiler can define a language-version macro while a particular library, target, or feature remains incomplete or unavailable.

Prefer:

- compiler feature tests
- header availability checks
- library capability checks
- project configuration tests
- compile-time assertions
- a supported-toolchain matrix

## Defect Reports And Clarifications

Standards are revised through more than numbered editions. WG14 also tracks:

- defect reports
- clarification requests
- technical corrigenda
- working papers
- proposals for future revisions

A defect report may clarify how an existing rule should be understood. A proposal may change a future standard. These are not interchangeable.

When reading a standards discussion:

1. Identify the target standard edition.
2. Determine whether the paper is a proposal, issue, interpretation, or final wording.
3. Check whether the change was adopted.
4. Check whether the compiler implements it.
5. Avoid treating a future working draft as a current portable requirement.

The [WG14 issues index](https://www.open-std.org/jtc1/sc22/wg14/issues/) is the right place to follow published issue tracking.

## A Practical Conformance Record

For an embedded component, record at least:

~~~text
Language dialect:     C17
Compiler:             arm-none-eabi-gcc <project-pinned-version>
C library:            <newlib/picolibc/vendor libc/none>
ABI:                  <target ABI>
CPU and ISA:          <exact processor and options>
Execution model:      freestanding with RTOS
Extensions:           vendor interrupt attributes, selected built-ins
Warnings:             project warning profile
Static analysis:      project analyzer and rule set
Linker:               project linker script and memory map
Verification:         host tests, target tests, hardware-in-the-loop
~~~

This record turns vague statements such as “we use standard C” into an auditable engineering description.

## How To Read A Standard Rule

When a rule is difficult:

1. Locate the exact clause.
2. Identify whether it is a constraint, semantic rule, library rule, or note.
3. Read the definitions of every term used.
4. Check the preconditions.
5. Classify the result as specified, implementation-defined, unspecified, or undefined.
6. Compare the compiler behavior only after understanding the rule.
7. Write a minimal experiment if the interpretation is still unclear.
8. Record the compiler and target used for the experiment.

Do not use a compiler experiment to prove that behavior is portable. An experiment proves what one implementation did under one build configuration.

## Learning Exercises

### Exercise 1: Classify behavior

For each case, determine the category:

- signed integer overflow
- order of evaluation of function arguments
- plain char signedness
- an invalid format string
- a compiler-specific attribute
- a missing prototype in a selected language mode
- a documented target ABI calling convention

Then state which document or tool would provide the answer.

### Exercise 2: Build a dialect matrix

Create a small matrix for the toolchains you use:

| Toolchain | ISO mode | Extension mode | Hosted/freestanding | C library | C23 support |
|---|---|---|---|---|---|
| Host compiler |  |  |  |  |  |
| Embedded compiler |  |  |  |  |  |
| CI compiler |  |  |  |  |  |

### Exercise 3: Separate guarantees

Take one embedded register-access function and mark every assumption as:

- ISO C
- compiler extension
- ABI
- vendor SDK
- processor manual
- board design
- product requirement

## Common Mistakes

- Treating a compiler accepting code as proof that the code is standard-conforming.
- Calling every compiler warning undefined behavior.
- Calling every implementation-defined choice a bug.
- Treating an informative note as a normative requirement.
- Confusing an old public draft with the current published standard.
- Assuming that C23 support implies complete C23 library support.
- Using __STDC_VERSION__ as the only feature test.
- Leaving the selected dialect implicit in a production build.
- Reading a POSIX rule as if it were part of ISO C.
- Treating a vendor HAL API as portable C language behavior.

## Related Topics

- [Hosted And Freestanding C](./hosted-and-freestanding.md)
- [Use Cases And Environments](./use-cases-and-environments.md)
- [Undefined Behavior](../semantics-and-memory/undefined-behavior.md)
- [Compiler Modes, Warnings, And Optimization](../compilation-linking-and-abi/compiler-modes-warnings-and-optimization.md)
- [Portability](../correctness-quality-and-security/portability.md)

## References

- [ISO/IEC 9899:2024](https://www.iso.org/standard/82075.html)
- [WG14 C language committee](https://open-std.org/jtc1/sc22/wg14/)
- [WG14 project status and standards history](https://open-std.org/jtc1/sc22/wg14/www/projects.html)
- [WG14 approved standards and public drafts](https://open-std.org/jtc1/sc22/wg14/www/standards.html)
- [WG14 C standard issues](https://www.open-std.org/jtc1/sc22/wg14/issues/)
- [GCC C dialect options](https://gcc.gnu.org/onlinedocs/gcc/C-Dialect-Options.html)
- [Clang command guide](https://clang.llvm.org/docs/CommandGuide/clang.html)

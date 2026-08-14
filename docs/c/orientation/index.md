---
status: draft
reviewed: false
domain: c
difficulty: beginner
last_reviewed: null
---

# Orientation

Orientation establishes the historical, standards, environmental, and tooling context needed to learn C accurately. It answers a question that is easy to skip and expensive to ignore:

> Which part of a C program's behavior comes from the language, which part comes from the implementation, and which part comes from the platform?

This distinction is especially important in embedded work. A desktop C program normally begins inside an operating system process with a substantial runtime and standard library. A microcontroller program may begin at a reset vector, execute before any operating system exists, and depend on a linker script and vendor-defined register layout.

## What This Chapter Teaches

- Why C was designed around a small machine-oriented core.
- How C evolved from early Unix development into successive ISO standards.
- How to distinguish an ISO C guarantee from a compiler extension, ABI rule, operating-system interface, or hardware requirement.
- What hosted and freestanding environments provide.
- How C is used across bare-metal firmware, RTOSes, embedded Linux, kernels, bootloaders, drivers, and libraries.
- How to build a disciplined learning and investigation workflow.
- How to choose the right experiment: host executable, object file, disassembly, unit test, target image, or hardware trace.

## Recommended Order

1. [Origins And History](./origins-and-history.md)
2. [Standards And Conformance](./standards-and-conformance.md)
3. [Use Cases And Environments](./use-cases-and-environments.md)
4. [Hosted And Freestanding C](./hosted-and-freestanding.md)
5. [Learning Workflow And Tooling](./learning-workflow-and-tooling.md)

The order is deliberate:

- History explains why the language has its shape.
- Standards explain what the language actually guarantees.
- Environments explain why the same source code behaves differently across systems.
- Hosted and freestanding C make the runtime boundary explicit.
- Tooling turns those ideas into observable evidence.

## The Three-Contract Mental Model

Treat every C project as the intersection of three contracts.

### 1. Language contract

The ISO C standard defines things such as:

- syntax
- types
- expressions
- object lifetime rules
- conversions
- program execution semantics
- standard-library interfaces
- required diagnostics for constraint violations
- implementation-defined and undefined behavior categories

The language contract does not define every detail of a deployed system.

### 2. Implementation contract

The compiler, assembler, linker, C library, and ABI define or constrain things such as:

- supported C dialects
- extensions
- data-model choices
- calling conventions
- object-file format
- startup objects
- generated helper functions
- optimization assumptions
- library availability
- debug information
- linker-script behavior

Two compilers can accept the same ISO C source and produce different but valid binaries.

### 3. Platform and product contract

The processor, board, operating system, RTOS, boot flow, hardware manual, and product requirements define things such as:

- reset behavior
- memory map
- interrupt model
- register semantics
- cache and DMA rules
- scheduling limits
- device ownership
- update and recovery behavior
- safety and security requirements
- acceptable latency, memory, and energy budgets

A correct embedded program must satisfy all three contracts at once.

## Core Vocabulary

| Term | Meaning in this roadmap |
|---|---|
| ISO C | The standardized programming language and standard library |
| Implementation | A compiler and its associated execution environment |
| Hosted environment | An environment with the full standard library and a defined program startup model |
| Freestanding environment | An environment with a smaller required library and implementation-defined startup and termination |
| C dialect | A selected language version plus implementation extensions, such as C17 or GNU C17 |
| ABI | The binary interface governing calling conventions, layout, symbols, and linkage |
| Platform | The processor, runtime, operating system, board, and hardware interfaces |
| BSP | Board-support package containing startup, hardware setup, drivers, and platform integration |
| Toolchain | Compiler, assembler, linker, debugger, libraries, binary utilities, and build integration |
| Conformance | The degree to which a program or implementation follows a specified standard and documented profile |

## Orientation Checkpoint

Before moving into the language fundamentals, you should be able to answer:

- What does the ISO C standard specify?
- What does it intentionally leave to the implementation?
- Why can a program have no operating-system process and still be a C program?
- Why is a vendor HAL not part of ISO C?
- Why might a kernel compile as GNU C but not use a normal hosted libc?
- Which evidence would you inspect when a source-level assumption and the final binary disagree?

## Related Topics

- [Language Fundamentals](../language-fundamentals/index.md)
- [Compilation, Linking, And ABI](../compilation-linking-and-abi/index.md)
- [Embedded C And Hardware](../embedded-c-and-hardware/index.md)
- [Systems And Embedded Architecture](../../systems-and-embedded-architecture/index.md)
- [Build Systems](../../build-systems/index.md)

## References

- [ISO/IEC 9899:2024 overview](https://www.iso.org/standard/82075.html)
- [WG14 C language committee](https://open-std.org/jtc1/sc22/wg14/)
- [WG14 project status and standards history](https://open-std.org/jtc1/sc22/wg14/www/projects.html)

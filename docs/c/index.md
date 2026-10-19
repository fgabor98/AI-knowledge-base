---
status: draft
reviewed: false
domain: c
difficulty: beginner
last_reviewed: null
---

# C Programming

A comprehensive C roadmap for systems and embedded engineering, from language fundamentals through senior-level platform, toolchain, safety, and low-level hardware knowledge.

## How To Use This Section

Track what you have covered on the [progress checklist](./progress.md): tick topics
as you learn them, and see which pages you have visited but not finished.

If you already write C professionally, do not start at page one:

1. Take the [Language Fundamentals](./language-fundamentals/test-out.md) and
   [Semantics And Memory](./semantics-and-memory/test-out.md) test-outs. Each question
   points to the page to read if you miss it, so you read only the pages that cover your
   gaps.
2. Work through the [Failure Case Studies](./failure-case-studies/index.md): fifteen
   reproducible failures, each with the generated code, the rule involved, and which
   tools did and did not catch it.
3. Use the later chapters as reference and practice. The pages on
   [GDB](./correctness-quality-and-security/debugging-with-gdb.md),
   [sanitizers](./correctness-quality-and-security/sanitizers-and-dynamic-analysis.md),
   [binary inspection](./compilation-linking-and-abi/debug-information-and-binary-inspection.md),
   [linking](./compilation-linking-and-abi/static-and-dynamic-linking.md), and
   [linker scripts](./compilation-linking-and-abi/linker-scripts-and-memory-layout.md)
   walk through real tool sessions.
4. Finish with the [capstones](./professional-and-capstone/index.md) and the
   [senior-level outcomes](./professional-and-capstone/senior-level-outcomes.md).

Code on these pages is checked: C code blocks compile under `-Wall -Wextra -Werror`,
and the examples, case studies, test-out answers, and tool-session transcripts are
rerun by `scripts/validate-c.sh`. Transcripts come from GCC 13 on x86-64 Linux; where a
page states a result for another target, it says so.

## Learning Path

1. [Orientation](./orientation/index.md)
2. [Language Fundamentals](./language-fundamentals/index.md)
3. [Semantics And Memory](./semantics-and-memory/index.md)
4. [Modular Design And APIs](./modular-design-and-apis/index.md)
5. [Standard Library And Ecosystem](./standard-library-and-ecosystem/index.md)
6. [Compilation, Linking, And ABI](./compilation-linking-and-abi/index.md)
7. [Correctness, Quality, And Security](./correctness-quality-and-security/index.md)
8. [Embedded C And Hardware](./embedded-c-and-hardware/index.md)
9. [Platform-Specific C](./platform-specific-c/index.md)
10. [Advanced C](./advanced-c/index.md)
11. [Failure Case Studies](./failure-case-studies/index.md)
12. [Professional Practice And Capstones](./professional-and-capstone/index.md)

## Coverage

- Origins and standards
- Hosted and freestanding C
- Syntax, declarations, types, expressions, and control flow
- Pointers, memory, lifetime, representation, and undefined behavior
- Header design, APIs, ownership, errors, and architecture
- ISO C library, libc, POSIX, RTOS, and embedded libraries
- Compilation, linking, startup, ABI, cross-compilation, and binary inspection
- Testing, debugging, static analysis, sanitizers, safety, and security
- Embedded C, interrupts, MMIO, DMA, real-time constraints, and drivers
- Processor platforms, embedded Linux, multicore systems, and compiler extensions
- Advanced concurrency, optimization, numerical C, protocols, and interoperability
- Reproducible failure case studies with compiler output, detection results, and fixes
- Senior engineering practice and integrated capstone projects

## Related Tracks

- [Algorithms And Data Models](../algorithms-and-data-structures/index.md)
- [Build Systems](../build-systems/index.md)
- [Linux Userspace And System Programming](../linux-userspace-and-system-programming/index.md)
- [Linux Kernel Programming](../linux-kernel/index.md)
- [Debugging And Diagnostics](../debugging/index.md)
- [Systems And Embedded Architecture](../systems-and-embedded-architecture/index.md)
- [Embedded Productization](../embedded-productization/index.md)
- [Topic Map](../topic-map.md)

## References

- [ISO/IEC 9899 standards and drafts (WG14)](https://open-std.org/jtc1/sc22/wg14/www/standards.html)
- [C11 public draft N1570](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n1570.pdf);
  clause numbers in this section refer to it unless a page says otherwise
- [GCC manual](https://gcc.gnu.org/onlinedocs/gcc/)
- [GNU Binary Utilities](https://sourceware.org/binutils/docs/binutils.html) and
  [GNU ld](https://sourceware.org/binutils/docs/ld/)
- [GDB documentation](https://sourceware.org/gdb/current/onlinedocs/gdb.html/)
- [System V ABI and ELF specifications](https://refspecs.linuxfoundation.org/elf/)
- [Arm ABI specifications](https://github.com/ARM-software/abi-aa)
- [POSIX.1-2024](https://pubs.opengroup.org/onlinepubs/9799919799/)
- [SEI CERT C Coding Standard](https://wiki.sei.cmu.edu/confluence/display/c)

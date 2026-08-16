---
status: draft
reviewed: false
domain: c
difficulty: intermediate
last_reviewed: null
---

# Standard Library Overview

The ISO C library is organized into headers that declare types, macros, functions, and objects. The standard defines contracts, not one implementation strategy. A hosted implementation must provide the required facilities for its selected edition; a freestanding implementation has a smaller minimum set and may omit or replace many runtime services.

## Learning Objectives

- Map common headers to their responsibilities.
- Distinguish ISO C facilities from POSIX and implementation extensions.
- Understand hosted and freestanding library requirements.
- Check feature, version, and implementation availability.
- Treat library preconditions and postconditions as part of API design.
- Evaluate a library call for embedded cost and execution context.

## Header Map

| Header | Main facilities | Embedded questions |
| --- | --- | --- |
| stddef.h | size_t, ptrdiff_t, NULL, offsetof | Are size and pointer-difference types used correctly? |
| stdint.h | fixed-width integer types and limits | Does the target provide the requested exact width? |
| inttypes.h | integer format and conversion macros | Are diagnostics and serialization widths explicit? |
| limits.h | implementation limits | What are CHAR_BIT and integer ranges? |
| stdbool.h | bool, true, false before C23 | Is the selected language mode consistent? |
| stdlib.h | allocation, conversion, exit, search, sort | Is allocation bounded and allowed in this context? |
| string.h | byte and string operations | Are bounds, overlap, and termination valid? |
| stdio.h | streams and formatted I/O | Does this pull in locks, heap, or syscalls? |
| errno.h | error indicator macros | Is errno available and context-safe on this libc? |
| assert.h | diagnostic assertions | What happens when assertions are disabled? |
| math.h | floating-point functions | Is libm present and is the cost acceptable? |
| time.h | calendar and processor time | What clock and resolution does the target provide? |
| ctype.h | character classification and case conversion | Is locale state configured and input unsigned-char safe? |
| stdatomic.h | atomic types and operations | Which lock-free and memory-order guarantees exist? |
| threads.h | C11 thread API | Does the target provide a C threads runtime? |
| signal.h | ISO signal interface | Which signals and contexts are meaningful on the target? |
| setjmp.h | non-local control transfer | Is cleanup and asynchronous use safe? |

The exact set and behavior depend on the selected C edition and implementation. Check the target’s headers and documentation instead of assuming a desktop libc is representative.

## Hosted And Freestanding

Hosted implementations provide a defined program startup and the full standard library. Freestanding implementations are allowed a smaller environment, including a limited set of headers and functions. The implementation may provide additional facilities, but application code must not assume hosted services exist in early boot or bare-metal firmware.

A freestanding project often supplies:

- startup and reset code;
- a minimal memcpy, memset, and integer runtime;
- system-call stubs or retargeting hooks;
- a linker script and section initialization;
- a selected allocator or no allocator;
- UART or semihosting diagnostics;
- board and interrupt support.

The C language remains C even when main, files, processes, and a normal exit path do not exist.

## Version And Feature Selection

Select the language edition explicitly:

~~~sh
cc -std=c17 -Wall -Wextra -Wpedantic -c module.c
~~~

Do not use a header’s presence as the only compatibility test. Check:

- compiler language mode;
- libc version;
- target feature macros;
- optional type or function definitions;
- link libraries and startup objects;
- ABI and syscall support.

Use a portability header for project policy:

~~~c
#ifndef PROJECT_C_FEATURES_H
#define PROJECT_C_FEATURES_H

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
#define PROJECT_HAS_C11 1
#else
#define PROJECT_HAS_C11 0
#endif

#endif
~~~

Compiler predefined macros describe the implementation, not a guarantee that every related library feature is present.

## Library Contracts

Read each facility’s contract for:

- valid argument ranges and null rules;
- object lifetime and required storage;
- return values and error indicators;
- allocation and release behavior;
- thread safety and reentrancy;
- locale or global state;
- blocking and cancellation;
- signal or interrupt restrictions;
- undefined behavior on violations.

A function can be standard and still be wrong for an ISR, bootloader, safety path, or hard real-time loop.

## Hosted-Only Assumptions

Common assumptions that must be challenged in firmware include:

- stderr exists and is connected to a console;
- file streams are available;
- malloc never fails;
- time returns wall-clock time;
- environment variables exist;
- locale data is initialized;
- threads.h maps to the project RTOS;
- signals interrupt execution in a useful way;
- exit performs a meaningful shutdown;
- floating-point and formatted I/O are cheap.

Wrap such facilities behind project interfaces when multiple environments are supported.

## Portability Layers

A useful structure is:

~~~c
struct diagnostic_port {
    int (*write)(void *context, const char *data, size_t length);
    void *context;
};

int diagnostic_write(const struct diagnostic_port *port,
                     const char *data, size_t length);
~~~

The policy module depends on diagnostic_port. The host, RTOS, and bare-metal adapters decide whether the implementation uses write, fwrite, a queue, or a UART.

Do not put POSIX or vendor types in a portable layer unless that layer is explicitly platform-specific.

## Exercises

1. Categorize every header used by a small firmware module as ISO C, POSIX, RTOS, vendor, or project-local.
2. Build a freestanding-style test configuration with a restricted library and identify missing assumptions.
3. Create a portability header for C version, fixed-width types, atomics, and compiler diagnostics.
4. Measure the linked-image change from adding formatted I/O, floating-point math, and dynamic allocation.
5. Write a project wrapper for diagnostic output and provide host and target adapters.
6. Review a library call for null, bounds, lifetime, blocking, allocation, and error behavior.

## Common Mistakes

- Calling a POSIX function and describing it as ISO C.
- Assuming a header’s presence proves all functions are implemented.
- Using hosted I/O or exit in freestanding startup code.
- Ignoring the selected C version and feature macros.
- Treating standard library functions as ISR-safe by default.
- Forgetting that locale and global library state affect behavior.
- Assuming libc implementation details are ABI-portable.
- Linking a large facility without measuring flash, RAM, stack, and latency.

## Debugging Checklist

1. Identify the standard or implementation that defines the symbol.
2. Inspect the selected language mode and include search paths.
3. Check target headers and link map for the actual implementation.
4. Read preconditions, error, allocation, and context rules.
5. Measure code size, static RAM, stack, heap, and timing.
6. Test unavailable, failing, interrupted, and boundary cases.
7. Replace nonportable calls with an injected port where needed.
8. Record the library and toolchain versions in the build manifest.

## Related Topics

- [Standard Library And Ecosystem overview](./index.md)
- [Embedded libc Implementations](./embedded-libc.md)
- [POSIX And System Interfaces](./posix-and-system-interfaces.md)
- [Compilation, Linking, And ABI](../compilation-linking-and-abi/index.md)
- [Language Fundamentals](../language-fundamentals/index.md)
- [C Programming](../index.md)

## References

- [ISO/IEC 9899 standards and drafts](https://open-std.org/jtc1/sc22/wg14/www/standards.html)
- [C11 public draft N1570](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n1570.pdf)
- [GCC C dialect options](https://gcc.gnu.org/onlinedocs/gcc/C-Dialect-Options.html)
- [POSIX.1-2024 base definitions](https://pubs.opengroup.org/onlinepubs/9799919799/basedefs/V1_chap01.html)

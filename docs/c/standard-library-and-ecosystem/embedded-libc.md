---
status: draft
reviewed: false
domain: c
difficulty: intermediate
last_reviewed: null
---

# Embedded libc Implementations

An embedded libc supplies some or all of the C library in a runtime with limited memory, no filesystem, custom startup, and target-specific I/O. Choosing a libc is a system decision involving compiler ABI, linker scripts, syscall retargeting, reentrancy, allocation, code size, licensing, and support.

## Learning Objectives

- Compare the roles of newlib, picolibc, musl, glibc, and vendor libraries.
- Understand syscall stubs and retargeting.
- Recognize reentrancy and libc-lock requirements.
- Measure printf, scanf, math, locale, and malloc footprint.
- Choose semihosting, UART, host syscalls, or no I/O deliberately.
- Verify startup, heap, stack, and thread integration.

## Implementation Families

| Implementation | Typical environment | Design concern |
| --- | --- | --- |
| glibc | GNU/Linux hosted processes | Broad features, dynamic integration, larger runtime |
| musl | Linux and static/minimal systems | Small, standards-focused, static-linking suitability |
| newlib | Cross-compiled and embedded systems | Retargeting, reentrancy, heap and syscall hooks |
| newlib-nano | Reduced newlib configuration | Footprint tradeoffs and feature limitations |
| picolibc | Resource-constrained embedded targets | Size, modern toolchains, configurable stdio and allocation |
| Vendor libc | MCU SDK or proprietary toolchain | ABI, support, feature and documentation boundaries |

The table is a starting point, not a benchmark. Check the exact build, target, version, and configuration.

## Syscall Retargeting

An embedded libc may call project-provided low-level functions for I/O and process behavior:

~~~c
#include <stddef.h>
#include <sys/types.h>

ssize_t _write(int file, const void *buffer, size_t length)
{
    const unsigned char *bytes = buffer;
    (void)file;

    for (size_t i = 0u; i < length; ++i) {
        if (uart_putc(bytes[i]) != 0) {
            return -1;
        }
    }

    return (ssize_t)length;
}
~~~

Names and signatures vary by toolchain. Some environments use write, _write, __write, fputc hooks, or a board-specific port. Verify the linker map to see which symbol the libc actually references.

Retargeting also needs policies for read, close, lseek, fstat, isatty, exit, abort, sbrk, and time where the selected library expects them. Returning plausible success from an unsupported stub can be worse than returning a clear error.

## Heap Integration

A libc allocator needs a defined memory source:

~~~c
#include <stddef.h>
#include <stdint.h>

extern unsigned char __heap_start__;
extern unsigned char __heap_end__;

static unsigned char *heap_cursor = &__heap_start__;

void *simple_sbrk(ptrdiff_t increment)
{
    unsigned char *next = heap_cursor + increment;
    if (next < &__heap_start__ || next > &__heap_end__) {
        return (void *)-1;
    }

    unsigned char *previous = heap_cursor;
    heap_cursor = next;
    return previous;
}
~~~

This is illustrative only. A real allocator needs alignment, overflow checks, concurrency protection, stack collision detection, and a linker-defined contract. Do not implement sbrk by pointer arithmetic across unrelated objects without the implementation’s supported model.

Many embedded products avoid a general libc heap and provide a project allocator, pool, or arena instead.

## Reentrancy And Locks

Library calls can share global state such as errno, locale, stdio buffers, malloc metadata, and random state. A libc may provide a reentrancy structure or require hooks for allocator locking.

Newlib, for example, documents reentrant variants and malloc lock hooks. The project must integrate those with task context and interrupt restrictions. A library that is reentrant is not automatically safe from an ISR or free of blocking.

Check:

- whether errno is per-thread;
- whether stdio streams are locked;
- whether malloc is protected;
- whether locale is global;
- whether the library uses TLS;
- whether calls can invoke syscalls;
- whether callbacks or hooks re-enter the library.

## printf Footprint

Formatted I/O is configurable but can be expensive:

- integer formatting;
- floating-point formatting;
- scanning;
- locale and wide-character support;
- stream buffering;
- locking;
- heap allocation;
- syscall retargeting.

Measure variants rather than guessing:

~~~sh
arm-none-eabi-size firmware.elf
arm-none-eabi-nm -S --size-sort firmware.elf | tail
arm-none-eabi-objdump -h firmware.elf
~~~

A project logger with fixed-format event records may provide better bounded behavior than general printf. If printf is retained for diagnostics, disable unused features and keep it out of time-critical paths.

## Semihosting

Semihosting routes target I/O through a debugger. It is useful during early development but usually has unbounded latency and may fault or block when no debugger is attached.

Never leave semihosting on a production real-time or field-recovery path without an explicit product requirement and watchdog policy. Provide a compile-time or link-time way to remove it.

## Startup And Termination

The libc may expect startup code to:

- initialize stack and vector state;
- copy initialized data;
- clear bss;
- initialize constructors or runtime tables where applicable;
- set up heap and reentrancy;
- call main or the freestanding entry;
- route exit and abort behavior.

A microcontroller reset handler may never return from main. Define what a returned main means, whether interrupts and clocks are configured, and how fatal termination records evidence.

## Selecting A libc

Evaluate:

- required C edition and headers;
- compiler and ABI compatibility;
- code and data footprint;
- heap behavior and locking;
- reentrancy and TLS;
- floating-point and locale support;
- filesystem and syscall assumptions;
- diagnostics and debugger integration;
- licensing and maintenance;
- tests on the exact target and optimization profile.

Do not switch libc only because a hello-world image is smaller. Measure the functions used by the product and verify corner-case behavior.

## Exercises

1. Inspect a target map and identify libc objects pulled in by printf, malloc, and math.
2. Trace one write call from application source to UART or host output.
3. Implement and test explicit unsupported-syscall failures.
4. Compare newlib, newlib-nano, or picolibc configurations on a representative image.
5. Test libc calls from multiple tasks and document required locks or reentrancy setup.
6. Run with and without semihosting and record latency and failure behavior.
7. Verify startup data, bss, heap, TLS, and termination integration.

## Common Mistakes

- Assuming a hosted libc and embedded libc have the same facilities and costs.
- Providing syscall stubs that claim success without implementing behavior.
- Ignoring allocator alignment, bounds, or concurrency.
- Calling non-reentrant libc functions from multiple tasks.
- Using printf or semihosting in real-time or fault paths.
- Assuming errno and stdio are ISR-safe.
- Measuring only a trivial program instead of the product image.
- Forgetting libc startup and termination requirements.
- Mixing incompatible compiler, libc, startup, and linker configurations.
- Treating newlib or picolibc defaults as universal target behavior.

## Debugging Checklist

1. Identify the exact libc, version, configuration, compiler, and ABI.
2. Inspect undefined and resolved symbols in the map and ELF.
3. Trace retargeted I/O and allocator calls.
4. Check reentrancy, TLS, locks, and interrupt context.
5. Measure code, data, stack, heap, and worst-case latency.
6. Test missing debugger, absent filesystem, allocation failure, and syscall errors.
7. Verify startup and fatal termination paths.
8. Record the configuration used to produce the measured image.

## Related Topics

- [Standard Library And Ecosystem overview](./index.md)
- [Standard Library Overview](./standard-library-overview.md)
- [I/O, Diagnostics, And Errors](./io-diagnostics-and-errors.md)
- [Memory Layout And Allocation](../semantics-and-memory/memory-layout-and-allocation.md)
- [Compilation, Linking, And ABI](../compilation-linking-and-abi/index.md)
- [Embedded C And Hardware](../embedded-c-and-hardware/index.md)
- [C Programming](../index.md)

## References

- [newlib C library documentation](https://sourceware.org/newlib/libc.html)
- [newlib documentation index](https://sourceware.org/newlib/docs.html)
- [picolibc project documentation](https://github.com/picolibc/picolibc)
- [musl libc](https://musl.libc.org/)
- [GNU C Library manual](https://sourceware.org/glibc/manual/)

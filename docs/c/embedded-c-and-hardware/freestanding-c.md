---
status: draft
reviewed: false
domain: c
difficulty: advanced
last_reviewed: null
---

# Freestanding C

A freestanding implementation is a C environment in which the full hosted execution environment is not required. This is the normal starting point for many microcontrollers, boot stages, kernels, hypervisors, and small firmware components. Freestanding does not mean “the compiler can do anything”; it means the available runtime, library, startup, and termination contracts are different and must be made explicit.

## Learning Objectives

- distinguish hosted and freestanding implementation obligations;
- define the minimum startup, compiler-runtime, and library support a product needs;
- avoid accidental dependencies on processes, filesystems, environment variables, and initialized libc state;
- integrate linker symbols, memory initialization, heap policy, and low-level I/O;
- decide which standard facilities are safe, unavailable, bounded, or replaced;
- test freestanding policy on a host without hiding target restrictions.

## What Changes In Freestanding C

In a hosted environment, the implementation provides a startup model and a broad standard library. In freestanding C, the implementation still translates C source, but the project may supply:

- reset entry and vector table;
- stack and memory initialization;
- `memcpy`, `memset`, integer helper, or atomic runtime support;
- a small libc, newlib/picolibc configuration, or no libc;
- system-call/retargeting hooks;
- linker script and memory symbols;
- the application entry and termination policy.

The language rules remain relevant, but an operation can be syntactically valid and still be unavailable or inappropriate because the target has no filesystem, scheduler, locale, heap, or implementation support for it.

## Startup Dependency Graph

Before calling a function, ask whether its dependencies are initialized:

```text
reset -> stack -> clocks/memory -> data/bss -> runtime/libc
      -> allocator/TLS/RTOS -> drivers -> application
```

For example, `printf` can depend on initialized streams, locks, reentrancy state, formatting tables, heap, floating-point support, and a retargeted output hook. A startup diagnostic should use a deliberately smaller path.

## Library Policy

Create a project table for library facilities:

| Facility | Decision to document |
| --- | --- |
| `memcpy`/`memset` | implementation, overlap rules, early-startup availability |
| allocation | forbidden, pool-only, or selected heap and lock policy |
| formatted I/O | forbidden, diagnostic-only, or bounded project logger |
| time | hardware counter, RTOS tick, RTC, or unavailable |
| threads | RTOS API, C threads, or unavailable |
| file I/O | no filesystem, block device layer, or POSIX adapter |
| atomics | instruction support, library helpers, ISR policy |
| floating point | hardware/software ABI, lazy context, timing budget |
| termination | reset, halt, watchdog, bootloader return, or task exit |

Do not expose a hosted implementation through a common header and assume it will work on bare metal. Put the policy behind an adapter and test unsupported operations explicitly.

## Compiler Runtime Support

Even a “no libc” image can need compiler runtime functions for:

- division and remainder of unsupported widths;
- 64-bit arithmetic on a 32-bit core;
- floating-point conversion;
- atomic operations not implemented in one instruction;
- stack checking or overflow traps;
- compiler built-ins and LTO support.

Inspect undefined and resolved symbols in the final ELF. If a runtime helper is required, provide a compatible implementation or link the correct compiler runtime. Do not write a replacement based only on a function name; verify calling convention, clobbers, corner cases, and licensing.

## Minimal Memory Helpers

A simple byte copy can be useful in early startup, but it must be constrained to non-overlapping regions and a valid memory system:

~~~c
#include <stddef.h>

void startup_copy(unsigned char *destination,
                  const unsigned char *source,
                  size_t length)
{
    for (size_t i = 0u; i < length; ++i) {
        destination[i] = source[i];
    }
}

void startup_clear(unsigned char *destination, size_t length)
{
    for (size_t i = 0u; i < length; ++i) {
        destination[i] = 0u;
    }
}
~~~

This is not a general replacement for `memcpy` or `memset`. It does not handle overlap, MMIO side effects, cache maintenance, alignment optimization, or fault recovery. Use it only where the linker and startup contract prove the ranges valid.

## Linker-Provided Boundaries

Linker symbols commonly describe data, bss, heap, stack, retention, or image regions:

~~~c
#include <stddef.h>

extern unsigned char __heap_start__[];
extern unsigned char __heap_end__[];

size_t heap_capacity(void)
{
    return (size_t)(__heap_end__ - __heap_start__);
}
~~~

The symbol declarations must match the linker script and address space. A capacity calculation does not make the region available: memory controllers, MPU permissions, cache policy, and other owners must be initialized first.

## Allocation Choices

Choose deliberately among:

- no dynamic allocation;
- fixed object pools;
- region/arena allocation with reset points;
- bounded RTOS heaps;
- general-purpose heap with measured fragmentation and locking;
- external memory allocator with DMA/cache rules.

For safety and real-time paths, bounded pools and arenas are often easier to analyze. If a general heap is used, define failure behavior, alignment, ownership, maximum allocation, concurrency, fragmentation monitoring, and what contexts may call it.

## Low-Level I/O And Retargeting

An output adapter should express partial progress and failure:

~~~c
#include <stddef.h>

int board_write(const unsigned char *data, size_t length, size_t *written)
{
    if (data == NULL || written == NULL) {
        return -1;
    }
    *written = 0u;
    while (*written < length) {
        if (uart_try_put(data[*written]) != 0) {
            break;
        }
        ++*written;
    }
    return *written == length ? 0 : -2;
}
~~~

The real implementation must define whether `uart_try_put` blocks, touches MMIO, is ISR-safe, and requires a clock or pin configuration. A libc `_write` hook may have a different contract; adapt it rather than pretending the names are universal.

## Unsupported Operations

Fail clearly when the target cannot provide a facility. A stub that returns success for `close`, `lseek`, or `isatty` can cause higher-level code to believe data was persisted or a device is interactive. For fatal unsupported paths, record a reason and enter the product’s safe termination policy.

## Host Simulation

Host tests should preserve freestanding restrictions:

- compile with a restricted portability header;
- forbid accidental POSIX and libc calls with include or link checks;
- provide fake clocks and deterministic allocators;
- model MMIO as an explicit object with access hooks;
- test error paths and resource exhaustion;
- keep target adapters separate from policy code.

Do not make the host fake more capable than the target and then call the host result a freestanding proof.

## Exercises

1. Build a no-startup object and list every compiler runtime dependency it creates.
2. Implement and test data-copy/bss-clear helpers with empty and boundary ranges.
3. Define a linker heap/stack contract and add collision assertions.
4. Compare no-heap, pool, arena, and general-heap behavior under failure.
5. Replace a libc output hook with a bounded UART adapter and test partial progress.
6. Make unsupported file/time/locale operations fail visibly in a freestanding configuration.
7. Build a host test that uses only the project’s freestanding portability layer.

## Common Mistakes

- assuming freestanding means no ABI or runtime obligations;
- calling libc before startup state exists;
- forgetting compiler helper libraries;
- providing successful stubs for unsupported operations;
- treating linker symbols as allocated C objects;
- using a general heap in an analyzed path without fragmentation or locking evidence;
- using startup copy loops for overlapping or MMIO regions;
- allowing host tests to use services the target does not have;
- calling low-level I/O from an ISR without a bounded contract.

## Related Topics

- [Standard Library And Ecosystem](../standard-library-and-ecosystem/index.md)
- [Startup, Runtime, And `main`](../compilation-linking-and-abi/startup-runtime-and-main.md)
- [Linker Scripts And Memory Layout](../compilation-linking-and-abi/linker-scripts-and-memory-layout.md)
- [Memory Layout And Allocation](../semantics-and-memory/memory-layout-and-allocation.md)
- [Embedded C And Hardware overview](./index.md)

## References

- [C11 public draft N1570, hosted and freestanding environments](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n1570.pdf)
- [newlib C library documentation](https://sourceware.org/newlib/libc.html)
- [picolibc project](https://github.com/picolibc/picolibc)
- [GCC link options](https://gcc.gnu.org/onlinedocs/gcc/Link-Options.html)

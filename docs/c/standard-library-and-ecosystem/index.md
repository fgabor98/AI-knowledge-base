---
status: draft
reviewed: false
domain: c
difficulty: intermediate
last_reviewed: null
---

# Standard Library And Ecosystem

The C library is a family of contracts rather than one universal binary. ISO C defines language-level headers and functions; a hosted libc implements them with an operating system or runtime; a freestanding implementation may provide only a subset; POSIX and RTOS libraries add platform interfaces; vendor libraries add hardware behavior.

This chapter teaches how to choose and use those layers without confusing a portable C facility with a Linux, RTOS, compiler, or vendor API.

## What This Chapter Teaches

For every library call, identify:

1. Which standard or platform defines it?
2. What headers, feature macros, link libraries, and startup support are required?
3. What are the valid inputs, ownership, lifetime, and thread-safety rules?
4. Can it allocate, block, access global state, use floating point, or touch the filesystem?
5. What does the embedded implementation actually include and cost?
6. What happens when the operation fails or is interrupted?

## Recommended Progression

1. [Standard Library Overview](./standard-library-overview.md) — map ISO C headers and hosted/freestanding boundaries.
2. [Memory And String APIs](./memory-and-string-apis.md) — use byte and text functions with explicit bounds.
3. [I/O, Diagnostics, And Errors](./io-diagnostics-and-errors.md) — understand streams, formatting, logging, and error channels.
4. [Numeric, Time, And Character APIs](./numeric-time-and-character-apis.md) — account for precision, locale, time bases, and code size.
5. [Atomics, Threads, And Signals](./atomics-threads-and-signals.md) — select C concurrency facilities and know their target limits.
6. [Embedded libc Implementations](./embedded-libc.md) — understand retargeting, reentrancy, syscalls, and footprint.
7. [POSIX And System Interfaces](./posix-and-system-interfaces.md) — use file descriptors, sockets, mmap, and Linux interfaces accurately.
8. [Common Embedded Libraries](./common-embedded-libraries.md) — evaluate HALs, RTOSes, protocol stacks, and support libraries.

## Library Boundaries

| Layer | Examples | Contract source |
| --- | --- | --- |
| ISO C | stdint.h, memcpy, fopen, strtod, thrd_create | Selected ISO C edition and implementation |
| C implementation | newlib, picolibc, musl, glibc, vendor libc | Library documentation, ABI, build options |
| POSIX | open, read, poll, mmap, pthreads, sockets | POSIX version and operating system |
| RTOS | task, queue, semaphore, timer APIs | RTOS documentation and port |
| Vendor/BSP | register HAL, CMSIS, SDK drivers | Chip and board documentation |
| Third-party | lwIP, mbed TLS, USB, filesystem stacks | Project version, configuration, license, API docs |

A symbol’s familiar spelling does not establish portability. For example, printf is in ISO C, open is POSIX, a vendor GPIO function belongs to a platform SDK, and a FreeRTOS queue API is neither ISO C nor POSIX.

## Choosing A Facility

Before adding a library call, check:

- correctness and exact contract;
- target availability and required startup support;
- worst-case latency;
- stack, static RAM, heap, and flash cost;
- locking and reentrancy;
- interrupt and DMA interaction;
- locale, floating-point, filesystem, or syscall dependencies;
- license and update policy;
- testability and fault injection;
- whether a smaller project-local abstraction is safer.

Do not replace a standard function by hand for performance until a target measurement shows the need. Do replace an unsuitable general facility when the embedded contract requires bounded storage, no allocation, no blocking, or deterministic formatting.

## Hosted, Freestanding, And Embedded Linux

A hosted environment provides the full required standard library and a defined startup model. A freestanding implementation has a smaller required library and implementation-defined startup and termination. Embedded Linux usually provides a hosted C environment inside a process, while kernel and early-boot code use different APIs and restrictions.

The same source module may have several adapters:

- a host implementation using stdio and POSIX;
- a bare-metal implementation using a UART and static buffers;
- an RTOS implementation using queues and tasks;
- a Linux implementation using file descriptors and epoll.

Keep the portable policy above those adapters.

## Footprint And Determinism

Library use can pull in more than the called symbol suggests:

- formatted I/O may add parsing, conversion, locking, and locale support;
- floating-point formatting may add large conversion code;
- malloc may add heap state and locks;
- time functions may require syscalls or timezone data;
- threads may require a runtime, TLS, and scheduler;
- locale and wide-character functions may add tables;
- filesystem functions may add buffering and file-system code.

Measure the linked image, map file, stack, and worst-case timing. Enable only the library features the product needs.

## A Practical Selection Record

For each nontrivial facility, record:

~~~text
Facility: formatted logging
Layer: project wrapper over target UART / host stderr
Allowed contexts: task and host test; not ISR
Allocates: no in normal path
Blocks: may wait for output queue
Failure: drops record and increments counter
Representation: ASCII subset, explicit newline policy
Tests: full queue, malformed format, concurrent producers
Target evidence: flash, RAM, stack, worst-case latency
~~~

This record turns an implicit library assumption into a reviewable engineering decision.

## Chapter Outcomes

After completing this chapter, you should be able to:

- distinguish ISO C, libc, POSIX, RTOS, vendor, and third-party APIs;
- choose a library facility based on contract and target cost;
- use memory, string, I/O, numeric, time, and character functions safely;
- explain why errno, streams, locale, and TLS may differ on embedded targets;
- use C atomics without confusing them with RTOS synchronization;
- retarget an embedded libc deliberately;
- assess an external embedded library for API, footprint, ownership, licensing, and maintenance;
- isolate platform facilities behind stable project interfaces.

## Running Exercise

Build a portable diagnostic service with multiple backends:

1. A host backend writes to stderr.
2. A bare-metal backend writes bounded records to a UART queue.
3. An RTOS backend uses a task-safe queue.
4. A Linux backend writes to a file descriptor.
5. All backends share severity, timestamp, truncation, and failure semantics.
6. Measure the cost and context restrictions of each implementation.

## Related Topics

- [Language Fundamentals](../language-fundamentals/index.md)
- [Semantics And Memory](../semantics-and-memory/index.md)
- [Modular Design And APIs](../modular-design-and-apis/index.md)
- [Compilation, Linking, And ABI](../compilation-linking-and-abi/index.md)
- [Correctness, Quality, And Security](../correctness-quality-and-security/index.md)
- [Embedded C And Hardware](../embedded-c-and-hardware/index.md)
- [C Programming](../index.md)
- [Topic Map](../../topic-map.md)

## References

- [ISO/IEC 9899 standards and drafts — WG14](https://open-std.org/jtc1/sc22/wg14/www/standards.html)
- [C11 public draft N1570](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n1570.pdf)
- [POSIX.1-2024 base definitions](https://pubs.opengroup.org/onlinepubs/9799919799/basedefs/V1_chap01.html)
- [GCC C library and runtime options](https://gcc.gnu.org/onlinedocs/gcc/Link-Options.html)

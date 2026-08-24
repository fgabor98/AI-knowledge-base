---
status: draft
reviewed: false
domain: c
difficulty: beginner
last_reviewed: null
---

# Use Cases And Environments

C is not one kind of program. The same language is used in environments with radically different startup, memory, scheduling, privilege, library, and failure models.

Learning C for embedded work means learning to recognize the environment before choosing an API, storage strategy, synchronization method, or debugging technique.

## The Environment Dimensions

When entering an unfamiliar C project, identify these dimensions first:

| Dimension | Questions |
|---|---|
| Startup | Does execution begin at main, a reset handler, a bootloader entry point, or a kernel-specific entry point? |
| Privilege | Does code run in a user process, kernel mode, secure world, machine mode, or an interrupt handler? |
| Runtime | Is there a full libc, a small libc, an RTOS, a custom runtime, or no runtime? |
| Memory | Is virtual memory present? Is allocation allowed? Are memory regions cacheable or DMA-visible? |
| Concurrency | Are there processes, threads, tasks, interrupts, multiple cores, or no concurrency? |
| I/O | Does code use files and sockets, memory-mapped registers, queues, DMA, or direct buses? |
| Failure | Can the component return an error, restart, reset the board, enter recovery, or fail safe? |
| Timing | Are operations best-effort, latency-sensitive, deadline-bound, or safety-critical? |
| Toolchain | Which C dialect, ABI, extensions, linker, and debug tools are supported? |
| Verification | Are tests host-based, target-based, hardware-in-the-loop, static, formal, or certified? |

The correct C design depends on these answers more than on whether the source file is named .c.

## Environment Matrix

| Environment | Typical startup | Typical runtime | Main constraints | Common C responsibilities |
|---|---|---|---|---|
| Hosted desktop/server | runtime invokes main | full libc and OS | portability, security, performance, concurrency | applications, libraries, services, tools |
| Bare-metal MCU | reset handler or bootloader | freestanding support, vendor SDK | memory, interrupts, deterministic timing, power | startup, drivers, control loops, protocol handling |
| RTOS application | board startup then scheduler | RTOS kernel plus selected libc | task priorities, ISR context, bounded memory, latency | tasks, queues, drivers, state machines |
| Bootloader | reset or previous boot stage | minimal runtime | recovery, image validation, early hardware | storage, transport, authentication, handoff |
| Linux kernel | architecture entry and kernel init | custom kernel runtime | privilege, concurrency, no normal libc, DMA, preemption | drivers, subsystems, hardware control |
| Embedded Linux userspace | process loader invokes main | libc, POSIX, kernel services | storage, process recovery, IPC, deployment | services, hardware-facing daemons, tools |
| Protocol stack | host, RTOS, or firmware startup | environment-dependent | bounds, framing, endian, timeouts, malformed input | parsers, serializers, state machines |
| Safety-critical system | controlled startup | restricted and qualified runtime | determinism, traceability, evidence, failure response | safety functions, monitoring, diagnostics |
| C library or runtime | implementation startup | implementation-defined | ABI, portability, reentrancy, size, correctness | startup, allocation, I/O, string and numeric primitives |

The table is a classification tool, not a set of strict categories. A product can combine several environments: a bootloader, a secure monitor, an RTOS application, and a Linux userspace service may all use C on one device.

## Bare-Metal Firmware

Bare-metal firmware executes without a general-purpose operating system. Typical responsibilities include:

- reset handling
- vector-table setup
- clock and power initialization
- memory initialization
- GPIO and peripheral configuration
- interrupt handlers
- polling loops
- DMA setup
- watchdog servicing
- nonvolatile storage
- communication protocols
- fault handling
- bootloader handoff

The program may still use a vendor-provided C library or a small runtime, but it cannot assume:

- a process
- a filesystem
- a scheduler
- a shell
- a normal standard input or output stream
- a working heap
- a fully initialized C environment before startup code runs

Bare-metal designs should make resource ownership and timing visible. A function that is harmless in a desktop process may be inappropriate in an interrupt handler because it allocates, blocks, takes an unbounded lock, or accesses a cache-incoherent buffer.

## RTOS Applications

An RTOS introduces scheduling and synchronization, but it does not make the system equivalent to desktop Linux.

Important distinctions include:

- task context versus interrupt context
- scheduler state
- priority and preemption
- blocking and timeout behavior
- static versus dynamic task objects
- stack allocation and stack high-water marks
- ISR-safe API variants
- priority inversion
- critical sections
- tick and tickless timing
- queue ownership
- watchdog deadlines

A queue, semaphore, or task API is not part of ISO C. It is an RTOS contract. FreeRTOS documentation, for example, distinguishes task and interrupt API usage and provides separate documentation for tasks, queues, mutexes, semaphores, memory, and stack protection. [FreeRTOS kernel developer documentation](https://www.freertos.org/Documentation/02-Kernel/02-Kernel-features/00-Developer-docs)

When learning RTOS C, always record:

- which calls may block
- which calls may be made from an ISR
- who owns each buffer
- what happens on timeout
- which priority executes the callback
- whether allocation occurs
- how failure is reported

## Bootloaders

A bootloader is a C program with unusually strict sequencing and recovery requirements. It may need to:

- initialize only enough hardware to read an image
- communicate over a recovery interface
- validate image metadata
- verify signatures or hashes
- protect rollback counters
- handle interrupted updates
- select among images
- configure memory and caches
- pass arguments to the next stage
- preserve diagnostic evidence

Bootloader code often runs before the normal application runtime and may use a deliberately small library subset. It must not assume that the application has initialized clocks, caches, memory controllers, storage, or security state.

A bootloader is also a contract boundary. The handoff should document:

- register and memory arguments
- image entry address
- stack requirements
- memory ownership
- cache and MMU state
- device-tree or metadata pointers
- security state
- reset and watchdog state

## Device Drivers

“Driver” can mean different things:

- a bare-metal peripheral driver
- an RTOS device abstraction
- a Linux kernel driver
- an embedded Linux userspace service using a kernel interface
- a vendor HAL module
- a middleware adapter

The language is C in many of these cases, but the surrounding contracts differ.

A driver usually needs to define:

- device ownership
- initialization and shutdown
- register access
- interrupt handling
- DMA buffers
- concurrency
- error recovery
- power management
- timeout behavior
- reset behavior
- user or middleware API
- observability

For Linux kernel code, C is used in a freestanding kernel environment with GNU C extensions and kernel-specific APIs. The Linux documentation explicitly describes the kernel as typically compiled as GNU C11 and notes that it does not rely on a normal standard C library. [Linux kernel programming language documentation](https://www.kernel.org/doc/html/latest/process/programming-language.html)

For Cortex-M software, CMSIS provides standardized processor, peripheral, driver, RTOS, and tooling interfaces intended to improve reuse across vendors and components. It does not eliminate device-specific knowledge; it organizes some of the boundaries. [CMSIS overview](https://arm-software.github.io/CMSIS_5/General/html/index.html)

## Embedded Linux Userspace

An embedded Linux userspace program is generally hosted from the perspective of ISO C: a process loader starts the program, libc is available, and the program can use operating-system services.

It is still embedded software because it may face:

- read-only or small filesystems
- limited storage
- long uptime
- power loss
- watchdog resets
- asynchronous device removal
- constrained CPU and memory
- cross-compilation
- vendor kernel behavior
- update and rollback requirements
- field diagnostics requirements

Typical C components include:

- hardware-facing daemons
- command-line diagnostics
- IPC services
- networking services
- serial and GPIO tools
- update agents
- manufacturing utilities
- media and sensor pipelines
- kernel-facing libraries

POSIX is separate from ISO C. POSIX defines an operating-system interface and environment, including C-language system interfaces, process behavior, files, signals, threads, and utilities. [POSIX.1-2024 introduction](https://pubs.opengroup.org/onlinepubs/9799919799/basedefs/V1_chap01.html)

## Protocol Stacks

Protocol code can run in any of the environments above. Its defining constraints are usually data and timing rather than startup.

A robust C protocol component should make explicit:

- byte order
- field widths
- framing
- length validation
- maximum message size
- partial input
- malformed input
- versioning
- timeouts
- retransmission
- checksum or authentication
- buffer ownership
- allocation policy
- parser state
- error recovery

Do not map an external wire format directly onto a C structure without checking:

- padding
- alignment
- endian representation
- integer width
- bit-field layout
- compiler packing rules
- untrusted length fields

Protocol parsing is a particularly good place to combine host-based tests, fuzzing, sanitizers, and target integration tests.

## Safety-Critical And Security-Critical Systems

Safety and security concerns overlap but are not identical.

Safety-oriented C emphasizes:

- predictable behavior
- bounded resource use
- failure containment
- traceability
- coding rules
- static analysis
- test evidence
- justified deviations
- deterministic timing
- safe-state transitions

Security-oriented C emphasizes:

- hostile input
- privilege boundaries
- confidentiality
- integrity
- authentication
- memory-safety failures
- cryptographic correctness
- update authenticity
- attack-surface reduction
- recovery after compromise

Both domains need clear evidence, but the governing process and hazard model may differ. MISRA C, CERT C, ISO secure-coding guidance, and sector standards should be treated as project constraints layered on top of the language.

## C Interoperability

C is often the boundary language between components written in different languages. A C ABI commonly provides:

- stable function names
- simple value and pointer parameters
- explicit ownership conventions
- opaque handles
- predictable header declarations
- compatibility with C++, Rust, Python extensions, and vendor tools

A good C boundary avoids exposing unstable implementation details such as:

- compiler-private types
- C++ classes
- language-specific exceptions
- ownership rules that are not documented
- structures whose layout changes without a version field
- allocator mismatches
- callbacks whose lifetime is implicit

The C ABI is not automatically identical across all platforms. Calling conventions, structure layout, alignment, symbol naming, and error representation remain implementation and ABI concerns.

## Choosing The Right Environment For An Experiment

Use a hosted host build when you want to investigate:

- pure language semantics
- parsing
- data structures
- algorithms
- error handling
- protocol behavior
- memory safety
- compiler diagnostics
- sanitizer findings

Use a freestanding or target build when you need to investigate:

- startup
- linker placement
- interrupt entry
- register access
- cache and DMA behavior
- ABI details
- timing
- power
- hardware faults
- vendor extensions

Use both when possible. Host tests are fast and observable. Target tests are necessary when the behavior depends on hardware or the platform contract.

## Environment Discovery Checklist

Before changing unfamiliar C code, answer:

- What starts this code?
- Which context calls this function?
- Can it block?
- Can it allocate?
- Can it be interrupted?
- Can it run concurrently?
- Who owns the input and output buffers?
- What is the maximum input size?
- What does failure do?
- What standard library is actually linked?
- Which compiler and C dialect are selected?
- Which ABI and linker script are used?
- What hardware or operating-system document defines the external interface?
- How is the behavior tested in CI and on the target?

## Learning Exercises

### Exercise 1: Classify existing components

Choose one component from each category:

- command-line host utility
- embedded Linux daemon
- RTOS task
- bare-metal interrupt-driven driver
- bootloader function
- protocol parser

For each, complete the environment dimensions table.

### Exercise 2: Compare one API across environments

Compare “read a byte from a device” in:

- a hosted file-based test
- an embedded Linux device file
- an RTOS UART driver
- a bare-metal memory-mapped UART

Document:

- blocking behavior
- ownership
- error reporting
- timing
- initialization
- test strategy

### Exercise 3: Draw the boundary map

For a small embedded product, draw:

- boot ROM
- bootloader
- firmware
- RTOS or Linux kernel
- userspace
- hardware peripherals
- update mechanism
- diagnostic path

Mark which components use hosted C, freestanding C, POSIX, vendor APIs, or custom interfaces.

## Common Mistakes

- Treating embedded Linux userspace as if it had bare-metal startup constraints.
- Treating an RTOS as if it provided POSIX semantics.
- Calling a blocking API from an interrupt handler.
- Assuming a driver owns memory merely because it received a pointer.
- Treating a wire-format structure as a portable in-memory representation.
- Assuming that a full libc is available in a bootloader or kernel.
- Designing a hardware abstraction that hides essential timing or ownership rules.
- Testing only on the host when the behavior depends on cache, DMA, interrupts, or ABI.
- Testing only on hardware when the logic could be made faster and safer to test on the host.
- Mixing kernel APIs, POSIX APIs, RTOS APIs, and ISO C APIs without documenting the boundary.

## Related Topics

- [Hosted And Freestanding C](./hosted-and-freestanding.md)
- [Standard Library And Ecosystem](../standard-library-and-ecosystem/index.md)
- [Embedded C And Hardware](../embedded-c-and-hardware/index.md)
- [Embedded Linux](../../embedded-linux/index.md)
- [Linux Kernel Programming](../../linux-kernel/index.md)
- [Linux Userspace And System Programming](../../linux-userspace-and-system-programming/index.md)
- [Systems And Embedded Architecture](../../systems-and-embedded-architecture/index.md)

## References

- [POSIX.1-2024 introduction](https://pubs.opengroup.org/onlinepubs/9799919799/basedefs/V1_chap01.html)
- [Linux kernel programming language](https://www.kernel.org/doc/html/latest/process/programming-language.html)
- [Linux kernel coding style](https://www.kernel.org/doc/html/latest/process/coding-style.html)
- [CMSIS overview](https://arm-software.github.io/CMSIS_5/General/html/index.html)
- [FreeRTOS kernel developer documentation](https://www.freertos.org/Documentation/02-Kernel/02-Kernel-features/00-Developer-docs)
- [WG14 C language committee](https://open-std.org/jtc1/sc22/wg14/)

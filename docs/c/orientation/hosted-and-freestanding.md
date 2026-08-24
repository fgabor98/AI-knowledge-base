---
status: draft
reviewed: false
domain: c
difficulty: beginner
last_reviewed: null
---

# Hosted And Freestanding C

Hosted and freestanding describe the execution environment assumed by a C implementation. They are not labels for “desktop” and “embedded” alone.

A desktop application is normally hosted. A microcontroller firmware is normally freestanding. An RTOS application may be freestanding from the ISO C perspective even though it has a scheduler and many libraries. An embedded Linux userspace service is normally hosted. A Linux kernel is a freestanding program that uses a large custom runtime and many compiler extensions.

## Why The Distinction Matters

The environment determines whether you can assume:

- a program entry point named main
- a complete standard library
- file streams
- dynamic allocation
- process services
- threads
- a filesystem
- a scheduler
- normal program termination
- initialized hardware
- a working console
- a linker-provided runtime
- compiler-generated calls to helper functions

A source file can compile successfully in one environment and fail at link time or behave incorrectly in another because the surrounding runtime contract is different.

## Hosted Environment

A hosted environment provides the full standard library and a defined startup model for ordinary C programs.

The familiar model is:

1. The operating system or loader starts a process.
2. The implementation initializes its runtime.
3. The implementation invokes main.
4. The program returns from main or calls an exit function.
5. The runtime performs termination processing.

The exact mechanics are implementation-specific. The ISO C model describes the required language-level behavior, not the operating-system loader, ELF startup objects, process creation details, or shell.

Typical hosted facilities include:

- standard input, output, and error streams
- files
- dynamic memory
- formatted I/O
- time functions
- environment arguments
- locale support
- mathematical functions
- process and thread APIs supplied by the platform
- debugging and crash-dump integration supplied by the platform

Hosted does not mean every operating-system API is ISO C. POSIX, Windows, BSD, Linux, and vendor APIs remain separate contracts.

## Freestanding Environment

A freestanding environment is one in which the full hosted library and hosted startup model are not required.

The implementation may define:

- how execution starts
- whether main exists
- how termination works
- which library facilities are supplied
- how hardware and memory are initialized
- whether an operating system exists
- how interrupts and exceptions enter C code

Typical freestanding programs include:

- bootloaders
- operating-system kernels
- hypervisors
- microcontroller firmware
- early board initialization
- secure monitors
- firmware support libraries

Freestanding does not mean “no C library.” A project may provide a small or large library; it simply cannot assume that the hosted ISO C environment exists unless the project explicitly provides it.

## Required Library Support

The minimum required library facilities have changed across C revisions. Historically, freestanding implementations were required to provide a small set of headers such as:

- float.h
- limits.h
- stdarg.h
- stddef.h

Later revisions added additional facilities, including headers associated with Boolean values, fixed-width integers, alignment, and non-returning functions. C23 changes the freestanding requirements again.

Do not memorize one historical list and apply it to every target. Instead:

1. identify the selected C standard,
2. read the implementation's freestanding documentation,
3. inspect the supplied headers,
4. inspect the linker's available libraries,
5. test the exact target configuration.

The [WG14 C standards material](https://open-std.org/jtc1/sc22/wg14/www/standards.html) and the compiler documentation are the right starting points.

## Compiler Environment Flags

GCC exposes the intended environment through options such as:

- -fhosted
- -ffreestanding
- -fno-hosted
- -fno-freestanding

GCC documents -ffreestanding as asserting that the program targets a freestanding environment. It also implies -fno-builtin, which prevents the compiler from assuming the normal meanings of standard-library function names in the same way as a hosted compilation.

GCC documents -fhosted as asserting a hosted environment. It implies assumptions about the standard library and main.

These flags communicate compiler assumptions. They do not create:

- a startup file
- a linker script
- a libc
- a scheduler
- an interrupt vector
- a board memory map
- a device driver

Clang similarly notes that a freestanding build still needs a C standard library or equivalent support for the required interfaces when the program is linked. [Clang command guide](https://clang.llvm.org/docs/CommandGuide/clang.html)

## Detecting The Environment In Source

The standard predefined macro __STDC_HOSTED__ indicates the intended environment when it is provided:

- 1 indicates hosted
- 0 indicates freestanding

A small diagnostic probe can make this visible:

~~~c
#include <stdio.h>

int main(void)
{
#if defined(__STDC_HOSTED__) && __STDC_HOSTED__
    puts("hosted compilation");
#else
    puts("freestanding compilation");
#endif

    return 0;
}
~~~

This example itself assumes that stdio and main are available, so it is a hosted probe. A genuinely freestanding probe should not include stdio or assume that execution reaches main:

~~~c
#if defined(__STDC_HOSTED__) && __STDC_HOSTED__
#define C_ENVIRONMENT_HOSTED 1
#else
#define C_ENVIRONMENT_HOSTED 0
#endif

int environment_is_hosted(void)
{
    return C_ENVIRONMENT_HOSTED;
}
~~~

The macro is useful for conditional compilation, but it should not become a substitute for a project configuration contract. Large behavior differences hidden behind environment macros can make code difficult to test and review.

## Startup Is The Major Boundary

In hosted code, main is usually the first function most application developers see.

In freestanding code, startup commonly includes:

1. reset or boot entry
2. stack selection
3. processor mode setup
4. memory-controller or clock setup
5. copying initialized data
6. clearing zero-initialized data
7. setting vector tables
8. configuring security or privilege state
9. initializing the C runtime
10. calling platform initialization
11. starting the scheduler or application
12. entering an idle, dispatch, or fault path

Some of these steps are written in assembly, some in C, and some are supplied by the toolchain or vendor SDK.

The boundary must define when C objects are valid. For example:

- Is the stack initialized?
- Has the data section been copied from flash?
- Has the BSS region been cleared?
- Are clocks configured?
- Are caches enabled?
- Is the FPU available?
- Are interrupts masked?
- Is the heap initialized?
- Can constructors or registration tables be processed?
- Can the function call a library routine?

These are not stylistic questions. They determine whether the C execution model's preconditions are satisfied.

## Freestanding Does Not Mean No Runtime

A practical freestanding image may contain:

- startup assembly
- vector tables
- a linker script
- compiler runtime helpers
- integer division routines
- memcpy, memmove, memset, and memcmp
- a small printf-like logger
- an allocator
- a board-support package
- a vendor HAL
- an RTOS
- a filesystem
- a network stack
- a cryptographic library

For example, GCC may generate references to memory or arithmetic helper routines even when application source does not call them explicitly. The build must provide those symbols or configure the compiler to avoid the operations.

Inspect the final binary instead of guessing:

- read the linker map
- inspect undefined symbols
- inspect the symbol table
- disassemble the relevant function
- check which libraries were linked
- verify the startup object and linker script

## -ffreestanding Is Not -nostdlib

These options solve different problems.

- -ffreestanding tells the compiler about the language environment and disables assumptions associated with hosted library functions.
- -nostdlib tells the linker not to use the usual startup files and libraries.
- -nodefaultlibs changes default library selection but may still leave startup objects.
- -nostartfiles omits startup files but may retain libraries.

Exact behavior is compiler- and driver-specific. A freestanding project may still intentionally link a libc or compiler support library. Conversely, a hosted-looking source file can fail if all startup and libraries are removed.

Treat compiler-driver options as a toolchain contract. Verify them with verbose build output and binary inspection.

## RTOS And Freestanding C

An RTOS application commonly has:

- a reset handler
- board initialization
- a C runtime
- an RTOS scheduler
- tasks
- synchronization objects
- selected library facilities

From an engineering perspective, it may feel hosted because it has tasks and services. From the ISO C perspective, it may still be freestanding because:

- startup is not the hosted main model
- the full standard library is not present
- process and file abstractions are absent
- the RTOS provides non-standard interfaces
- termination may mean a reset or a permanently blocked task

Record the actual model instead of using “embedded” or “RTOS” as a substitute for a technical description.

## Testing Hosted And Freestanding Code

A useful project separates:

### Portable logic

- parsers
- state machines
- checksums
- data structures
- configuration validation
- numerical transformations
- error policy

Test this on a host with:

- sanitizers
- fuzzers
- coverage
- a debugger
- fast repeated execution

### Platform adapter

- register access
- interrupts
- DMA
- cache maintenance
- RTOS calls
- device files
- boot handoff
- target-specific timing

Test this with:

- target builds
- emulator or simulator where useful
- hardware-in-the-loop
- trace and register capture
- fault injection
- power-cycle and reset tests

The boundary should be explicit enough that portable logic can be tested without pretending that a host process is a microcontroller.

## A Minimal Freestanding Compilation Experiment

This command only compiles an object file; it does not promise that the object can be linked or executed:

~~~sh
cc -std=c17 -ffreestanding -fno-builtin -Wall -Wextra -c probe.c -o probe.o
~~~

For a complete target image, the project must additionally provide:

- startup code
- a linker script
- target headers
- compiler runtime support
- required memory functions
- the correct ABI options
- the target library or deliberate library substitutes
- a flashing and debug path

## Learning Exercises

### Exercise 1: Compare hosted and freestanding probes

Build the two probes above:

- once as a normal host program
- once as a freestanding object
- once with the actual embedded compiler if available

Record:

- predefined macros
- warnings
- generated symbols
- link requirements
- startup assumptions

### Exercise 2: Inspect hidden runtime dependencies

Write a function that performs:

- integer division
- structure copying
- a memory copy
- a formatted log call

Compile it for a host and an embedded target. Compare:

- assembly
- undefined symbols
- linked libraries
- code size
- generated helper calls

### Exercise 3: Document the startup contract

For one firmware project, document:

- reset entry
- stack location
- data and BSS initialization
- vector-table setup
- clock setup
- interrupt state
- heap initialization
- RTOS startup
- first application callback

## Common Mistakes

- Assuming every embedded program is freestanding in exactly the same way.
- Assuming freestanding means the standard library is completely absent.
- Assuming -ffreestanding supplies a runtime.
- Assuming -nostdlib is a complete freestanding configuration.
- Calling main before the C runtime and memory initialization are valid.
- Calling a library function because it compiled without checking whether it links and behaves on the target.
- Using standard I/O in early boot without providing a deliberate backend.
- Hiding platform-specific behavior behind an environment macro without tests.
- Treating an RTOS task as a process with POSIX semantics.
- Forgetting compiler-generated helper calls.
- Assuming volatile alone makes peripheral or shared-memory access correct.

## Related Topics

- [Standards And Conformance](./standards-and-conformance.md)
- [Use Cases And Environments](./use-cases-and-environments.md)
- [Freestanding C](../embedded-c-and-hardware/freestanding-c.md)
- [Startup, Reset, And Vector Tables](../embedded-c-and-hardware/startup-reset-and-vector-tables.md)
- [Startup, Runtime, And main](../compilation-linking-and-abi/startup-runtime-and-main.md)
- [Embedded libc Implementations](../standard-library-and-ecosystem/embedded-libc.md)

## References

- [ISO/IEC 9899:2024](https://www.iso.org/standard/82075.html)
- [WG14 approved standards and public drafts](https://open-std.org/jtc1/sc22/wg14/www/standards.html)
- [GCC C dialect options](https://gcc.gnu.org/onlinedocs/gcc/C-Dialect-Options.html)
- [GCC standards and hosted/freestanding environments](https://gcc.gnu.org/onlinedocs/gcc/Standards.html)
- [Clang command guide](https://clang.llvm.org/docs/CommandGuide/clang.html)
- [Linux kernel programming language](https://www.kernel.org/doc/html/latest/process/programming-language.html)

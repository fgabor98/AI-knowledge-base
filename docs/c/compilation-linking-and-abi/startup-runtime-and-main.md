---
status: draft
reviewed: false
domain: c
difficulty: advanced
last_reviewed: null
---

# Startup, Runtime, And `main`

The first C statement is not necessarily the first instruction executed. A hosted process begins with an operating-system loader and runtime entry; a microcontroller begins at a reset vector; an RTOS application may begin in a port-specific scheduler startup; and a bootloader may validate and hand off to an application. This page follows those paths into C and identifies what must be initialized before ordinary code is safe.

## Learning Objectives

- trace reset or process loading through startup code and the C runtime;
- understand stack setup, vector tables, data copying, bss clearing, and hardware initialization;
- distinguish what ISO C guarantees about `main` from implementation and platform behavior;
- integrate constructors, libc, TLS, heap, RTOS, and termination deliberately;
- diagnose faults that happen before or around `main`.

## Hosted Process Startup

A typical hosted executable has an ELF entry point that is not necessarily `main`. A platform startup object receives the initial machine state, establishes a stack and runtime conventions, processes arguments and environment data, and eventually calls a libc entry routine that initializes the library before calling `main`.

The exact names are implementation-specific, but the conceptual path is:

```text
OS loader -> ELF entry -> startup object -> libc initialization
          -> main(argc, argv, envp) -> exit/termination
```

The runtime may initialize:

- thread-local storage;
- relocations and shared-library dependencies;
- stdio, heap, locale, and environment state;
- constructor arrays for languages or toolchain extensions;
- atexit handlers and termination hooks.

Inspect rather than assume:

~~~sh
readelf -hW app | grep 'Entry point'
readelf -lW app | grep -A1 INTERP
nm -an app | head -n 40
objdump -d app | less
~~~

Do not call private runtime symbols from application code simply because they appear in a disassembly. They are platform and libc implementation details.

## Microcontroller Reset Path

A common Cortex-M-like reset sequence is:

1. The processor reads the initial stack pointer from the vector table.
2. It loads the reset handler address.
3. The reset handler establishes any early CPU state.
4. It initializes clocks, memory controllers, protection, or external memory as required.
5. It copies initialized data from its flash load address to its RAM run address.
6. It clears the bss region.
7. It performs platform, libc, C++ runtime, or RTOS initialization as configured.
8. It calls the application entry, often `main`.
9. It handles a returned entry according to the product policy, commonly by entering a safe idle loop.

The exact order is not universal. For example, a boot ROM may configure clocks, an RTOS may take ownership of `main`, and external RAM may not be available until board initialization. Document the order as a state machine and mark which C operations are valid in each state.

## Vector Table And Entry Symbols

A vector table is usually an array of addresses or target-defined records. A minimal C declaration might look like:

~~~c
#include <stdint.h>

typedef void (*isr_handler_t)(void);

extern unsigned char __stack_top__;
void Reset_Handler(void);
void Default_Handler(void);

__attribute__((section(".isr_vector"), used))
const uintptr_t vector_table[] = {
    (uintptr_t)&__stack_top__,
    (uintptr_t)&Reset_Handler,
    (uintptr_t)&Default_Handler,
};
~~~

This is target-specific. Some architectures encode mode bits, exception metadata, or a different entry representation. The compiler’s integer/pointer conversion, alignment, section retention, and linker placement must match the processor’s boot specification. Validate the programmed bytes, not only the C initializer.

## Data, BSS, And Static Initialization

Before ordinary C code uses an object with static storage duration:

- `.data` objects need their initial bytes copied to the runtime address;
- `.bss` objects need to be zeroed;
- any custom zero/init sections need their own documented policy;
- the stack must satisfy the ABI alignment requirement;
- the memory region must be powered, mapped, and accessible;
- caches and protection attributes must be configured consistently.

The copy and clear loops should use linker-provided boundaries and be tested for empty ranges, alignment, and overlap. Avoid calling a general libc `memcpy` before the selected implementation and memory system are ready unless that dependency is explicitly supported. A small startup loop is often easier to reason about.

## `main` And Termination

In a hosted implementation, `main` has defined forms and returning from it is equivalent to returning a status to the host through normal termination. In a freestanding implementation, the existence, signature, and caller of `main` are implementation/project choices. Embedded code should define what happens if it returns:

- enter a low-power idle loop;
- restart through a watchdog or software reset;
- report a fatal status and halt;
- return to a bootloader handoff;
- let an RTOS wrapper delete or suspend the task.

Do not let a returned `main` fall into an invalid address. A default handler should make the state observable and prevent uncontrolled execution.

## Constructors, Destructors, And Initialization Arrays

Toolchains can emit initialization and termination function tables, commonly represented by sections such as `.init_array` and `.fini_array`. Hosted runtimes usually process them. Bare-metal projects must decide whether to:

- call the toolchain’s runtime initialization entry;
- iterate the arrays in startup code;
- forbid such mechanisms in early firmware;
- use an explicit project registration system instead.

If a linker script discards these sections or startup never processes them, static objects with required initialization may appear to compile and link while never being initialized. Conversely, running constructors before clocks, heap, or hardware are ready can fail before the application has logging.

## Heap, TLS, And libc Runtime

Startup and runtime integration may include:

- heap boundaries and allocator locks;
- thread-local storage template and per-task blocks;
- reentrancy structures;
- `errno`, stdio, locale, and random-state initialization;
- compiler runtime support for division, floating point, atomics, or stack checks;
- system-call hooks.

An RTOS port must decide whether libc state is per task, protected by locks, or restricted to one context. Do not call allocation, formatted I/O, or TLS-dependent functions from reset or interrupt context without explicit support.

## Bootloader Handoff

Before jumping from a bootloader to an application, define:

- vector-table and application base address;
- stack pointer and ABI alignment;
- interrupt enable/pending state;
- cache, MPU/MMU, and memory-controller configuration;
- clock and peripheral ownership;
- watchdog state;
- image validation and rollback result;
- registers used to pass reset reason or boot metadata.

The application reset handler should not assume the bootloader left every peripheral in reset state. Either specify the handoff state or make the application reinitialize what it owns.

## Faults Before `main`

Common causes include:

- invalid vector-table address or alignment;
- wrong instruction-set state bit or entry encoding;
- stack placed outside usable RAM;
- data-copy source or destination outside mapped memory;
- bss bounds overlapping a reserved region;
- external RAM accessed before initialization;
- constructor or libc hook calling an unavailable service;
- C runtime expecting symbols or syscalls not supplied by the image;
- watchdog reset during slow initialization;
- incompatible image base or bootloader handoff state.

Use a minimal fault record in retention RAM or a debugger trace before enabling complex initialization. Record the reset reason, fault registers, stack pointer, program counter, linker build ID, and image version.

## Exercises

1. Draw the exact startup path for a hosted executable and a bare-metal board.
2. Inspect a vector table and verify its first entries against the linker map.
3. Implement data-copy and bss-clear loops in a simulator or host model with empty and non-empty ranges.
4. Return from an embedded `main` deliberately and verify the documented policy.
5. Enable and disable initialization arrays and observe the startup behavior.
6. Add a pre-`main` fault record and recover it after a watchdog reset.
7. Define a bootloader handoff document and test every invariant at the application reset entry.

## Common Mistakes

- assuming `main` is the reset entry point;
- calling libc before its startup contract is complete;
- forgetting the `.data` copy or `.bss` clear;
- using linker symbols without verifying the actual memory map;
- discarding constructor or registration sections;
- treating returned `main` as impossible and leaving execution uncontrolled;
- inheriting undocumented bootloader peripheral or interrupt state;
- testing only a warm reset when the product also performs cold, watchdog, and brownout resets;
- diagnosing a pre-`main` fault with application logging that is not initialized yet.

## Related Topics

- [Linker Scripts And Memory Layout](./linker-scripts-and-memory-layout.md)
- [Cross-Compilation And Sysroots](./cross-compilation-and-sysroots.md)
- [Freestanding C](../embedded-c-and-hardware/freestanding-c.md)
- [Startup, Reset, And Vector Tables](../embedded-c-and-hardware/startup-reset-and-vector-tables.md)
- [Bootloaders And Firmware Images](../embedded-c-and-hardware/bootloaders-and-firmware-images.md)

## References

- [GCC link options](https://gcc.gnu.org/onlinedocs/gcc/Link-Options.html)
- [GNU ld entry point and linker scripts](https://sourceware.org/binutils/docs/ld/Entry-Point.html)
- [GNU ld section and memory commands](https://sourceware.org/binutils/docs/ld/Basic-Script-Concepts.html)
- [Arm Application Binary Interface repository](https://github.com/ARM-software/abi-aa)

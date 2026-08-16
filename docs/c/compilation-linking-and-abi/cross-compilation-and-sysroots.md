---
status: draft
reviewed: false
domain: c
difficulty: advanced
last_reviewed: null
---

# Cross-Compilation And Sysroots

Cross-compilation means the build machine and the execution target differ. Embedded work depends on this distinction because a host compiler, host headers, host libc, target ABI, board SDK, and firmware linker script can all be present on one workstation while only one consistent combination is valid for a given image.

## Learning Objectives

- distinguish build, host, and target roles;
- read target triples and select a matching compiler, assembler, linker, libc, and runtime;
- use sysroots and multilibs without accidentally including host files;
- choose CPU, ISA, endianness, floating-point, and ABI options consistently;
- separate target-independent tests from target-dependent integration;
- make cross-builds reproducible, inspectable, and safe in CI.

## Build, Host, And Target

The terms are relative to the tool being built:

| Term | Meaning in a common firmware build |
| --- | --- |
| build machine | machine running the compiler, often x86-64 Linux |
| host | machine on which a generated tool runs; relevant when building a compiler or generator |
| target | CPU/system on which the C program runs, such as Cortex-M or AArch64 Linux |

For an ordinary cross-compiled application, the build and host may be the same workstation while the target is the board. When building a compiler, all three can differ. Write them down in toolchain documentation; vague use of “host” causes wrong sysroot and emulator decisions.

## Target Triples

A target triple commonly encodes architecture, vendor, operating system, and environment, for example:

~~~text
arm-none-eabi
aarch64-linux-gnu
riscv64-unknown-elf
x86_64-linux-gnu
~~~

Triples are naming conventions, not a complete ABI specification. They may omit CPU revision, floating-point ABI, endianness, libc, or board. Confirm with the compiler:

~~~sh
arm-none-eabi-gcc -dumpmachine
arm-none-eabi-gcc -v
arm-none-eabi-gcc -print-sysroot
arm-none-eabi-gcc -print-multi-lib
arm-none-eabi-gcc -print-search-dirs
~~~

The driver’s reported target, selected specs, compiler runtime, headers, and libraries must agree. A triple that looks right can still select the wrong multilib or vendor SDK.

## Sysroots

A sysroot is a logical target filesystem root used for target headers, libraries, startup objects, and related files. With GCC-family drivers:

~~~sh
aarch64-linux-gnu-gcc --sysroot=/opt/sdk/sysroot \
    -Iproject/include -c app.c -o app.o
aarch64-linux-gnu-gcc --sysroot=/opt/sdk/sysroot \
    app.o -o app
~~~

The sysroot should contain a coherent set of target headers and libraries. It is not a random directory of copied files. Check:

- headers match the target libc and kernel/userspace interface;
- startup objects match the selected ABI;
- libraries have the correct ELF class, machine, endianness, and floating-point convention;
- dynamic loader paths refer to the target runtime;
- no host `/usr/include` or `/usr/lib` entered unintentionally;
- vendor headers do not redefine the ABI expected by the library.

Use `-nostdinc`, `-nostdinc++` where appropriate, include tracing, and verbose driver output to detect contamination. Do not use a broad `-I` path to hide a missing sysroot component.

## CPU, ISA, Endianness, And Floating Point

Target options affect both generated instructions and ABI:

- CPU revision and instruction-set extensions;
- ARM state/Thumb state or AArch64 features;
- RISC-V base ISA and extensions;
- byte order;
- pointer width and data model;
- hardware versus software floating point;
- vector or DSP extensions;
- unaligned-access support;
- atomic instruction availability.

All separately compiled objects that cross an ABI boundary must agree. A library compiled for hard-float registers cannot be called safely by a soft-float caller merely because the C prototypes match. The linker may diagnose some mismatches, but not every semantic mismatch.

Record target options in the artifact manifest and use compiler-provided attributes or note sections where available to detect incompatible objects.

## Bare-Metal Toolchains

A bare-metal toolchain commonly supplies:

- a cross compiler, assembler, linker, and binary utilities;
- compiler runtime support;
- a freestanding libc such as newlib or picolibc, or no libc;
- board/device headers and startup files from an SDK;
- a linker script and programmer/debugger integration.

The compiler does not know that a particular board has a UART or that an interrupt vector belongs at a certain address. Those are supplied by startup code, headers, linker scripts, and vendor documentation. Treat vendor SDK examples as evidence to review, not as a substitute for understanding the target contract.

## SDK Toolchains And Wrappers

An SDK wrapper may add hidden include paths, specs files, libraries, linker scripts, or post-link tools. Capture its expanded command line:

~~~sh
cmake --build build --verbose
make V=1
arm-none-eabi-gcc -### ...
~~~

When upgrading an SDK, compare:

- compiler and binutils versions;
- default language mode and target flags;
- libc and compiler runtime;
- startup and linker script;
- device headers and register definitions;
- post-link image format and signing steps;
- debugger and flash-tool assumptions.

Avoid mixing an SDK’s headers with a different vendor library or startup package unless the compatibility boundary is documented and tested.

## Host Tests And Target Integration

Keep portable logic independent of target headers where possible:

```text
portable policy / algorithms
        |
        +-- host adapter and tests
        +-- RTOS adapter
        +-- bare-metal/HAL adapter
        +-- embedded-Linux adapter
```

Host tests can validate parsing, state machines, arithmetic, and error policy quickly. They cannot prove MMIO ordering, interrupt latency, cache coherency, DMA behavior, or exact ABI integration. Mark tests with their execution environment and include target smoke tests for every hardware boundary.

## Reproducible Cross-Builds

Pin or record:

- toolchain release and binary hashes;
- target triple and complete flags;
- sysroot and SDK version;
- linker script and startup sources;
- generated headers and device configuration;
- environment variables affecting search paths;
- post-link conversion, signing, and checksum tools;
- source and submodule revisions.

Use a container, hermetic SDK, or controlled build environment where practical. A reproducible build is not only a supply-chain property; it is what lets a field crash address be mapped to the exact image that ran.

## Exercises

1. Print and document the complete target configuration of a cross compiler.
2. Create a minimal sysroot and prove that a host header cannot satisfy a target include.
3. Build one object for two CPU/ABI variants and inspect the ELF headers and compiler notes.
4. Intentionally mix hard-float and soft-float objects and identify the earliest diagnostic.
5. Compare verbose build commands before and after an SDK upgrade.
6. Run portable module tests on the host and a hardware smoke test through the target adapter.
7. Rebuild in a clean environment and compare hashes, map files, and image metadata.

## Common Mistakes

- using the host compiler because it is first in `PATH`;
- treating a target triple as a complete board description;
- copying a few target headers into a host include path;
- mixing sysroot, libc, startup, and compiler-runtime versions;
- using CPU flags inconsistently across libraries and applications;
- confusing endian, floating-point, and pointer-width choices;
- trusting host tests for hardware ordering or real-time behavior;
- allowing SDK wrappers to hide the effective command line;
- omitting generated headers or post-link tools from provenance;
- assuming a successful link proves that the image can boot on the board.

## Related Topics

- [Compiler Modes, Warnings, And Optimization](./compiler-modes-warnings-and-optimization.md)
- [Linker Scripts And Memory Layout](./linker-scripts-and-memory-layout.md)
- [Startup, Runtime, And `main`](./startup-runtime-and-main.md)
- [Platform-Specific C](../platform-specific-c/index.md)
- [Embedded C And Hardware](../embedded-c-and-hardware/index.md)

## References

- [GCC directory options and `--sysroot`](https://gcc.gnu.org/onlinedocs/gcc/Directory-Options.html)
- [GCC cross-compiler configuration and sysroots](https://gcc.gnu.org/install/configure.html)
- [GCC overall options](https://gcc.gnu.org/onlinedocs/gcc/Overall-Options.html)
- [GNU Binutils documentation](https://sourceware.org/binutils/docs/binutils.html)

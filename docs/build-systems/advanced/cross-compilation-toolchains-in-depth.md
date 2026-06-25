---
status: draft
reviewed: false
domain: build-systems
difficulty: advanced
reviewer: null
last_reviewed: null
---

# Cross-Compilation Toolchains In Depth

## What Problem Does This Solve?

Cross-compilation failures often come from ABI, libc, sysroot, compiler, linker, and runtime-loader mismatches. This page extends the intermediate cross-compilation material for platform engineers maintaining complete embedded Linux systems.

## Core Concepts

- GCC
- Clang
- binutils
- libc
- target triple
- ABI
- floating-point ABI
- sysroot
- dynamic linker
- external toolchain
- Yocto SDK
- relocatable SDK
- ccache

## Toolchain Components

```text
compiler driver
-> assembler
-> linker
-> libc headers
-> startup files
-> libgcc/compiler runtime
-> dynamic loader
-> target libraries
```

A "toolchain" is not just `gcc`. It is a compatible set of tools, headers, startup objects, libraries, and runtime assumptions.

## ABI Questions

Record:

- architecture
- endianness
- word size
- floating point ABI
- libc family and version
- kernel headers version
- dynamic loader path
- C++ standard library
- exception/unwind runtime

If two binaries disagree on these, linking or runtime loading can fail.

## SDKs

Yocto and vendor SDKs usually provide:

- cross compiler
- target sysroot
- environment setup script
- pkg-config configuration
- CMake toolchain hints
- debug symbols or separate debug packages

Use the SDK environment exactly for first reproduction. Then automate it in CI.

## Common Mistakes

- mixing compiler from one SDK with sysroot from another
- using host `/usr/include`
- using host `pkg-config`
- ignoring the dynamic loader path
- assuming hard-float and soft-float binaries can mix
- upgrading toolchain without rebuilding C++ libraries

## Related Topics

- [Cross-Compilation](../cross-compilation.md)
- [Target Triples and Sysroots](../target-triples-and-sysroots.md)
- [Shared Libraries, ABI, and Runtime Linking](../shared-libraries-abi-and-runtime-linking.md)


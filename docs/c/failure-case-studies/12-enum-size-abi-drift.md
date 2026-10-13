---
status: draft
reviewed: false
domain: c
difficulty: advanced
last_reviewed: null
---

# 12. Enum Size ABI Drift

A library and an application share a header containing a struct with an `enum` member.
They are compiled with different enum-size settings, so they disagree on the struct's
size and on every field offset after the enum. The compiler, the linker, and LTO all
accept the mismatch without a diagnostic.

## Symptom

Configuration values are wrong: a rate limit of 0, missing flags. Or the stack
protector aborts in a function that "only calls `config_load`". The same source works
when everything is built from one build system. It fails when a prebuilt vendor library
or a separately built component is linked in.

## Reproducer

```c
--8<-- "examples/c/failure-case-studies/12-enum-size-abi-drift/bug/config.h"
```

`config.c` fills all three fields and `main.c` prints them. The two files are compiled
with different settings:

```text
$ gcc -c -O2 -Wall -Wextra bug/config.c -o config.o
$ gcc -c -O2 -Wall -Wextra -fshort-enums bug/main.c -o main.o
$ gcc main.o config.o -o drift && ./drift
*** stack smashing detected ***: terminated

$ gcc -c -O2 -fshort-enums -fno-stack-protector bug/main.c -o main-nsp.o
$ gcc main-nsp.o config.o -o drift-nsp && ./drift-nsp
sizeof = 4 level = 1 flags = 0x00 rate_limit = 0
```

## What Happened

| Field | `config.o` (default) | `main.o` (`-fshort-enums`) |
| --- | --- | --- |
| `level` | offset 0, 4 bytes | offset 0, 1 byte |
| `flags` | offset 4 | offset 1 |
| `rate_limit` | offset 6 | offset 2 |
| `sizeof` | 8 | 4 |

`main` reserves 4 bytes on its stack. `config_load` writes 8 bytes, overwriting the
stack canary (first run) or other stack contents (second run). `main` then reads
`flags` and `rate_limit` at offsets where `config_load` never wrote.

## The Rule

Each enumerated type is compatible with `char`, a signed integer type, or an unsigned
integer type, and the choice is implementation-defined (C11 6.7.2.2p4).
`-fshort-enums` selects a different but conforming implementation. Linking objects
built for two implementations makes the declarations of `struct log_config`
incompatible, which is undefined (C11 6.2.7p2). C has no mechanism that checks layouts
across translation units, so nothing reports it.

## Why Review And Testing Missed It

- The header is identical in both builds, so source review sees nothing.
- C object files do not record struct layouts. The linker resolves the symbol
  `config_load` by name only.
- On Arm bare-metal targets, `arm-none-eabi-gcc` uses short enums by default, while
  other toolchains and Arm Linux use 32-bit enums. A vendor library built with "the
  other" setting is enough to trigger the bug, without anyone choosing
  `-fshort-enums`.

## Detection

| Tool | Result |
| --- | --- |
| `gcc -Wall -Wextra` (both objects) | silent |
| LTO with `-Wlto-type-mismatch` | silent: the build linked and printed the same wrong values |
| Stack protector | aborts, but only because the larger write happened to hit the canary |
| `_Static_assert(sizeof(struct log_config) == 8, ...)` in the header | `error: static assertion failed: "log_config ABI changed"` in the `-fshort-enums` build |

On Arm EABI targets, the object files record the enum size in their build
attributes (`Tag_ABI_enum_size`, visible with `readelf -A`), and GNU ld warns when
objects disagree. That check is Arm-specific. On x86-64 there is nothing to compare.

## Fix

Keep implementation-defined types out of shared layouts, and make the header check its
own contract:

```c
--8<-- "examples/c/failure-case-studies/12-enum-size-abi-drift/fixed/config.h"
```

```text
$ gcc -c -O2 -Wall -Wextra fixed/config.c -o config-fixed.o
$ gcc -c -O2 -Wall -Wextra -fshort-enums fixed/main.c -o main-fixed.o
$ gcc main-fixed.o config-fixed.o -o fixed && ./fixed
sizeof = 4 level = 1 flags = 0x01 rate_limit = 500
```

The same flag mismatch is now harmless, and any future change that alters the layout
fails to compile in every translation unit that includes the header.

## Keep It Fixed

- Fields in structs that cross a library, IPC, file, or wire boundary use
  fixed-width types. Enums, `bool`, `long`, bit-fields, and pointers are
  implementation-defined in size or layout.
- Put `_Static_assert` on `sizeof` and on `offsetof` of key fields in every public
  header that defines a shared struct.
- Record ABI-affecting flags (`-fshort-enums`, `-fpack-struct`, `-mabi`,
  `-mfloat-abi`, `-m32`/`-m64`) in the build manifest, and fail the build when a
  prebuilt library's attributes do not match.

## Same Root Cause Elsewhere

- `#pragma pack` that "leaks" from one header into the ones included after it.
- A struct containing `long` shared between 32- and 64-bit processes, or between a
  Cortex-M core and a Cortex-A core.
- `bool` or bit-field members in a struct shared with another compiler.
- A struct that grew a field in a new library version, linked against an old
  application: see versioned structs (`size` as the first member) in
  [APIs And Opaque Types](../modular-design-and-apis/api-and-opaque-types.md).

## Review Question

> Is this struct's layout defined by this header alone, or also by the compiler flags
> of everyone who includes it?

## Related Topics

- [Failure Case Studies overview](./index.md)
- [ABI, Calling Conventions, And FFI](../compilation-linking-and-abi/abi-calling-conventions-and-ffi.md)
- [Object Representation, Alignment, And Padding](../semantics-and-memory/object-representation-alignment-and-padding.md)
- [Structures, Unions, And Enumerations](../language-fundamentals/structures-unions-and-enums.md)

## References

- C11 (N1570) 6.7.2.2p4, enumeration compatible type; 6.2.7p2, compatible declarations
  across translation units
- [GCC `-fshort-enums`](https://gcc.gnu.org/onlinedocs/gcc/Code-Gen-Options.html)
- [Arm ABI: Addenda, build attributes (`Tag_ABI_enum_size`)](https://github.com/ARM-software/abi-aa)

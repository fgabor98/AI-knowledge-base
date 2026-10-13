---
status: draft
reviewed: false
domain: c
difficulty: advanced
last_reviewed: null
---

# 10. Struct Padding Leak

A struct is "serialized" with `memcpy`. Its padding bytes are never written, so they
still hold whatever was in that stack memory before, and they go out with the packet.
In the reproducer, that is part of a key handled by an earlier function.

## Symptom

Nothing fails. Every field decodes correctly on the receiver. Packet captures show
bytes between fields that change from run to run. In a security review those bytes
turn out to contain fragments of keys, passwords, or pointers, which also defeat
address-space randomization.

## Reproducer

```c
--8<-- "examples/c/failure-case-studies/10-struct-padding-leak/bug.c"
```

```text
$ gcc -O0 bug.c -o bug-O0 && ./bug-O0
01 58 59 5a 44 33 22 11 07 00 47 48
$ gcc -O2 -Wall -Wextra bug.c -o bug-O2 && ./bug-O2
01 54 55 56 44 33 22 11 07 00 43 44
```

| Offset | 0 | 1–3 | 4–7 | 8–9 | 10–11 |
| --- | --- | --- | --- | --- | --- |
| Content | `kind` | **padding** | `value` (LE) | `channel` (LE) | **padding** |
| `-O0` bytes | `01` | `58 59 5a` = `XYZ` | `44 33 22 11` | `07 00` | `47 48` = `GH` |
| `-O2` bytes | `01` | `54 55 56` = `TUV` | `44 33 22 11` | `07 00` | `43 44` = `CD` |

The padding contains the `A`–`Z` pattern written by `handle_session_key()`. The
leaked letters differ between `-O0` and `-O2` because the stack layout differs.

## What Happened

The compiler inserts 3 bytes after `kind` to align `value` to 4 bytes, and 2 bytes at
the end so the struct size is a multiple of 4. Assigning the three members writes 7 of
the 12 bytes. `memcpy` copies all 12.

## The Rule

When a value is stored in a structure object, including in one of its members, the
padding bytes "take unspecified values" (C11 6.2.6.1p6). Padding is not part of the
value. Copying a struct's object representation copies the padding as it is. Zeroing
the struct first helps in practice, but the standard allows a later member store to
change padding bytes again. Only explicit field encoding defines every byte that is
sent.

## Why Review And Testing Missed It

- The receiver decodes only the fields, so all functional tests pass.
- Padding is not visible in the source code.
- The leak depends on what earlier code left on the stack, so it is different every
  run and in every build.

## Detection

| Tool | Result |
| --- | --- |
| `gcc -Wall -Wextra` | silent |
| `-Wpadded` | `padding struct to align 'value'` and `padding struct size to alignment boundary with 2 bytes`: shows where the holes are, not that they leak |
| `-fanalyzer` | silent |
| `-fsanitize=address,undefined` | silent, and the instrumented run printed **zeros** in the padding, because ASan's stack layout differs |
| `-ftrivial-auto-var-init=zero` (GCC 12+) | padding is `00`: a hardening backstop that removes this leak |

ASan is the wrong tool for this bug, and here it hid the symptom. Clang's
MemorySanitizer reports uninitialized bytes reaching a system call such as `send`,
but it is not available in GCC.

## Fix

Define the wire format separately from the in-memory struct and encode each field:

```c
--8<-- "examples/c/failure-case-studies/10-struct-padding-leak/fixed.c:24:34"
```

```text
$ ./fixed
01 44 33 22 11 07 00
```

The packet is now 7 bytes, with a defined byte order and no padding. This fixes the
leak and also removes the struct layout and the host's endianness from the protocol.

## Keep It Fixed

- Never `memcpy`/`write`/`send` a struct across a trust or ABI boundary. That includes
  network, files, IPC, `copy_to_user`, and shared memory with another core.
- When a struct must be copied as a whole (for example a fixed-layout hardware
  descriptor), zero it with `memset` immediately before filling it, and use
  `_Static_assert(sizeof ...)` to make a layout change break the build.
- Enable `-ftrivial-auto-var-init=zero` (or the kernel's
  `CONFIG_INIT_STACK_ALL_ZERO`) as defense in depth, not as the fix.
- Decode-and-compare tests do not find this leak. Test that the encoded bytes match
  an expected byte array exactly.

## Same Root Cause Elsewhere

- Uninitialized struct members, not just padding: the same leak, with the same fix.
- `union` members smaller than the union: writing the small member leaves the rest of
  the bytes unchanged.
- Linux kernel information leaks: many were fixed by adding a `memset` before
  `copy_to_user` of a struct with padding. This recurring bug class is one of the
  reasons the kernel added automatic stack initialization options.

## Review Question

> Is this struct's memory copied anywhere outside the process, and which bytes of it
> did the code not explicitly write?

## Related Topics

- [Failure Case Studies overview](./index.md)
- [Object Representation, Alignment, And Padding](../semantics-and-memory/object-representation-alignment-and-padding.md)
- [Protocols And Serialization](../advanced-c/protocols-and-serialization.md)
- [Security](../correctness-quality-and-security/security.md)

## References

- C11 (N1570) 6.2.6.1p6, padding bytes take unspecified values
- [GCC `-ftrivial-auto-var-init`](https://gcc.gnu.org/onlinedocs/gcc/Optimize-Options.html)
- [GCC `-Wpadded`](https://gcc.gnu.org/onlinedocs/gcc/Warning-Options.html)
- [Clang MemorySanitizer](https://clang.llvm.org/docs/MemorySanitizer.html)

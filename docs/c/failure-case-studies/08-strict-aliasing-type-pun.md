---
status: draft
reviewed: false
domain: c
difficulty: advanced
last_reviewed: null
---

# 08. Strict Aliasing Type Pun

A function writes an object as `float`, modifies it through a `uint32_t *`, and reads it
back as `float`. At `-O2` the read returns the value from before the modification,
because GCC assumes a `uint32_t` store cannot change a `float`.

## Symptom

Bit-manipulation code for floats, register images, packet headers, or checksums
returns stale values at `-O2` but not at `-O0`. Adding a `printf` or moving code into
another function changes the result. The object in memory holds the right value, but
the function's return value does not.

## Reproducer

```c
--8<-- "examples/c/failure-case-studies/08-strict-aliasing-type-pun/bug.c"
```

```text
$ gcc -O0 bug.c -o bug-O0 && ./bug-O0
returned -1.0, object holds -1.0
$ gcc -O2 -Wall -Wextra bug.c -o bug-O2 && ./bug-O2
returned 1.0, object holds -1.0
```

## What The Compiler Did

```text
$ gcc -S -O2 bug.c                      # store_and_negate (comments added)
store_and_negate:
	movss	xmm0, DWORD PTR .LC0[rip]       # xmm0 = 1.0f
	movss	DWORD PTR [rdi], xmm0           # *value = 1.0f
	add	DWORD PTR [rsi], -2147483648    # *bits ^= 0x80000000
	ret                                     # return xmm0, still 1.0f
```

The function never reloads `*value`. The compiler knows it just stored `1.0f`, and
under the aliasing rules a store through `uint32_t *` cannot modify an object of type
`float`, so it returns the value still in the register. (Flipping only the top bit can
be done with an add, which explains `add` instead of `xor`.)

```text
$ gcc -O2 -fno-strict-aliasing bug.c -o bug-nsa && ./bug-nsa
returned -1.0, object holds -1.0
```

## The Rule

An object may have its stored value accessed only through an lvalue of a compatible
type, a signed/unsigned or qualified variant of it, an aggregate or union containing
it, or a character type (C11 6.5p7). `float` and `uint32_t` are not compatible, so
`*bits` accessing the `float` object is undefined. The cast in `main` is legal. The
access through the result is not.

This rule is what allows the compiler to keep values in registers across stores
through pointers of other types. Without it, every `int *` store would force
every cached `float` to be reloaded.

## Why Review And Testing Missed It

- Casting a pointer to "look at the bits" is common in older embedded code, and
  it usually works when the access is in a different function from the store.
- `-Wall -Wextra` is silent. GCC's default `-Wstrict-aliasing` level warns only about
  a few direct patterns.
- The bug depends on inlining and on what the optimizer can see in a single
  function, so it can appear or disappear when unrelated code changes.

## Detection

| Tool | Result |
| --- | --- |
| `gcc -Wall -Wextra` | silent |
| `-Wstrict-aliasing=1` | `dereferencing type-punned pointer might break strict-aliasing rules` (at the cast in `main`) |
| `-fanalyzer` | silent |
| `-fsanitize=undefined` | silent: UBSan does not check effective types |

Level 1 of `-Wstrict-aliasing` is the most aggressive and the least precise. It warns
about the cast whether or not a bad access follows. Use it as a way to find
candidates, not as a CI gate.

## Fix

Copy the representation instead of aliasing it:

```c
--8<-- "examples/c/failure-case-studies/08-strict-aliasing-type-pun/fixed.c:6:17"
```

```text
$ gcc -S -O2 fixed.c                    # negate_bits
negate_bits:
	movd	eax, xmm0
	add	eax, -2147483648
	movd	xmm0, eax
	ret
```

The `memcpy` calls are compiled away into register moves. A representation copy is
the defined way to reinterpret bytes, and at `-O1` and above it usually costs nothing.

## Keep It Fixed

- Ban casts between unrelated object-pointer types outside a small, reviewed set of
  "representation" helpers (`load_le32`, `float_bits`, and similar). These helpers use
  `memcpy` or byte-by-byte shifts.
- Do not cast a byte buffer to a `struct` pointer to parse it. Decode fields from bytes
  (see [case 10](./10-struct-padding-leak.md) for the encoding direction).
- If a codebase cannot be fixed, build it with `-fno-strict-aliasing` and record the
  decision. The Linux kernel builds this way. It is a valid choice, but it makes the
  code depend on that flag, and the flag has a performance cost.

## Same Root Cause Elsewhere

- `*(uint32_t *)&packet[4]`: an aliasing violation, and also a misaligned access on
  cores without unaligned support.
- Allocated buffers reused for a different type without `memcpy`: the effective type
  rules (C11 6.5p6) apply to `malloc` memory too.
- Union type-punning is defined in C (C11 footnote 95) but not in C++. A header shared
  with C++ needs a different approach.

## Review Question

> Is any object accessed through a pointer whose type differs from the object's type,
> other than `unsigned char *`?

## Related Topics

- [Failure Case Studies overview](./index.md)
- [Conversions, Promotions, And Aliasing](../semantics-and-memory/conversions-promotions-and-aliasing.md)
- [Object Representation, Alignment, And Padding](../semantics-and-memory/object-representation-alignment-and-padding.md)
- [Protocols And Serialization](../advanced-c/protocols-and-serialization.md)

## References

- C11 (N1570) 6.5p6–7, effective type and aliasing; 6.5.2.3 footnote 95, union
  type punning
- [GCC `-fstrict-aliasing`](https://gcc.gnu.org/onlinedocs/gcc/Optimize-Options.html)
- [GCC `-Wstrict-aliasing`](https://gcc.gnu.org/onlinedocs/gcc/Warning-Options.html)

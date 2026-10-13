---
status: draft
reviewed: false
domain: c
difficulty: advanced
last_reviewed: null
---

# 04. Shift Count Equal To Width

`(1u << n) - 1` is the usual way to build an `n`-bit mask, but it is undefined for
`n == 32` on a 32-bit type. In the reproducer, the same call with the same argument
returns two different values in one executable.

## Symptom

A "full width" configuration, such as all 32 GPIO lines, a 32-bit field extractor, or a
32-bit CRC width, produces a mask of `0` on the target. A unit test that passes a
constant `32` gets `0xffffffff` and passes.

## Reproducer

```c
--8<-- "examples/c/failure-case-studies/04-shift-count-width/bug.c"
```

```text
$ gcc -O0 bug.c -o bug-O0 && ./bug-O0 32
low_bits_mask(32) at run time   = 0x00000000
low_bits_mask(32) constant-folded = 0x00000000
$ gcc -O2 -Wall -Wextra bug.c -o bug-O2 && ./bug-O2 32
low_bits_mask(32) at run time   = 0x00000000
low_bits_mask(32) constant-folded = 0xffffffff
```

## What The Compiler Did

```text
$ gcc -S -O2 bug.c                      # low_bits_mask
low_bits_mask:
	mov	ecx, edi
	mov	eax, 1
	sal	eax, cl
	sub	eax, 1
	ret
```

There are two code paths, and each is a reasonable result of undefined input:

- **At run time** the shift is the x86 `SAL`/`SHL` instruction. The CPU masks a 32-bit
  shift count to its low 5 bits, so `1 << 32` executes as `1 << 0 == 1`, and
  `1 - 1 == 0`.
- **At compile time**, with the argument known, GCC folded the expression with its own
  arithmetic and produced `0xffffffff`.

On a 32-bit Arm core, a register-specified shift uses the low byte of the count, and
`1 << 32` gives `0`, so the runtime result is `0xffffffff`. The C source is the same,
but the target is different, and so is the answer.

## The Rule

If the right operand of a shift is negative, or greater than or equal to the width of
the *promoted* left operand, the behavior is undefined (C11 6.5.7p3). For signed left
operands, shifting a 1 into or past the sign bit is also undefined.

## Why Review And Testing Missed It

- The helper's comment says `0..32`, and the expression looks right for all of them.
- Tests usually pass literal arguments. Literal arguments are constant-folded, and
  constant folding gives the "expected" answer.
- The failure needs a value computed at run time, which usually comes from
  configuration.

## Detection

| Tool | Result |
| --- | --- |
| `gcc -Wall -Wextra` | silent (it warns only for a constant shift count in the expression itself) |
| `-fanalyzer` | `shift by count ('32') >= precision of type ('32') [-Wanalyzer-shift-count-overflow]` |
| `-fsanitize=undefined` | `runtime error: shift exponent 32 is too large for 32-bit type 'unsigned int'` |

## Fix

Handle the full-width case explicitly:

```c
--8<-- "examples/c/failure-case-studies/04-shift-count-width/fixed.c:6:13"
```

```text
low_bits_mask(32) at run time   = 0xffffffff
low_bits_mask(32) constant-folded = 0xffffffff
low_bits_mask(5) at run time   = 0x0000001f
```

A branch-free alternative is `UINT32_MAX >> (32u - n)` for `n` in `1..32`, which has
its own special case at `n == 0`. Whichever form you choose, write the domain in the
interface and test both ends.

## Keep It Fixed

- Keep bit-field and mask helpers in one reviewed module with tests at `0`, `1`,
  `width - 1`, and `width`, using arguments that are **not** compile-time constants
  (read them from a `volatile` or from input).
- Run those tests under UBSan.
- Watch for promotions: `uint8_t x; x << 8` is an `int` shift and is fine, but
  `x << 31` can shift into the sign bit of `int`.

## Same Root Cause Elsewhere

- Extracting a field with `(value >> shift) & mask`, where `shift` can reach the
  register width.
- Rotations written as `(x << n) | (x >> (32 - n))`, which are undefined at
  `n == 0`. Use `(x << (n & 31)) | (x >> (-n & 31))` with an unsigned `n`; compilers recognize it as a rotate.
- 64-bit code that builds a mask with `1 << n` (an `int`) instead of `UINT64_C(1) << n`.

## Review Question

> What is the largest shift count that can reach this expression, and what is the
> width of the left operand *after promotion*?

## Related Topics

- [Failure Case Studies overview](./index.md)
- [Expressions And Operators](../language-fundamentals/expressions-and-operators.md)
- [Undefined Behavior](../semantics-and-memory/undefined-behavior.md)
- [x86-64](../platform-specific-c/x86-64.md)

## References

- C11 (N1570) 6.5.7p3–4, bitwise shift operators
- Intel 64 and IA-32 Architectures Software Developer's Manual, `SAL/SAR/SHL/SHR`:
  the count is masked to 5 bits (6 bits with REX.W)
- [GCC `-Wanalyzer-shift-count-overflow`](https://gcc.gnu.org/onlinedocs/gcc/Static-Analyzer-Options.html)

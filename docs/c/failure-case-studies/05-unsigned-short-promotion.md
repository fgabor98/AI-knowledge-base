---
status: draft
reviewed: false
domain: c
difficulty: advanced
last_reviewed: null
---

# 05. Unsigned Short Promotion

Both operands are unsigned and the result is stored in a `uint32_t`, yet the
multiplication is a signed `int` multiplication that can overflow. The compiler uses
that to fold a range check to a constant.

## Symptom

A check on a product of two 16-bit values, such as a pixel count, sample count × sample
size, or a register-field product, never fires. The function behaves as if the product
can never exceed `INT_MAX`.

## Reproducer

```c
--8<-- "examples/c/failure-case-studies/05-unsigned-short-promotion/bug.c"
```

```text
$ gcc -O0 bug.c -o bug-O0 && ./bug-O0
product_is_large(0xffff, 0xffff) = 1
$ gcc -O2 -Wall -Wextra -Wconversion bug.c -o bug-O2 && ./bug-O2
product_is_large(0xffff, 0xffff) = 0
```

The true product is `0xfffe0001`, which is above `0x80000000`, so the correct answer
is `1`.

## What The Compiler Did

```text
$ gcc -S -O2 bug.c                      # product_is_large
product_is_large:
	xor	eax, eax
	ret
```

The compiler's reasoning:

1. `a` and `b` are promoted to `int` (on this target, `int` can represent every
   `uint16_t` value).
2. Both are non-negative, and signed overflow is undefined, so `a * b` is in
   `[0, INT_MAX]`.
3. Converting a value in that range to `uint32_t` gives at most `0x7fffffff`.
4. `product >= 0x80000000u` is therefore always false.

The fixed version keeps the multiplication:

```text
product_is_large:
	movzx	esi, si
	movzx	edi, di
	imul	edi, esi
	shr	edi, 31
	mov	eax, edi
	ret
```

## The Rule

Integer promotions convert operands of rank lower than `int` to `int` when `int` can
represent all their values, and otherwise to `unsigned int` (C11 6.3.1.1p2). So
`uint16_t * uint16_t` is an `int` multiplication on a platform with 32-bit `int`,
and overflow of that multiplication is undefined (C11 6.5p5). The
assignment to `uint32_t` happens afterwards and cannot make the multiplication
unsigned.

The same source is well defined on a platform with 16-bit `int` (the operands are
promoted to `unsigned int`) and undefined on a platform with 32-bit `int`. The
signedness of the arithmetic depends on the target.

## Why Review And Testing Missed It

- Every visible type is unsigned. Reviewers check the destination type, not the type
  in which the expression is evaluated.
- `-Wconversion` is silent: there is no narrowing conversion, only a promotion.
- Only products above `INT_MAX` fail, so at least one operand has to be above
  46,340. Typical test values are much smaller.

## Detection

| Tool | Result |
| --- | --- |
| `gcc -Wall -Wextra -Wconversion` | silent |
| `-fanalyzer` | silent |
| `-fsanitize=undefined` | `runtime error: signed integer overflow: 65535 * 65535 cannot be represented in type 'int'` |

The sanitized build printed `1`: once again, the build with the report produced the
correct answer.

## Fix

Convert to the intended arithmetic type before the operation:

```c
--8<-- "examples/c/failure-case-studies/05-unsigned-short-promotion/fixed.c:6:12"
```

Converting one operand is enough, because the usual arithmetic conversions then
convert the other operand to `uint32_t` as well. Some teams convert both for clarity.

## Keep It Fixed

- Code review rule: in any arithmetic expression on `uint8_t`/`uint16_t` operands,
  write down the type the expression is evaluated in. If it is `int`, check its range.
- Boundary tests should use the maximum value of every narrow operand at the same
  time.
- MISRA C's "essential type" model (rules 10.x) targets this class of bug. A checker
  that enforces those rules finds it statically.

## Same Root Cause Elsewhere

- **Bitwise complement on narrow types:** `uint8_t b = 0x5a; if (~b == 0xa5)` is
  false, because `~b` is the `int` `0xffffffa5`. `-Wextra` reports this one
  (`comparison of promoted bitwise complement of an unsigned value with unsigned`).
- **Shifts:** `uint8_t x = 0x80; x << 24` shifts into the sign bit of `int`.
- **Subtraction:** `uint16_t a = 1, b = 2; if (a - b > 0)` is false, because `a - b`
  is the `int` `-1`, not a large unsigned value.

## Review Question

> In what type is this expression *evaluated*, as opposed to the type it is stored
> into?

## Related Topics

- [Failure Case Studies overview](./index.md)
- [Conversions, Promotions, And Aliasing](../semantics-and-memory/conversions-promotions-and-aliasing.md)
- [02. Signed Overflow Check](./02-signed-overflow-check.md)
- [Safety Standards And MISRA](../correctness-quality-and-security/safety-standards-and-misra.md)

## References

- C11 (N1570) 6.3.1.1p2, integer promotions; 6.3.1.8, usual arithmetic conversions;
  6.5p5, exceptional conditions
- [GCC `-fsanitize=signed-integer-overflow`](https://gcc.gnu.org/onlinedocs/gcc/Instrumentation-Options.html)

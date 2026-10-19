---
status: draft
reviewed: false
domain: c
difficulty: advanced
last_reviewed: null
---

# 02. Signed Overflow Check

An overflow check written as "did the result wrap?" relies on the overflow it is
supposed to detect. For signed integers that overflow is undefined, so GCC rewrites the
check into something that never fires. In this case that happens even at `-O0`.

## Symptom

A length, timestamp, or counter calculation that should be rejected near `INT_MAX` is
accepted. The guard is visible in the source and seems to work in a code walk-through,
but it never triggers.

## Reproducer

```c
--8<-- "examples/c/failure-case-studies/02-signed-overflow-check/bug.c"
```

```text
$ gcc -O0 bug.c -o bug-O0 && ./bug-O0
add_overflows(INT_MAX, 1) = 0
$ gcc -O2 -Wall -Wextra bug.c -o bug-O2 && ./bug-O2
add_overflows(INT_MAX, 1) = 0
$ gcc -O2 -fwrapv bug.c -o bug-wrapv && ./bug-wrapv
add_overflows(INT_MAX, 1) = 1
```

## What The Compiler Did

```text
$ gcc -S -O2 bug.c                      # add_overflows
add_overflows:
	shr	esi, 31
	mov	eax, esi
	ret
```

The function returns the sign bit of `b`. Since `a + b` cannot overflow in a defined
program, `a + b < a` holds exactly when `b < 0`, and that is what GCC computes. The
`-O0` output contains the same `shr eax, 31` and no addition at all. GCC's front end
folds the comparison before any optimization pass runs, so turning off optimization
does not bring the wraparound back. Only `-fwrapv`, which defines signed overflow as
two's-complement wraparound, does.

## The Rule

If an arithmetic result is not representable in its type, the behavior is undefined
(C11 6.5p5). Unsigned arithmetic is the exception: it is defined to wrap modulo
2^N (C11 6.2.5p9). Therefore "compute, then check whether it wrapped" is valid for
unsigned types and invalid for signed types.

## Why Review And Testing Missed It

- The idiom is correct for unsigned arithmetic, and many engineers learned it there.
- Tests usually exercise values in the middle of the range; the boundary test is the
  one that fails.
- "It must be the optimizer" leads people to test at `-O0`. Here `-O0` gives the same
  answer, which can make a reader conclude the logic is wrong rather than undefined.

## Detection

| Tool | Result |
| --- | --- |
| `gcc -Wall -Wextra` | silent |
| `-Wstrict-overflow=5` | silent |
| `-fanalyzer` | silent |
| `-fsanitize=undefined` | `runtime error: signed integer overflow: 2147483647 + 1 cannot be represented in type 'int'` |

The UBSan build printed `1`, the "intended" answer, while it reported the error,
because the instrumented build performs the addition. Check the sanitizer output, not
whether the program's result looks right.

## Fix

Check the operands so the overflowing operation is never evaluated:

```c
--8<-- "examples/c/failure-case-studies/02-signed-overflow-check/fixed.c:6:10"
```

```text
add_overflows(INT_MAX, 1) = 1
add_overflows(INT_MIN, -1) = 1
add_overflows(1, 1) = 0
```

Alternatives, in order of preference for new code:

- C23 `ckd_add(&result, a, b)` from `<stdckdint.h>` (GCC 14 and later; not available
  in the GCC 13 used here);
- `__builtin_add_overflow(a, b, &result)` in GCC and Clang, which also produces the
  result;
- do the arithmetic in a wider type when one exists and the range is known.

## Keep It Fixed

- Provide one checked-arithmetic helper module and forbid ad-hoc "did it wrap"
  checks on signed values in review.
- Run boundary-value tests (`INT_MIN`, `INT_MAX`, `-1`, `0`, `1`) under UBSan in CI.
- If the project builds with `-fwrapv` (the Linux kernel uses `-fno-strict-overflow`),
  record that as a project-wide decision. Code that relies on it cannot be moved
  to a project without that flag.

## Same Root Cause Elsewhere

- `if (x + 1 > x)` loop guards, `if (len * size / size != len)` multiplication checks
  on signed types, and timestamp comparisons such as `now + timeout < now`.
- Pointer arithmetic has the same rule: `ptr + len < ptr` is also folded, because forming
  a pointer outside the object is undefined (C11 6.5.6p8). This was the subject of
  CERT vulnerability note VU#162289.

## Review Question

> Does any overflow check compute the possibly overflowing value first?

## Related Topics

- [Failure Case Studies overview](./index.md)
- [Undefined Behavior](../semantics-and-memory/undefined-behavior.md)
- [Conversions, Promotions, And Aliasing](../semantics-and-memory/conversions-promotions-and-aliasing.md)
- [05. Unsigned Short Promotion](./05-unsigned-short-promotion.md)

## References

- C11 (N1570) 6.5p5, exceptional conditions; 6.2.5p9, unsigned wraparound
- [GCC integer overflow builtins](https://gcc.gnu.org/onlinedocs/gcc/Integer-Overflow-Builtins.html)
- [GCC `-fwrapv` and `-fstrict-overflow`](https://gcc.gnu.org/onlinedocs/gcc/Code-Gen-Options.html)
- [CERT VU#162289](https://www.kb.cert.org/vuls/id/162289)
- Wang, Chen, Jia, Zeldovich, Kaashoek, *Undefined Behavior: What Happened to My Code?*
  (APSys 2012)

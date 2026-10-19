---
status: draft
reviewed: false
domain: c
difficulty: advanced
last_reviewed: null
---

# 15. Fast-Math NaN Check

A signal filter rejects corrupted samples with `isnan()`. The project builds with
`-ffast-math` for speed, which lets the compiler assume NaN never occurs. The guard
compiles to nothing, and the first NaN sample permanently sets the filter state to
NaN.

## Symptom

After one bad sensor reading (a disconnected ADC, a 0/0 in calibration, an
uninitialized float from a peripheral), every later output of the filter is `nan`.
Control loops that use the output lock up or saturate. The code has an explicit NaN
check, and it worked in the unoptimized build.

## Reproducer

```c
--8<-- "examples/c/failure-case-studies/15-fast-math-nan-check/bug.c"
```

```text
$ gcc -O2 -Wall -Wextra bug.c -o bug && ./bug nan
smooth(nan, 20.0) = 20.000000
$ gcc -O2 -Wall -Wextra -ffast-math bug.c -o bug-fast && ./bug-fast nan
smooth(nan, 20.0) = nan
$ gcc -O2 -ffinite-math-only bug.c -o bug-finite && ./bug-finite nan
smooth(nan, 20.0) = nan
```

## What The Compiler Did

```text
$ gcc -S -O2 -ffast-math bug.c          # smooth
smooth.constprop.0:
	mulsd	xmm0, QWORD PTR .LC0[rip]
	addsd	xmm0, QWORD PTR .LC1[rip]
	ret
```

There is no comparison: `isnan(sample)` was folded to `0`. The `.constprop` suffix
means GCC also specialized the function for the constant `previous == 20.0`.

## The Rule

ISO C defines `isnan` (C11 7.12.3.4), and Annex F binds C floating point to
IEC 60559 (IEEE 754), where NaN is an ordinary value that arithmetic produces and
propagates. `-ffast-math` turns on `-ffinite-math-only`, which, according to the GCC
manual, allows optimizations "that assume that arguments and results are not NaNs or
+-Infs". With that flag the compiler is no longer providing Annex F semantics, and
tests for NaN or infinity can legally be folded away. The source is correct; the build
flag changed what the language guarantees.

`-ffast-math` also enables reassociation, reciprocal approximations, and
`-fno-signed-zeros`, and when used to link an executable, it may link startup code
that enables flush-to-zero for the whole process, including libraries that were not
compiled with it.

## Why Review And Testing Missed It

- The flag is in the build system, not in the source under review.
- The NaN guard is correct C and passes every test in a build without
  `-ffast-math`.
- GCC 13 gives no warning that a NaN test was removed.

## Detection

| Tool | Result |
| --- | --- |
| `gcc -Wall -Wextra -ffast-math` | silent |
| `-fanalyzer`, UBSan, ASan | silent: this is not undefined behavior in the source |
| A unit test feeding NaN and Inf, run with the **production** flags | fails |

Only a test that runs with the same floating-point flags as the shipped build can
catch this.

## Fix

Two complementary fixes.

**Build:** do not use `-ffast-math` globally. If profiling shows that a specific
kernel needs it, enable only the parts that are safe for that code. For example,
`-fno-math-errno` lets `sqrt` and similar functions compile to single instructions.
Where the algorithm tolerates different rounding, `-ffp-contract=fast` allows fused
multiply-add. Keep `-fno-finite-math-only` for any translation unit that handles
external data.

**Source:** where the input is untrusted and the build flags are not under your
control, classify the value by its representation, which no math flag can change:

```c
--8<-- "examples/c/failure-case-studies/15-fast-math-nan-check/fixed.c:8:27"
```

```text
$ gcc -O2 -Wall -Wextra -ffast-math fixed.c -o fixed
$ ./fixed nan
smooth(nan, 20.0) = 20.000000
$ ./fixed 30
smooth(30, 20.0) = 21.000000
```

## Keep It Fixed

- Validate floating-point input at the boundary (driver, parser, IPC), in a
  translation unit built without `-ffinite-math-only`. Inside the boundary, fast-math
  assumptions are then actually true.
- Run the numerical unit tests with the production compiler flags, including NaN,
  ±Inf, subnormal, and signed-zero inputs.
- Record floating-point flags in the build manifest. Treat changes to them like ABI
  changes: they change program behavior, not just speed.

## Same Root Cause Elsewhere

- `x != x` as a NaN test is folded the same way: with `-ffinite-math-only` it
  compiles to `xor eax, eax`.
- Infinity checks (`isinf`, `x > DBL_MAX`) used for overflow detection.
- Kahan summation and other compensated algorithms: `-fassociative-math` can
  simplify the compensation term to zero.
- Fixed-point or DSP code that relies on exact rounding or signed zero, built with a
  vendor's "max speed" preset that silently includes fast-math.

## Review Question

> Which floating-point guarantees does this code rely on, and does the build
> configuration for this file still provide them?

## Related Topics

- [Failure Case Studies overview](./index.md)
- [Numerical And Fixed-Point C](../advanced-c/numerical-and-fixed-point-c.md)
- [Compiler Modes, Warnings, And Optimization](../compilation-linking-and-abi/compiler-modes-warnings-and-optimization.md)
- [Numeric, Time, And Character APIs](../standard-library-and-ecosystem/numeric-time-and-character-apis.md)

## References

- C11 (N1570) 7.12.3.4, `isnan`; Annex F, IEC 60559 floating-point arithmetic
- [GCC `-ffast-math` and `-ffinite-math-only`](https://gcc.gnu.org/onlinedocs/gcc/Optimize-Options.html)

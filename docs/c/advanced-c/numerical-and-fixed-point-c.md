---
status: draft
reviewed: false
domain: c
difficulty: advanced
last_reviewed: null
---

# Numerical And Fixed-Point C

Numerical C is the engineering of representation, range, precision, rounding, units,
and error. The expression `a * b / scale` is not a complete fixed-point design, and
the presence of an FPU does not make floating-point behavior deterministic, exact, or
free. Embedded code needs an explicit numerical contract for sensor values, control
loops, DSP pipelines, calibration, graphics, and communications.

## Learning Objectives

- Choose between integer, fixed-point, floating-point, decimal, and scaled-unit
  representations.
- Design fixed-point formats with documented range, resolution, rounding, saturation,
  and conversion rules.
- Detect intermediate overflow before it happens and keep signed arithmetic defined.
- Understand IEEE 754 NaN, infinity, signed zero, subnormal, rounding, and exception
  behavior at the level needed for production code.
- Assess numerical stability, quantization, error accumulation, DSP layout, and SIMD
  trade-offs.

## Start With A Numerical Contract

For every value, document:

- physical unit and reference point;
- mathematical range and permitted invalid/sentinel values;
- storage representation and scale;
- resolution and acceptable absolute/relative error;
- rounding mode and tie behavior;
- overflow/underflow behavior: reject, saturate, wrap, flag, or use infinity;
- endian/serialization representation;
- whether comparisons, ordering, and equality are exact;
- time budget, determinism, and hardware acceleration requirements.

Do not mix units in a typedef-less integer API. A millivolt count, a Q15 normalized
value, and a raw ADC code can all be `int32_t` while having incompatible ranges.
Use names, wrapper types, conversion functions, or comments at every public boundary.

## Fixed-Point Representation

In a signed Q-format with `F` fractional bits, an integer storage value `raw` represents

```text
value = raw / 2^F
```

For an N-bit two's-complement storage type, the approximate range is

```text
[-2^(N-1) / 2^F, (2^(N-1)-1) / 2^F]
```

The exact integer representation, sign, and overflow behavior still belong to the
implementation contract. A format such as Q1.15 usually means one sign/integer bit
and fifteen fractional bits in a 16-bit storage value, but naming conventions vary;
write the scale explicitly in the API.

Choose fractional bits from the worst-case intermediate, not only the input. A filter
may have coefficients less than one but accumulate many products. Keep guard bits in
an accumulator and narrow only at a controlled point.

## Checked And Saturating Arithmetic

Wraparound is usually wrong for physical quantities and control signals. Saturation
clamps to the representable range, while checked arithmetic reports that the ideal
result did not fit. Choose the behavior per API: a safety limit may require an error,
an audio sample may saturate, and a modular counter may intentionally wrap.

```c
#include <stdint.h>

static int32_t saturate_i64_to_i32(int64_t value)
{
    if (value > INT32_MAX) {
        return INT32_MAX;
    }
    if (value < INT32_MIN) {
        return INT32_MIN;
    }
    return (int32_t)value;
}

static int32_t q15_add(int32_t left, int32_t right)
{
    return saturate_i64_to_i32((int64_t)left + (int64_t)right);
}

static int32_t q15_multiply(int32_t left, int32_t right)
{
    int64_t product = (int64_t)left * (int64_t)right;
    int64_t rounding = product >= 0 ? (INT64_C(1) << 14)
                                    : -(INT64_C(1) << 14);
    return saturate_i64_to_i32((product + rounding) >> 15);
}
```

This example uses a 15-bit fractional scale but does not enforce that inputs fit a
particular Q15 range. Add constructors and range checks at the API boundary if that is
required. The rounding rule for negative values should be specified; the shown rule is
one symmetric approximation, not the only valid policy.

Before using a wider intermediate, prove that the multiplication itself fits that
intermediate. If two 64-bit values can be multiplied, a 64-bit product is not enough;
use range reduction, wider arithmetic when available, or a checked multiply algorithm.

## Scaling And Unit Conversions

Conversions can overflow even when both endpoints are representable. Reorder operations
only when the algebra and rounding error remain acceptable. A safe conversion often:

1. validates the source range;
2. widens to an intermediate type;
3. multiplies by a bounded numerator;
4. divides with an explicitly selected rounding rule;
5. checks the destination range;
6. returns a value plus status or saturates according to policy.

Keep calibration coefficients in the same scale as the signal or convert once at a
well-defined boundary. Repeatedly converting between scales can accumulate rounding
error and make equality tests unstable.

## Floating-Point Model

Most modern embedded and hosted systems use IEEE 754 binary floating point, but C's
floating types and implementation options still vary. Verify width, evaluation method,
flush-to-zero behavior, FPU mode, fused operations, and library implementation on the
target.

Important values and behaviors include:

- **NaN:** unordered with ordinary numbers; `x == x` is false for a NaN.
- **Infinity:** can represent overflow or a mathematical unbounded result, but may
  contaminate later calculations.
- **Signed zero:** compares equal to positive zero but can affect reciprocals, signs,
  and some functions.
- **Subnormal values:** improve gradual underflow but may be slow or flushed to zero.
- **Rounding:** default-to-nearest is common, but conversion and FPU modes can change it.
- **Fused multiply-add:** computes `a*b+c` with one final rounding, which can improve
  accuracy but change bit-for-bit results.
- **Exceptions/status:** invalid, divide-by-zero, overflow, underflow, and inexact may
  set flags or traps depending on the environment.

Never use `memcmp` to compare floating-point values as mathematical numbers. Use an
absolute/relative tolerance appropriate to the scale, and handle NaN/infinity explicitly.
For deterministic protocols, convert to a specified integer or byte representation.

## Numerical Stability

An algorithm can be mathematically correct and numerically unstable. Look for:

- subtracting nearly equal values (cancellation);
- summing many values with different magnitudes;
- repeated multiplication of values slightly above/below one;
- ill-conditioned matrix or calibration problems;
- feedback loops whose quantization and delay change stability;
- division by a small or noisy quantity.

Use algebraically stable forms, compensated summation where cost permits, scaling and
normalization, appropriate accumulation order, and bounded intermediate values. Test
with worst-case and adversarial inputs, not only nominal sensor data.

## Quantization And Rounding

Quantization maps a continuous or high-resolution value to a finite set. It introduces
error, possible bias, and sometimes signal-dependent artifacts. Decide whether to use:

- truncation, which is cheap but biased;
- round-to-nearest, which generally reduces average error;
- ties-to-even, which reduces aggregate bias in some workloads;
- stochastic rounding, useful in some iterative or low-precision algorithms;
- dithering, useful when quantization artifacts are audible or patterned.

Rounding at every pipeline stage may accumulate error. Keep extra precision internally
and quantize at a boundary when the range permits. If saturation follows rounding, do
the operations in a type wide enough to represent the rounded intermediate.

## DSP-Oriented C

DSP code is often a pipeline of multiply-accumulate operations with strict layout and
latency requirements. Decide whether samples are interleaved or planar, how channels
are aligned, whether buffers wrap, and who owns them during DMA.

For a FIR filter, analyze the maximum sum of absolute coefficient times input bounds.
Use a wider accumulator, scale coefficients, or prove a bounded signal envelope. Do
not rely on “typical” signal levels to avoid overflow in a safety or control path.

Circular addressing, saturating instructions, packed SIMD, and DMA can improve cost but
couple the algorithm to the target. Keep a clear scalar reference implementation for
tests and differential comparison.

## SIMD And Intrinsics

SIMD changes the data layout, alignment, tail handling, and floating-point contraction
policy. An optimized loop must define behavior for a length that is not a multiple of
the vector width. Do not read past the logical end merely because an aligned load is
faster unless the extra bytes are allocated and legally accessible.

Separate:

- algorithmic vectorization and target-specific intrinsic wrappers;
- alignment assumptions and fallback paths;
- exact versus approximate math functions;
- exception/NaN behavior and flush-to-zero modes;
- feature detection and deployment baseline.

Compare the vector path with the scalar reference using tolerances and edge cases. A
faster approximation is not acceptable if it changes control decisions outside the
documented error budget.

## Overflow-Aware APIs

Make error behavior visible in the type or return value. Common patterns are:

- return status and write the result through an output pointer;
- return a `{ value, status }` structure;
- use a saturating function whose name explicitly says saturation;
- require a caller-proven range and state it as a precondition;
- use a wider intermediate and return a narrowed value only after checking.

Do not use a sentinel that can also be a valid numerical value without a separate status
bit. Do not hide saturation in a generic operator-like macro where callers cannot tell
whether information was lost.

## Numerical Testing

Use several oracles:

- a high-precision or arbitrary-precision reference model on the host;
- algebraic properties such as monotonicity, conservation, and boundedness;
- known vectors from the algorithm or hardware specification;
- randomized and boundary values, including NaN/infinity when supported;
- differential tests between scalar, fixed-point, and SIMD implementations;
- long-run tests for accumulation drift and feedback stability.

Record compiler flags, FPU mode, rounding mode, target features, and math-library
version. Bit-for-bit reproducibility requires more than using the same source code.

## Exercises And Diagnostics

1. Design a Q format for a sensor with a stated range and resolution; calculate the
   largest filter intermediate and choose guard bits.
2. Implement checked, wrapping, and saturating additions; test every boundary around
   the representable range.
3. Compare naive and compensated summation on values with large magnitude differences.
4. Build scalar and SIMD/FPU implementations of a small FIR filter and compare error,
   cycles, code size, and tail handling.
5. Inject NaN, infinity, subnormal, and signed-zero values into a control calculation
   and document the desired behavior.

## Common Mistakes

- Naming a scale without documenting range, units, rounding, and saturation.
- Performing fixed-point multiplication in a type too narrow for the product.
- Relying on signed overflow or accidental unsigned conversions.
- Comparing floating-point values with exact equality or `memcmp`.
- Assuming IEEE 754 behavior, FPU width, or denormal handling without checking the
  target and compiler options.
- Vectorizing without handling tails, alignment, aliasing, or feature availability.
- Using a mathematically stable-looking formula that loses precision in the target's
  range.
- Testing only nominal values and ignoring worst-case accumulation or feedback drift.

## Related Topics

- [Advanced C overview](./index.md)
- [Numerical And Fixed-Point C](./numerical-and-fixed-point-c.md)
- [Conversions, Promotions, And Aliasing](../semantics-and-memory/conversions-promotions-and-aliasing.md)
- [Performance And Code Size](./performance-and-code-size.md)
- [Compiler And Vendor Extensions](../platform-specific-c/compiler-and-vendor-extensions.md)
- [Advanced Data Structures](./advanced-data-structures.md)

## References

- [C11 draft N1570](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n1570.pdf)
- [IEEE 754 floating-point standard](https://ieeexplore.ieee.org/document/8766227)
- [C floating-point environment](https://en.cppreference.com/w/c/numeric/fenv)
- [Arm CMSIS-DSP documentation](https://arm-software.github.io/CMSIS-DSP/latest/)
- [GCC floating-point options](https://gcc.gnu.org/onlinedocs/gcc/Optimize-Options.html)
- The target FPU/DSP manual, compiler floating-point ABI, math library, and numerical
  requirements specification

---
status: draft
reviewed: false
domain: c
difficulty: advanced
last_reviewed: null
---

# Compiler Optimization And Undefined Behavior

The compiler does not translate C by preserving every source-level step. It translates
the behavior permitted by the C abstract machine into a faster implementation. When a
program executes undefined behavior (UB), the standard imposes no requirements on the
result, and the optimizer may use the assumption that the UB path never occurs. This is
why a bug can disappear at `-O0`, appear at `-O2`, and change again when a log statement
or an unrelated function is added.

## Learning Objectives

- Classify undefined, implementation-defined, unspecified, and strictly conforming
  behavior.
- Understand how aliasing, `restrict`, signed overflow, lifetime, and bounds feed
  optimizer assumptions.
- Distinguish `volatile`, atomics, compiler barriers, and hardware barriers.
- Read optimized assembly and identify inlining, constant propagation, dead-code
  elimination, vectorization, and speculative transformations.
- Use warnings, sanitizers, flags, and reduced test cases without treating a mitigation
  flag as a correctness proof.

## Four Categories Of Behavior

- **Undefined behavior:** the standard places no requirements on the program. Examples
  include signed integer overflow, out-of-bounds access, use-after-lifetime, invalid
  shifts, data races, and dereferencing an invalid pointer.
- **Implementation-defined behavior:** the implementation chooses a behavior and must
  document it, such as the representation of some types or the result of converting an
  out-of-range integer to a signed type.
- **Unspecified behavior:** one of several permitted results may be selected and the
  implementation need not document which one, such as the order of evaluation in some
  expressions.
- **Constraint violation:** a translation diagnostic is required; a successful build
  after a diagnostic does not make the source portable.

Do not describe UB as merely “the compiler may do anything” and stop there. Identify
the violated rule, the optimizer fact it enables, the smallest fix, and the test that
prevents recurrence.

## Undefined Behavior As An Optimization Assumption

If the language says a condition cannot occur in a defined execution, the compiler may
remove checks that depend on the condition or simplify control flow around it. For
example, signed overflow is not required to wrap, so an optimizer may reason that a
loop's signed induction variable cannot overflow during a defined execution. A null
pointer dereference is also not a portable way to probe whether a pointer is null.

Intentionally relying on a target's wraparound instruction requires an explicit unsigned
type, a checked conversion, or an implementation contract such as a compiler option
documented for the project. `-fwrapv` or `-fno-strict-aliasing` can change optimization
assumptions, but they do not repair all existing UB and may create ABI or performance
trade-offs.

## Signed And Unsigned Arithmetic

Unsigned integer arithmetic is defined modulo (2^N) for the type's value width. That
does not mean every unsigned expression is safe: conversion to a smaller type, array
index calculation, allocation-size addition, and signed/unsigned comparison can still
produce a wrong result.

Signed overflow is UB. Use a checked operation when the result must fit:

```c
#include <limits.h>

static int add_int_checked(int left, int right, int *result)
{
    if (result == 0) {
        return -1;
    }
    if ((right > 0 && left > INT_MAX - right) ||
        (right < 0 && left < INT_MIN - right)) {
        return -1;
    }
    *result = left + right;
    return 0;
}
```

For allocation sizes, prefer `size_t` and check before multiplying or adding. For
fixed-width protocols, check the mathematical range before converting. Avoid “clever”
overflow detection that itself performs the overflow it claims to detect.

## Evaluation Order And Unsequenced Effects

Some operators impose sequencing; others do not. A function-call boundary sequences the
evaluation of arguments before entering the function, but the relative order of the
arguments themselves may be unspecified. Modifying an object more than once without a
sequencing relationship is UB.

Use separate statements when order matters. Do not rely on a particular compiler's
argument evaluation order, even if it is stable in one release. Warnings such as
`-Wsequence-point`/`-Wunsequenced` are valuable, but a clean warning result is not a
formal proof for every lifetime or aliasing rule.

## Effective Type, Aliasing, And Object Representation

The optimizer may assume that pointers of unrelated types do not designate the same
object when the C rules prohibit such an access. Character types can inspect an object's
representation, but reading bytes through a cast pointer of an unrelated non-character
type is not a universal type-punning technique.

Use `memcpy` to move representation bytes into a destination object of the intended
type:

```c
#include <stdint.h>
#include <string.h>

static float bits_to_float(uint32_t bits)
{
    float value;
    _Static_assert(sizeof(value) == sizeof(bits), "unexpected float width");
    memcpy(&value, &bits, sizeof(value));
    return value;
}
```

The result still depends on the target's floating-point representation and signaling
behavior. `memcpy` avoids an invalid lvalue access; it does not invent a representation
that the target does not have. For portable serialization, decode fields explicitly.

Union type-punning is treated differently by C and C++ and can have implementation
extensions. If the union is a tagged variant, read only the active member according to
the design. If the goal is representation inspection, use a byte copy and document the
representation contract.

## `restrict` And No-Alias Promises

`restrict` is a promise about how an object is accessed through a pointer during the
relevant execution. If the caller violates the promise, behavior is undefined; the
qualifier is not a hint that can be ignored by the caller.

```c
#include <stddef.h>

void add_f32(size_t count,
             float *restrict destination,
             const float *restrict left,
             const float *restrict right)
{
    for (size_t index = 0u; index < count; ++index) {
        destination[index] = left[index] + right[index];
    }
}
```

The declaration promises valid arrays and non-overlapping access relationships for the
call. It may enable vectorization and load/store reordering. Do not add `restrict` to
silence an aliasing warning or because a benchmark improves; prove the precondition at
the API boundary and test overlapping calls separately if they are needed.

## Lifetime, Bounds, And Pointer Validity

The optimizer can exploit the fact that an object is not alive, an array access is in
bounds, or a pointer is not invalid in any defined execution. Common errors include:

- using a pointer after `free`, scope exit, or storage reuse;
- forming a pointer far outside an array and later bringing it back;
- subtracting pointers from different arrays;
- using a stale pointer after a reallocating operation;
- converting a size through a narrower integer before indexing;
- assuming a one-past pointer can be dereferenced.

Represent a buffer as a pointer plus a validated length. Keep ownership and invalidation
events visible. When a hardware address is involved, add the target's mapping, access
width, alignment, and lifetime contract; a numerically valid address is not necessarily
a valid C object.

## Inlining, Constant Propagation, And Dead-Code Elimination

Inlining substitutes a function body into a caller when profitable or requested. It
can expose constants, eliminate branches, propagate ranges, remove unused fields, and
enable interprocedural alias analysis. It can also increase code size, change cache
behavior, alter debug stepping, and turn a previously separate compiler-boundary
assumption into one optimization unit.

Dead-code elimination may remove code that has no defined observable effect. Ordinary
stores to an object that is never read can disappear. A `volatile` access is observable
to the implementation and is not removed under the rules for volatile accesses, but
volatile is not a general solution for shared memory or lifetime errors.

## Speculation Is Not A C Guarantee

Compilers may speculate computations when the transformed program has the same defined
observable behavior. CPUs may also execute instructions transiently out of order. A
source-level bounds check is necessary for C correctness, but security-sensitive code
may additionally need constant-time design, speculation barriers, masking, privilege
boundaries, or an OS mitigation.

Do not use `volatile` to claim constant-time behavior, and do not infer side-channel
properties from one assembly listing. Treat timing and speculation resistance as a
separate target and threat-model contract.

## Volatile, Atomic, And Barriers

- **`volatile`:** tells the C implementation that accesses are observable and must be
  performed according to volatile rules. It is appropriate for many MMIO registers and
  special memory, but does not provide inter-thread synchronization or atomicity for a
  multi-step operation.
- **C atomics:** provide atomic operations and ordering for C objects shared by C
  execution agents.
- **Compiler barrier:** constrains compiler motion around an interface; it may emit no
  hardware instruction.
- **Hardware barrier:** orders or completes processor/interconnect effects according to
  the ISA; it does not repair a C data race.
- **Device access protocol:** may additionally require register widths, posted-write
  completion, cache maintenance, polling, and reset sequencing.

Name which of these layers a low-level wrapper handles. A function called
`memory_barrier()` without a documented scope is a maintenance hazard.

## Reading Generated Assembly

When source and behavior disagree, compile a minimal reproducer with:

1. the exact C dialect, target, optimization, LTO, and ABI flags;
2. warnings, debug information, and sanitizer settings close to the failure;
3. an assembly listing with source interleaving and a disassembly of the final binary;
4. symbol/map information for section placement and inlining decisions.

Look for removed checks, widened/narrowed arithmetic, vectorized loops, folded loads,
unexpected calls, tail calls, merged identical functions, and changed volatile/atomic
instructions. Compare an optimization-disabled build only as a diagnostic; a bug that
vanishes at `-O0` is still a bug.

## Diagnostics And Flags

Useful tools include:

- warnings for conversion, sign, format, array bounds, strict aliasing, and unsequenced
  operations;
- UBSan for selected arithmetic, bounds, alignment, object-size, and type checks;
- ASan for many spatial and temporal memory errors on supported targets;
- TSan for C data races where its runtime and target are supported;
- static analyzers and formal tools for paths that dynamic tests cannot cover;
- compiler optimization reports and vectorization diagnostics;
- `-fno-strict-aliasing`, `-fwrapv`, or sanitizer recovery options only as explicitly
  reviewed implementation choices.

Sanitizers instrument a model of execution and cannot catch every hardware, DMA,
interrupt, or timing fault. A clean sanitizer run is evidence, not proof.

## Exercises And Diagnostics

1. Reduce a signed-overflow failure to a 20-line program and compare `-O0`, `-O2`, and
   sanitizer output. Explain which assumption changes.
2. Replace a cast-based type-pun with `memcpy`; inspect warnings and assembly on two
   targets and document representation limitations.
3. Add `restrict` to a measured vector operation, write a caller precondition, and test
   both valid and overlapping inputs.
4. Inspect a volatile MMIO wrapper, an atomic publication wrapper, and a compiler
   barrier; annotate exactly what each one does not guarantee.
5. Use an optimization report and disassembly to explain one inlining, one eliminated
   branch, one vectorized loop, and one unexpected library call.

## Common Mistakes

- Treating UB as a predictable wraparound or “works on this compiler” behavior.
- Assuming `-O0` validates correctness.
- Using pointer casts instead of a representation-safe copy or explicit conversion.
- Adding `restrict` without proving the caller-side no-alias contract.
- Using volatile to fix a data race, DMA cache problem, or device transaction.
- Believing a compiler barrier is a CPU/device barrier.
- Assuming a sanitizer or warning set covers interrupt, DMA, ABI, and power failures.
- Ignoring LTO and cross-translation-unit optimization in production builds.

## Related Topics

- [Advanced C overview](./index.md)
- [Conversions, Promotions, And Aliasing](../semantics-and-memory/conversions-promotions-and-aliasing.md)
- [Undefined Behavior](../semantics-and-memory/undefined-behavior.md)
- [Const, Volatile, And Restrict](../semantics-and-memory/qualifiers-const-volatile-restrict.md)
- [Correctness, Quality, And Security](../correctness-quality-and-security/index.md)
- [Compiler And Vendor Extensions](../platform-specific-c/compiler-and-vendor-extensions.md)

## References

- [C11 draft N1570](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n1570.pdf)
- [GCC optimization options](https://gcc.gnu.org/onlinedocs/gcc/Optimize-Options.html)
- [GCC warning options](https://gcc.gnu.org/onlinedocs/gcc/Warning-Options.html)
- [GCC instrumentation options](https://gcc.gnu.org/onlinedocs/gcc/Instrumentation-Options.html)
- [LLVM undefined behavior manual](https://llvm.org/docs/UndefinedBehavior.html)
- [LLVM Language Reference](https://llvm.org/docs/LangRef.html)
- The exact compiler version, optimization flags, ABI, target ISA, and sanitizer
  support matrix used by the project

---
status: draft
reviewed: false
domain: c
difficulty: intermediate
last_reviewed: null
---

# Undefined Behavior

Undefined behavior means that the C standard imposes no requirements when a program reaches a condition classified as undefined. The compiler may assume that such a condition never occurs and transform surrounding code accordingly. A result that appears stable in a debug build is not a contract.

Undefined behavior is different from a hardware fault, although it can lead to one. It is a source-level violation of the language rules that may be exploited by optimization.

## Learning Objectives

- Distinguish undefined, unspecified, implementation-defined, and constraint-violation behavior.
- Recognize common undefined operations in embedded code.
- Understand why optimizers can remove checks after undefined assumptions.
- Use warnings, sanitizers, static analysis, and tests appropriately.
- Avoid treating volatile, casts, or compiler flags as repairs for undefined source.

## Behavior Categories

| Category | Example | What to do |
| --- | --- | --- |
| Defined | Indexing an element inside its array | Rely on the specified rule |
| Implementation-defined | Whether plain char is signed | Read the implementation documentation |
| Unspecified | Which permitted option an implementation chooses | Do not depend on one result |
| Constraint violation | Invalid initializer or incompatible declaration | Fix the diagnostic |
| Undefined | Signed overflow or dereferencing a dangling pointer | Remove the condition completely |

A compiler may document an implementation-defined choice or extension. That documentation narrows behavior for that implementation; it does not create portable ISO C behavior.

## Common Undefined Operations

Examples include:

- signed integer overflow;
- division or remainder by zero;
- invalid shift counts or invalid signed shifts;
- out-of-bounds array access;
- dereferencing null, dangling, invalid, or misaligned pointers;
- reading an uninitialized or indeterminate value where not permitted;
- accessing an object through an incompatible lvalue type;
- violating a restrict contract;
- modifying a scalar more than once without required sequencing;
- use-after-free, double-free, and invalid free;
- data races on non-atomic objects;
- calling a function through an incompatible function-pointer type;
- overflowing allocation-size calculations and then indexing the undersized object.

A complete review asks which object, type, lifetime, bounds, and execution context make the operation valid.

## Signed Overflow

Signed overflow is not required to wrap:

~~~c
#include <limits.h>

int increment(int value)
{
    if (value == INT_MAX) {
        return INT_MAX;
    }
    return value + 1;
}
~~~

The guard makes the operation representable. If wraparound is the intended mathematical behavior, use an unsigned type or a checked operation whose policy is explicit.

Compiler flags such as fwrapv change implementation behavior for some signed arithmetic in a particular build; they should be a documented project decision, not a reason to leave accidental overflow in source.

## Bounds And Pointer Errors

~~~c
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

bool get_byte(const uint8_t *data, size_t length,
              size_t index, uint8_t *result)
{
    if (data == NULL || result == NULL || index >= length) {
        return false;
    }

    *result = data[index];
    return true;
}
~~~

The check must happen before the access, and the length must describe the same live object as data. A pointer copied from an earlier state can be dangling even when the numeric address still looks plausible.

## Unsequenced Side Effects

Do not modify an object more than once in an expression when the sequencing rules do not establish a safe order:

~~~c
#include <stdint.h>

void safe_update(uint32_t *value)
{
    uint32_t old = *value;
    *value = old + 1u;
}
~~~

Split complex expressions into named intermediate steps. This makes both the standard rule and the intended state transition visible.

## Data Races

A volatile object can still participate in a data race. If one task writes a non-atomic object while another reads or writes it without the required synchronization, the C memory model does not provide a defined result.

Use C atomics, RTOS synchronization, interrupt masking, or a platform-specific protocol as appropriate. Choose based on the execution context and memory-ordering requirement, not just on whether the variable is declared volatile.

## Optimizer Consequences

The optimizer may use undefined-behavior assumptions to:

- remove a null check after a dereference;
- eliminate a branch that would require signed overflow;
- reorder or combine accesses under aliasing rules;
- infer that a loop terminates;
- vectorize accesses based on alignment or restrict;
- fold an impossible condition into a constant.

~~~c
#include <stddef.h>

int find_value(const int *data, size_t length, int wanted)
{
    for (size_t i = 0u; i < length; ++i) {
        if (data[i] == wanted) {
            return (int)i;
        }
    }
    return -1;
}
~~~

The caller must satisfy data and length. If data is null even when length is zero and the implementation evaluates an invalid expression through it, the API contract should define whether null empty ranges are permitted and the implementation should avoid forming invalid accesses.

Do not use observed assembly as evidence that undefined source is safe.

## Detection Tools

Use multiple layers:

- compiler warnings for syntax, constraints, conversion, and suspicious constructs;
- UndefinedBehaviorSanitizer for selected runtime checks;
- AddressSanitizer for many bounds and lifetime errors;
- MemorySanitizer where supported for uninitialized reads;
- ThreadSanitizer for host-representable data races;
- static analysis for paths and target-specific rules;
- fuzzing for parser and boundary coverage;
- target hardware faults, MPU diagnostics, and trace for non-host behavior.

Sanitizers are test instrumentation. They do not prove the absence of undefined behavior and may not support a bare-metal target directly.

## Recovering From Diagnostics

When a sanitizer or compiler reports undefined behavior:

1. Preserve the smallest reproducer and exact toolchain options.
2. Identify the violated rule, not only the failing instruction.
3. Find the object, lifetime, bound, conversion, or synchronization precondition.
4. Fix the interface or state transition that allowed the invalid operation.
5. Add a regression test at the boundary.
6. Check optimized target output after the source is defined.

Suppressions and no-sanitize attributes should be narrow, justified, and reviewed. They are not a substitute for an invariant.

## Exercises

1. Write tests around INT_MAX, invalid shifts, zero divisors, and empty buffers.
2. Run AddressSanitizer and UndefinedBehaviorSanitizer on a deliberately faulty parser.
3. Create a race on a host and fix it with an atomic or mutex; explain why volatile alone is insufficient.
4. Compare optimized and unoptimized assembly for defined code and for a deliberately invalid example.
5. Add checked arithmetic helpers for packet length and allocation size.
6. Build a project rule that treats selected undefined-behavior warnings as errors.

## Common Mistakes

- Saying undefined behavior means “the hardware will wrap.”
- Treating one compiler’s result as a language guarantee.
- Assuming debug and optimized builds differ only in speed.
- Using volatile to repair a race.
- Using a cast, pragma, or no-sanitize attribute to hide a violation.
- Checking a pointer after dereferencing it.
- Assuming sanitizers cover hardware registers, DMA, or every concurrency path.
- Fixing the symptom while leaving the invalid ownership or bounds contract.

## Debugging Checklist

1. Capture compiler version, standard mode, optimization, and target.
2. Reduce the failure to one operation and one violated precondition.
3. Run sanitizers and static analysis on a host-equivalent reproducer.
4. Inspect warnings with conversion, bounds, alignment, and sequencing enabled.
5. Test both boundary values and error paths.
6. Trace lifetime and ownership across task, interrupt, and DMA boundaries.
7. Review optimizer assumptions such as strict aliasing, alignment, and finite loops.
8. Add a regression test and document any justified platform-specific exception.

## Related Topics

- [Semantics And Memory overview](./index.md)
- [Conversions, Promotions, And Aliasing](./conversions-promotions-and-aliasing.md)
- [Pointer Arithmetic And Bounds](./pointer-arithmetic-and-bounds.md)
- [Memory Safety And Lifetime](./memory-safety-and-lifetime.md)
- [Correctness, Quality, And Security](../correctness-quality-and-security/index.md)
- [C Programming](../index.md)

## References

- [ISO/IEC 9899 standards and drafts](https://open-std.org/jtc1/sc22/wg14/www/standards.html)
- [C11 public draft N1570](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n1570.pdf)
- [Clang UndefinedBehaviorSanitizer](https://clang.llvm.org/docs/UndefinedBehaviorSanitizer.html)
- [GCC warning options](https://gcc.gnu.org/onlinedocs/gcc/Warning-Options.html)
- [CERT C undefined-behavior rules](https://wiki.sei.cmu.edu/confluence/display/c)

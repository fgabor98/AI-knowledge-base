---
status: draft
reviewed: false
domain: c
difficulty: intermediate
last_reviewed: null
---

# Conversions, Promotions, And Aliasing

C performs many conversions implicitly. Some preserve a value, some change its range, some change its representation, and some alter which object accesses are legal. Promotions and usual arithmetic conversions are responsible for many embedded comparison and register-mask bugs; aliasing rules are responsible for many optimizer surprises.

## Learning Objectives

- Explain integer promotions and usual arithmetic conversions.
- Recognize signed and unsigned comparison hazards.
- Make narrowing conversions explicit and checked.
- Distinguish pointer conversion from pointer dereference validity.
- Explain effective type and strict aliasing at a practical level.
- Use character access and memcpy for representation operations.
- Establish a project policy for conversions and aliases.

## Integer Promotions

Integer types narrower than int are usually promoted before arithmetic, comparison, and many argument operations:

<!-- c-check: expect-warning -->

~~~c
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

bool impossible(uint8_t value)
{
    return value < 0;
}

int sum_bytes(uint8_t left, uint8_t right)
{
    return left + right;
}
~~~

The uint8_t operands are promoted to int when int can represent all their values. The result of sum_bytes is int, not uint8_t. A destination assignment can narrow it later.

The promotion rules depend on the implementation’s integer ranges. Do not infer the result only from the typedef spelling.

## Usual Arithmetic Conversions

When two arithmetic operands meet, C converts them to a common type. Signedness and rank matter:

<!-- c-check: expect-warning -->

~~~c
#include <stdbool.h>

bool surprising(int signed_value, unsigned int unsigned_value)
{
    return signed_value < unsigned_value;
}
~~~

If the unsigned type’s rank is at least the signed type’s rank, the signed operand can convert to unsigned. A negative value then becomes a large positive value modulo the unsigned range.

Choose one arithmetic domain before writing the expression:

~~~c
#include <stdbool.h>
#include <stdint.h>

bool below_limit(uint32_t value, uint32_t limit)
{
    return value < limit;
}
~~~

When comparing a signed measurement with an unsigned limit, validate the signed range or convert both to an explicitly chosen wider type.

## Narrowing And Range Checks

A cast documents conversion but does not check it:

~~~c
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

bool to_u8(uint16_t value, uint8_t *result)
{
    if (result == NULL || value > UINT8_MAX) {
        return false;
    }

    *result = (uint8_t)value;
    return true;
}
~~~

For signed-to-unsigned conversion, check the mathematical range before converting. For floating-to-integer conversion, check finite range and rounding policy. For integer-to-signed conversion, ensure the value is representable.

Use compiler conversion warnings as review prompts. Do not silence a warning with a cast until the range and policy are written down.

## Constant Conversions

Integer constants have types selected from candidate types based on base, value, and suffix. Make constants match their intended domain:

~~~c
#include <stdint.h>

uint32_t timeout_ticks = 1000u;
uint32_t mask = UINT32_C(1) << 7;
~~~

Unsuffixed constants can become signed int or a wider signed type. A shift or comparison can then occur in an unintended type. Use suffixes and standard constant macros where width matters.

## Pointer Conversions

Object pointers can convert to and from void pointers. Qualified pointers can gain qualifiers implicitly, but removing qualifiers requires an explicit cast and a safety proof:

~~~c
#include <stdint.h>

void accept_bytes(const uint8_t *data);

void pass_word(uint32_t *word)
{
    accept_bytes((const uint8_t *)word);
}
~~~

The conversion does not establish that reading the object as bytes is useful for the intended representation, nor that the receiving function has the correct length. Pointer alignment and lifetime still apply.

Converting between unrelated object pointer types and dereferencing the result can violate alignment, effective type, or aliasing rules. A cast changes the static type; it does not change the object.

## Effective Type And Access Paths

Allocated storage has no declared type. Its effective type is established by how it is written or accessed, subject to the standard rules. Declared objects have their declared type, and accesses through incompatible lvalue types can be undefined.

A practical rule is:

- access an object through its declared type or a compatible qualified type;
- use a character type to inspect or copy its representation;
- use memcpy to move representation bytes into an object of a different declared type;
- use a union only under a documented, implementation-aware type-punning policy;
- do not cast a byte buffer to an unrelated structure and dereference it.

~~~c
#include <stdint.h>
#include <string.h>

float decode_float(const uint8_t bytes[sizeof(float)])
{
    float result;
    memcpy(&result, bytes, sizeof result);
    return result;
}
~~~

The bytes still need to be produced according to the target floating-point representation. memcpy avoids an invalid lvalue access; it does not make arbitrary bytes a meaningful float.

## Strict Aliasing And Optimization

Compilers may assume that pointers to incompatible types do not designate the same object, enabling load elimination, vectorization, and reordering. GCC enables strict-aliasing optimizations at common higher optimization levels.

~~~c
#include <stddef.h>

void add_in_place(size_t length,
                  int *restrict destination,
                  const int *restrict source)
{
    for (size_t i = 0u; i < length; ++i) {
        destination[i] += source[i];
    }
}
~~~

The restrict contract makes non-overlap explicit. Even without restrict, accessing the same object through incompatible types is a separate aliasing problem.

Do not diagnose an aliasing bug by observing one optimization level. The source rule determines validity; generated behavior can change with compiler version, link-time optimization, target, and debug settings.

## Character Access

A character type may inspect the object representation of another object:

~~~c
#include <stddef.h>

void copy_representation(const void *object,
                         unsigned char *bytes,
                         size_t size)
{
    const unsigned char *source = object;
    for (size_t i = 0u; i < size; ++i) {
        bytes[i] = source[i];
    }
}
~~~

This is useful for diagnostics and byte copying, but padding bytes may be unspecified and the representation may not be portable.

## Conversion Policy

A mature project defines rules such as:

- no implicit signed/unsigned comparison in public interfaces;
- all narrowing conversions occur after range checks;
- fixed-width data uses stdint types and matching format macros;
- byte order is explicit at serialization boundaries;
- casts between unrelated pointers require a documented platform reason;
- restrict is used only on APIs with tested non-overlap contracts;
- memcpy or explicit shifts are used for representation changes;
- conversion warnings are enabled and reviewed.

A policy should permit well-understood low-level code while requiring evidence for exceptions.

## Exercises

1. Predict the type of mixed signed and unsigned expressions on two targets with different int widths.
2. Build a checked conversion library for signed, unsigned, and fixed-width values.
3. Compare an invalid type-punning cast with a memcpy-based representation conversion.
4. Add restrict to a non-overlapping array API and test that its precondition is enforced.
5. Search a driver for casts between integer and pointer types; classify each as representation, address, or bug.
6. Enable conversion and strict-aliasing warnings and resolve every new diagnostic deliberately.

### Model Answers

Try each exercise before opening its answer. Where an exercise asks you to build
something, the answer shows one good solution, not the only one.

??? note "Exercise 1: the same expressions with 32-bit and 16-bit int"
    With `uint16_t a = 1; int32_t s = 1; uint32_t w = 1;`. The 32-bit column was checked with
    GCC 13 on x86-64. The 16-bit column follows from the rules, for a target where `int` is
    16 bits and `int32_t` is `long`, as on MSP430:

    | Expression | 32-bit `int` | 16-bit `int` |
    | --- | --- | --- |
    | `a + a` | `int` | `unsigned int` |
    | `s + a` | `int` | `long` (it can represent every 16-bit `unsigned int`) |
    | `-1 < a` | 1 (`int` comparison) | 0 (`-1` converts to 65535) |
    | `w * a` | `unsigned int` | `unsigned long` |

    Integer promotion converts `uint16_t` to `int` only when `int` can represent all of its
    values (C11 6.3.1.1p2), which is true for 32-bit `int` and false for 16-bit `int`. The
    usual arithmetic conversions then decide between signed and unsigned (6.3.1.8p1). Code
    shared between an MCU and a host test therefore needs explicit types in comparisons and
    arithmetic. See also [test-out Q2](./test-out.md#q2-comparing-signed-and-unsigned).

??? note "Exercise 2: a checked conversion library"
    ```c
    #include <assert.h>
    #include <stdbool.h>
    #include <stdint.h>

    /* Each returns false and leaves *out unchanged if the value does not fit. */
    bool i32_to_u16(int32_t value, uint16_t *out)
    {
        if (value < 0 || value > UINT16_MAX) {
            return false;
        }
        *out = (uint16_t)value;
        return true;
    }

    bool u32_to_i32(uint32_t value, int32_t *out)
    {
        if (value > (uint32_t)INT32_MAX) {
            return false;
        }
        *out = (int32_t)value;
        return true;
    }

    bool i64_to_i16(int64_t value, int16_t *out)
    {
        if (value < INT16_MIN || value > INT16_MAX) {
            return false;
        }
        *out = (int16_t)value;
        return true;
    }

    int main(void)
    {
        uint16_t u16 = 7;
        int32_t i32 = 7;
        int16_t i16 = 7;

        assert(i32_to_u16(0, &u16) && u16 == 0);
        assert(i32_to_u16(UINT16_MAX, &u16) && u16 == UINT16_MAX);
        assert(!i32_to_u16(-1, &u16) && u16 == UINT16_MAX);        /* unchanged */
        assert(!i32_to_u16(UINT16_MAX + 1, &u16));
        assert(u32_to_i32((uint32_t)INT32_MAX, &i32) && i32 == INT32_MAX);
        assert(!u32_to_i32((uint32_t)INT32_MAX + 1u, &i32));
        assert(i64_to_i16(INT16_MIN, &i16) && i16 == INT16_MIN);
        assert(!i64_to_i16(INT16_MIN - 1, &i16) && i16 == INT16_MIN);
        return 0;
    }
    ```

    Every check compares against the *destination* limits in a type where both operands are
    exact. In `value > (uint32_t)INT32_MAX` both sides are unsigned. In the signed cases both
    sides are signed, so no comparison changes sign. Tests sit exactly at each limit and one
    past it.

??? note "Exercise 3: type punning by cast versus memcpy"
    ```c
    #include <stdint.h>
    #include <string.h>

    uint32_t float_bits_cast(float value)
    {
        return *(uint32_t *)&value;              /* invalid: float object read as uint32_t */
    }

    uint32_t float_bits_copy(float value)
    {
        uint32_t bits;

        _Static_assert(sizeof bits == sizeof value, "float must be 32 bits");
        memcpy(&bits, &value, sizeof bits);      /* copies the representation */
        return bits;
    }
    ```

    At `-O2`, GCC 13 warns about the first function:
    `dereferencing type-punned pointer will break strict-aliasing rules [-Wstrict-aliasing]`.
    There is no warning at `-O0`, because the check runs only with `-fstrict-aliasing`.
    The cast reads a `float` object through a `uint32_t` lvalue, which the effective-type
    rules forbid. The `memcpy` version is defined, and it costs nothing:

    ```text
    float_bits_copy:
    	movd	%xmm0, %eax
    	ret
    ```

    The copy only moves bytes. Whether those bytes are IEEE 754 binary32 is still a
    property of the target, so a portable decoder documents that assumption.

??? note "Exercise 4: restrict with a checked precondition"
    ```c
    #include <assert.h>
    #include <stdbool.h>
    #include <stddef.h>
    #include <stdint.h>

    /* Compares addresses as integers: comparing pointers into different objects with <
       is undefined, but uintptr_t comparison is well behaved on flat-memory targets. */
    static bool ranges_overlap(const void *a, size_t a_size, const void *b, size_t b_size)
    {
        uintptr_t a_start = (uintptr_t)a;
        uintptr_t b_start = (uintptr_t)b;

        return a_start < b_start + b_size && b_start < a_start + a_size;
    }

    /* Precondition: destination and source do not overlap. */
    void scale_into(int32_t *restrict destination, const int32_t *restrict source,
                    size_t count, int32_t factor)
    {
        assert(!ranges_overlap(destination, count * sizeof *destination,
                               source, count * sizeof *source));
        for (size_t i = 0; i < count; i++) {
            destination[i] = source[i] * factor;
        }
    }

    int main(void)
    {
        int32_t a[4] = { 1, 2, 3, 4 };
        int32_t b[4];

        assert(!ranges_overlap(a, sizeof a, b, sizeof b));
        assert(ranges_overlap(a, sizeof a, a + 3, sizeof a[0]));       /* last element shared */
        assert(!ranges_overlap(a, 2 * sizeof a[0], a + 2, 2 * sizeof a[0]));  /* adjacent */
        scale_into(b, a, 4, 10);
        assert(b[3] == 40);
        return 0;
    }
    ```

    The compiler never checks `restrict`. It trusts the promise and optimizes on it, so a
    violation is silent undefined behavior. The debug assertion turns the promise into a
    checked precondition. `ranges_overlap` is tested directly, because an assertion failure
    aborts the test. Calling `scale_into(a + 1, a, 3, 10)` in a debug build fails the
    assertion. In release builds the assertion is compiled out, and so is the protection.

??? note "Exercise 5: classifying integer-pointer casts"
    Find the casts with two searches: integer casts of pointers,
    `grep -nE '\((u?intptr_t|u?int(32|64)_t|unsigned( long)?)\)' drivers/foo/*.c`,
    and pointer casts of integers, `grep -nE '\*\)\s*\(?(0x|[A-Z_]+_BASE|\(u?intptr_t\))' drivers/foo/*.c`,
    then classify each hit:

    | Cast in the driver | Class | Verdict |
    | --- | --- | --- |
    | `(volatile uint32_t *)(UART0_BASE + 0x04)` | address: a documented MMIO register | valid; write the four facts next to it (see Expressions exercise 5) |
    | `((uintptr_t)buffer & 31u) == 0` | representation: inspects address bits | valid on flat-memory targets; uses `uintptr_t`, not `uint32_t` |
    | `dma->src = (uint32_t)buffer;` | address handed to hardware | valid only on a 32-bit target, and only if the CPU and DMA see the same address; needs `_Static_assert(sizeof(void *) == 4)` or a translation function |
    | `register_callback(cb, (void *)(intptr_t)channel)` | representation: a small integer smuggled through `void *` | implementation-defined but common; prefer a pointer to a real context |
    | `struct ctx *c = (struct ctx *)(uint32_t)arg;` | bug | truncates a 64-bit pointer in host tests; keep it as `void *` |

    The bug class usually shows up when the driver is first built for a 64-bit host:
    `-Wpointer-to-int-cast` and `-Wint-to-pointer-cast` (both in GCC's defaults) flag
    casts between pointers and narrower integers.

??? note "Exercise 6: resolving conversion warnings deliberately"
    <!-- c-check: expect-warning -->

    ```c
    #include <stddef.h>
    #include <stdint.h>

    uint8_t average(const uint8_t *samples, size_t count)
    {
        unsigned int sum = 0;

        for (int i = 0; i < count; i++) {
            sum += samples[i];
        }
        return sum / count;
    }

    int16_t to_centi(int32_t milli)
    {
        return milli / 10;
    }
    ```

    ```text
    $ gcc -std=c17 -Wall -Wextra -Wconversion -Wsign-conversion -c conv.c   # aka types trimmed
    conv.c:8:23: warning: comparison of integer expressions of different signedness: 'int' and 'size_t' [-Wsign-compare]
    conv.c:11:16: warning: conversion from 'size_t' to 'uint8_t' may change value [-Wconversion]
    conv.c:16:18: warning: conversion from 'int' to 'int16_t' may change value [-Wconversion]
    ```

    Each warning gets a decision rather than a cast:

    ```c
    #include <stdbool.h>
    #include <stddef.h>
    #include <stdint.h>

    /* count must be > 0; the average of uint8_t values always fits in uint8_t. */
    uint8_t average(const uint8_t *samples, size_t count)
    {
        uint32_t sum = 0;

        for (size_t i = 0; i < count; i++) {     /* index type matches the bound */
            sum += samples[i];
        }
        return (uint8_t)(sum / count);           /* cast after a proven range */
    }

    /* Fails for values outside int16_t, instead of silently wrapping. */
    bool to_centi(int32_t milli, int16_t *centi)
    {
        int32_t value = milli / 10;

        if (value < INT16_MIN || value > INT16_MAX) {
            return false;
        }
        *centi = (int16_t)value;
        return true;
    }
    ```

    - Line 8: the index was the wrong type. Changing it removes the warning and the bug.
    - Line 11: the value provably fits, so the cast is correct, and the comment says why.
      (`sum` would overflow only past about 16 million samples.)
    - Line 16: the value does not always fit, so the interface changes to report failure.

    `-Wstrict-aliasing` is on in `-Wall` and needs `-O2` (or `-fstrict-aliasing`) to run.
    Fix those warnings with `memcpy` (exercise 3), never with `-fno-strict-aliasing`,
    unless the whole project has decided to build that way.

## Common Mistakes

- Assuming a small integer stays small during arithmetic.
- Comparing negative signed values with unsigned values.
- Casting without checking range, alignment, or lifetime.
- Treating void pointer conversion as proof of compatible representation.
- Dereferencing a packet buffer as an unrelated structure.
- Using union or pointer casts for type-punning without a target policy.
- Assuming memcpy makes invalid bytes valid typed values.
- Adding restrict when callers can legally pass overlapping storage.
- Relying on one optimization level to define an aliasing result.

## Debugging Checklist

1. Write down the type of every operand after promotion.
2. Compile with sign-conversion and conversion warnings.
3. Add tests at signed, unsigned, and width boundaries.
4. Replace casts with checked conversion helpers where possible.
5. Inspect effective type and access path for every byte-to-object conversion.
6. Compare optimized and unoptimized builds only after finding the source rule violation.
7. Use compiler aliasing diagnostics and sanitizers where supported.
8. Document the ABI or hardware reason for every integer-pointer conversion.

## Related Topics

- [Semantics And Memory overview](./index.md)
- [Types, Values, And Objects](../language-fundamentals/types-values-and-objects.md)
- [Pointer Fundamentals](./pointer-fundamentals.md)
- [Const, Volatile, And Restrict](./qualifiers-const-volatile-restrict.md)
- [Object Representation, Alignment, And Padding](./object-representation-alignment-and-padding.md)
- [C Programming](../index.md)
- [Case study 05: Unsigned Short Promotion](../failure-case-studies/05-unsigned-short-promotion.md)
- [Case study 06: read() Error Passes A Length Check](../failure-case-studies/06-read-error-passes-length-check.md)
- [Case study 08: Strict Aliasing Type Pun](../failure-case-studies/08-strict-aliasing-type-pun.md)

## References

- [ISO/IEC 9899 standards and drafts](https://open-std.org/jtc1/sc22/wg14/www/standards.html)
- [C11 public draft N1570](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n1570.pdf)
- [GCC optimize options and strict aliasing](https://gcc.gnu.org/onlinedocs/gcc/Optimize-Options.html)
- [GCC warning options](https://gcc.gnu.org/onlinedocs/gcc/Warning-Options.html)
- [CERT C integer and expression rules](https://wiki.sei.cmu.edu/confluence/display/c)

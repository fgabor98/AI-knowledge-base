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

## References

- [ISO/IEC 9899 standards and drafts](https://open-std.org/jtc1/sc22/wg14/www/standards.html)
- [C11 public draft N1570](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n1570.pdf)
- [GCC optimize options and strict aliasing](https://gcc.gnu.org/onlinedocs/gcc/Optimize-Options.html)
- [GCC warning options](https://gcc.gnu.org/onlinedocs/gcc/Warning-Options.html)
- [CERT C integer and expression rules](https://wiki.sei.cmu.edu/confluence/display/c)

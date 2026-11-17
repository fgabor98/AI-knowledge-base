---
status: draft
reviewed: false
domain: c
difficulty: beginner
last_reviewed: null
---

# Expressions And Operators

An expression combines operands and operators to produce a value, a side effect, or both. Embedded C relies heavily on expressions for masks, register fields, timeout arithmetic, validation, and compact state updates. The same compactness can hide conversions, precedence mistakes, and unsequenced side effects.

The reliable habit is to ask three questions: what is the type of the expression, how is it grouped, and when are its operands evaluated?

## Learning Objectives

- Classify common C operators by purpose and precedence.
- Distinguish grouping from evaluation order.
- Predict integer promotions and usual arithmetic conversions in common cases.
- Use short-circuit operators intentionally.
- Write masks and shifts without signedness surprises.
- Recognize dangerous or unsequenced side effects.
- Use casts to document a deliberate conversion rather than hide a warning.

## Operator Families

A practical high-to-low precedence summary is:

| Level | Operators | Typical role |
| --- | --- | --- |
| Postfix | brackets, call, dot, arrow, increment | Access and call |
| Unary | sign, logical/bitwise negation, address, dereference, casts, sizeof | Single-operand operations |
| Multiplicative | multiply, divide, remainder | Arithmetic |
| Additive | plus, minus | Arithmetic |
| Shift | left and right shift | Bit movement |
| Relational | less-than and greater-than forms | Ordering |
| Equality | equal and not-equal | Equality |
| Bitwise | and, xor, or | Masks and bit fields |
| Logical | and-and, or-or | Conditions with short-circuiting |
| Conditional | question-colon | Select one expression |
| Assignment | assignment and compound assignment | Store a result |
| Comma | comma operator | Explicit sequencing |

Precedence answers which operands belong to which operator. It does not answer which function call or operand executes first. Add parentheses for intent.

## Values, Conversions, And Promotions

Small integer types are usually promoted to int or unsigned int before many arithmetic and comparison operations. Usual arithmetic conversions then bring two operands to a common type:

<!-- c-check: expect-warning -->

~~~c
#include <stdbool.h>
#include <stdint.h>

bool less_than_zero(uint8_t value)
{
    return value < 0;
}

uint32_t add_count(uint16_t left, uint16_t right)
{
    return (uint32_t)left + (uint32_t)right;
}
~~~

The first condition is always false for a uint8_t value after promotion. The explicit casts in add_count make the intended arithmetic domain visible, but the destination range must still be sufficient.

Mixed signed and unsigned arithmetic can change a comparison:

<!-- c-check: expect-warning -->

~~~c
#include <stdbool.h>

bool surprising(int signed_value, unsigned int unsigned_value)
{
    return signed_value < unsigned_value;
}
~~~

Do not fix mixed arithmetic with random casts. Decide the mathematical domain, convert operands deliberately, and test boundary values.

## Arithmetic Operators

For integer operands:

- division truncates toward zero;
- the remainder follows the sign of the left operand;
- division by zero is undefined;
- signed overflow is undefined;
- unsigned arithmetic wraps modulo one more than the maximum.

A common unsigned timer pattern is:

~~~c
#include <stdbool.h>
#include <stdint.h>

bool elapsed(uint32_t now, uint32_t start, uint32_t timeout)
{
    return (uint32_t)(now - start) >= timeout;
}
~~~

This is appropriate when an interval is shorter than one complete counter wrap. If a timer can remain pending longer, use a wider time base or a different design.

Inspect compiler output before manually replacing division with shifts or reciprocal arithmetic. A rewrite can change rounding or overflow behavior.

## Relational And Logical Operators

Relational and equality operators produce int values of zero or one:

<!-- c-check: skip -- expression fragment -->

~~~c
if (temperature_c > limit_c && !sensor_fault) {
    start_heater();
}
~~~

Logical and and logical or evaluate left to right and short-circuit:

- a && b does not evaluate b when a is false;
- a || b does not evaluate b when a is true.

This can guard an access:

<!-- c-check: skip -- expression fragment -->

~~~c
if (count < capacity && data[count] == expected) {
    ++count;
}
~~~

The guard is valid only when count, capacity, and data describe the same buffer and the access is not concurrently invalidated.

The logical not operator produces a normalized Boolean result. A direct comparison is often clearer than double negation.

## Bitwise Operators And Shifts

Use bitwise operators for flags and representations, not Boolean logic:

~~~c
#include <stdbool.h>
#include <stdint.h>

enum {
    STATUS_READY = 1u << 0,
    STATUS_ERROR = 1u << 1,
    STATUS_DMA = 1u << 2
};

bool status_has_error(uint32_t status)
{
    return (status & STATUS_ERROR) != 0u;
}

uint32_t status_set_ready(uint32_t status)
{
    return status | STATUS_READY;
}

uint32_t status_clear_ready(uint32_t status)
{
    return status & ~STATUS_READY;
}
~~~

Use unsigned types for masks and shifts unless signed behavior is deliberate. A shift count that is negative or at least the width of the promoted left operand is invalid. Left-shifting a signed value into an unrepresentable result is also dangerous.

Make field extraction and insertion explicit:

~~~c
#include <stdint.h>

uint32_t extract_mode(uint32_t register_value)
{
    const uint32_t mode_mask = 0x7u;
    const unsigned mode_shift = 4u;
    return (register_value >> mode_shift) & mode_mask;
}

uint32_t insert_mode(uint32_t register_value, uint32_t mode)
{
    const uint32_t mode_mask = 0x7u;
    const unsigned mode_shift = 4u;
    const uint32_t field_mask = mode_mask << mode_shift;

    return (register_value & ~field_mask)
         | ((mode & mode_mask) << mode_shift);
}
~~~

Masking mode before insertion prevents unrelated high bits from leaking into adjacent fields.

## Assignment And Compound Assignment

Assignment stores the converted right-hand value in the left-hand object and itself has a value. Compound assignment evaluates its left operand once conceptually:

~~~c
#include <stdint.h>

enum {
    STATUS_READY = 1u << 0,
    STATUS_ERROR = 1u << 1
};

uint32_t flags = 0u;
void update_flags(void)
{
    flags |= STATUS_READY;
    flags &= ~STATUS_ERROR;
}
~~~

Use assignments as statements when possible. Chained assignment is legal but can obscure types and error handling. Do not assign to an unmodifiable object, an object outside its lifetime, or shared state without the required synchronization.

## Conditional And Comma Operators

The conditional operator selects one of two expressions:

~~~c
#include <stdbool.h>

const char *state_name(bool ready)
{
    return ready ? "ready" : "waiting";
}
~~~

Only the selected second or third operand is evaluated. The resulting type follows conditional-operator rules, including pointer compatibility and arithmetic conversions.

The comma operator evaluates its left operand, discards its value, then evaluates and yields the right operand. It is not the same as commas separating function arguments:

~~~c
#include <stddef.h>
#include <stdint.h>

void copy_elements(uint8_t *buffer, const uint8_t *source, size_t length)
{
for (size_t i = 0u, j = 0u; i < length; ++i, ++j) {
    buffer[i] = source[j];
}
}
~~~

Use a normal block when a comma expression would make safety-critical code harder to inspect.

## Increment, Decrement, And Side Effects

Prefix and postfix increment both modify an object but produce different values:

~~~c
#include <stdint.h>

void show_increment(void)
{
    uint32_t index = 0u;
    uint32_t old_index = index++;
    uint32_t new_index = ++index;
    (void)old_index;
    (void)new_index;
}
~~~

Avoid combining multiple modifications of the same scalar in one expression:

~~~c
/* Do not depend on an unsequenced modification. */
// value = counter++ + counter++;
~~~

One important state change per statement is easier to review and debug.

## Casts

A cast requests a conversion or changes the type used to interpret an expression. It does not repair an invalid pointer, enlarge a buffer, or make hardware access atomic.

Good uses include converting a validated value to an API type, documenting deliberate narrowing, selecting unsigned arithmetic, and converting a byte before shifting:

~~~c
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

bool to_u8(uint16_t value, uint8_t *out)
{
    if (out == NULL || value > UINT8_MAX) {
        return false;
    }

    *out = (uint8_t)value;
    return true;
}
~~~

A cast that merely silences a warning without addressing range, lifetime, alignment, or aliasing is a defect disguised as clarity.

## sizeof And Unevaluated Expressions

sizeof normally does not evaluate its operand:

~~~c
#include <stddef.h>

size_t allocation_size(size_t count)
{
    return count * sizeof(int);
}
~~~

Variable-length arrays are a special case where evaluating a size expression can be required. Keep size calculations simple and avoid side effects inside sizeof.

sizeof(char) is one C byte; it is not necessarily one octet unless CHAR_BIT is eight.

## Exercises

1. Predict type and value for expressions involving uint8_t, int, and unsigned int, then compile with conversion warnings.
2. Implement set, clear, toggle, and test operations for a uint32_t flag word.
3. Write a wraparound-safe timeout test near UINT32_MAX.
4. Refactor a dense expression containing three increments into simple statements.
5. Find a cast in a driver and document the range, alignment, and ownership facts that make it safe.
6. Parenthesize a mixed bitwise, equality, and logical expression, then test every branch.

### Model Answers

Try each exercise before opening its answer. Where an exercise asks you to build
something, the answer shows one good solution, not the only one.

??? note "Exercise 1: predicting types and values"
    With `uint8_t u8 = 200; int i = -1; unsigned int u = 1;` on a target with 32-bit `int`
    (verified with GCC 13 on x86-64 using `_Generic` to print the types):

    | Expression | Type | Value | Why |
    | --- | --- | --- | --- |
    | `u8 + u8` | `int` | 400 | both operands promote to `int`; no wraparound at 255 |
    | `u8 * u8` | `int` | 40000 | fits in 32-bit `int`; with 16-bit `int` it overflows, which is undefined |
    | `i + u` | `unsigned int` | 0 | `i` converts to `UINT_MAX`, and the sum wraps |
    | `i < u` | `int` | 0 | the comparison happens in `unsigned int` |
    | `~u8` | `int` | -201 | promotion first, then the complement of 0x000000C8 |
    | `u8 << 24` | `int` | undefined | 200 × 2^24 does not fit in `int`; shifting into the sign bit is undefined |

    Two patterns cover most surprises: narrow types are promoted *before* the operator runs,
    and when a signed and an unsigned operand of the same rank meet, the signed one converts.
    `-Wconversion` and `-Wsign-compare` report many of the risky cases; `~u8` comparisons are
    caught by `-Wsign-compare` as well.

??? note "Exercise 2: flag-word operations"
    ```c
    #include <stdbool.h>
    #include <stdint.h>

    static inline uint32_t flag_bit(unsigned int bit)
    {
        return UINT32_C(1) << (bit & 31u);    /* bit must be 0..31 */
    }

    static inline void flag_set(uint32_t *flags, unsigned int bit)
    {
        *flags |= flag_bit(bit);
    }

    static inline void flag_clear(uint32_t *flags, unsigned int bit)
    {
        *flags &= ~flag_bit(bit);
    }

    static inline void flag_toggle(uint32_t *flags, unsigned int bit)
    {
        *flags ^= flag_bit(bit);
    }

    static inline bool flag_test(uint32_t flags, unsigned int bit)
    {
        return (flags & flag_bit(bit)) != 0u;
    }
    ```

    - `UINT32_C(1)` makes the shift unsigned and 32 bits wide. `1 << 31` with a plain `int`
      shifts into the sign bit, which is undefined.
    - `~flag_bit(bit)` is computed in `uint32_t`, so clearing bit 31 leaves the other 31 bits
      intact. With a narrower type, the complement would be promoted to `int` first.
    - Masking the bit number (`& 31u`) keeps the shift defined if a caller passes 32 or more.
      Many projects prefer an assertion instead, so the bad call is found rather than wrapped.
    - `flag_test` returns `bool`, so any set bit becomes `true`. Returning `flags & mask`
      through a narrower type such as `uint8_t` would drop bits 8 to 31: testing bit 9
      would return 0 even when the bit is set.
    - These read-modify-write operations are not atomic. If an interrupt or another thread
      changes the same word, use `atomic_fetch_or`/`atomic_fetch_and` or a critical section.

??? note "Exercise 3: a wraparound-safe timeout test"
    ```c
    #include <stdbool.h>
    #include <stdint.h>
    #include <stdio.h>

    static bool expired(uint32_t now, uint32_t start, uint32_t timeout)
    {
        return (uint32_t)(now - start) >= timeout;
    }

    static bool expired_naive(uint32_t now, uint32_t start, uint32_t timeout)
    {
        return now >= start + timeout;
    }

    int main(void)
    {
        const uint32_t start = UINT32_MAX - 5u;   /* 6 ticks before the counter wraps */
        const uint32_t timeout = 10u;
        const uint32_t samples[] = { start, start + 5u, start + 6u, start + 9u, start + 10u, start + 11u };

        for (size_t i = 0; i < sizeof samples / sizeof samples[0]; i++) {
            uint32_t now = samples[i];
            printf("now=0x%08x elapsed=%2u  expired=%d  naive=%d\n", (unsigned)now,
                   (unsigned)(uint32_t)(now - start), expired(now, start, timeout),
                   expired_naive(now, start, timeout));
        }
        return 0;
    }
    ```

    ```text
    now=0xfffffffa elapsed= 0  expired=0  naive=1
    now=0xffffffff elapsed= 5  expired=0  naive=1
    now=0x00000000 elapsed= 6  expired=0  naive=0
    now=0x00000003 elapsed= 9  expired=0  naive=0
    now=0x00000004 elapsed=10  expired=1  naive=1
    now=0x00000005 elapsed=11  expired=1  naive=1
    ```

    `expired` is correct across the wrap: unsigned subtraction gives the elapsed ticks modulo
    2^32, which is right as long as the real elapsed time is below 2^32 ticks. The naive
    version computes `start + timeout`, which wraps to 4, so it reports *expired* at the very
    start. The boundary samples (0, timeout − 1, timeout, timeout + 1) and a start just below
    `UINT32_MAX` are the cases worth keeping as a regression test.

??? note "Exercise 4: an expression with three increments"
    <!-- c-check: expect-warning -->

    ```c
    #include <stddef.h>
    #include <stdint.h>

    size_t pack(uint16_t *out, const uint8_t *data, size_t count)
    {
        size_t i = 0, n = 0;
        while (i + 1 < count) {
            out[n++] = (uint16_t)(data[i++] << 8 | data[i++]);
        }
        return n;
    }
    ```

    GCC 13: `warning: operation on 'i' may be undefined [-Wsequence-point]`. It is not only
    "may be": `i` is modified twice in one expression with no sequence point between the
    modifications, which is undefined behavior (C11 6.5p2). There is also no rule saying
    which `data[i++]` is evaluated first, so even a compiler that does "something sensible"
    can swap the bytes. Refactored:

    ```c
    #include <stddef.h>
    #include <stdint.h>

    size_t pack(uint16_t *out, const uint8_t *data, size_t count)
    {
        size_t n = 0;

        for (size_t i = 0; i + 1 < count; i += 2) {
            uint16_t high = data[i];
            uint16_t low = data[i + 1];

            out[n] = (uint16_t)(high << 8 | low);
            n++;
        }
        return n;
    }
    ```

    Each statement now has one side effect, the byte order is explicit, and the loop header
    says how far `i` advances.

??? note "Exercise 5: documenting a cast"
    A typical register-access cast in a driver, with the facts that make it valid written
    next to it:

    ```c
    #include <stdint.h>

    #define UART0_BASE     0x40011000u   /* reference manual, memory map table */
    #define UART_DR_OFFSET 0x04u         /* data register, 32-bit, word-aligned */

    static inline volatile uint32_t *uart0_dr(void)
    {
        /*
         * Range:     UART0_BASE + UART_DR_OFFSET lies inside the UART0 peripheral window
         *            (0x40011000-0x400113FF), so the address is a device register.
         * Alignment: the manual requires 32-bit, word-aligned access; 0x40011004 is
         *            4-byte aligned, and the pointer type makes the compiler use
         *            32-bit loads and stores.
         * Ownership: the register exists for the program's lifetime and is accessed only
         *            by this driver; interrupt handlers go through uart0_dr() as well.
         * Conversion: integer-to-pointer conversion is implementation-defined (C11
         *            6.3.2.3p5); GCC on this target maps the integer to the same address.
         */
        return (volatile uint32_t *)(uintptr_t)(UART0_BASE + UART_DR_OFFSET);
    }
    ```

    The addresses here are illustrative; the point is the four facts. A cast that cannot be
    documented this way is usually a bug. For example,
    `(struct packet *)rx_buffer` usually fails the alignment question and the effective-type
    rules (see [case study 08](../failure-case-studies/08-strict-aliasing-type-pun.md)).

??? note "Exercise 6: parenthesize and test every branch"
    <!-- c-check: expect-warning -->

    ```c
    #include <stdbool.h>
    #include <stdio.h>

    #define READY_MASK 0x6u

    static bool should_send_dense(unsigned int flags, int pending, bool force)
    {
        return flags & READY_MASK == READY_MASK && pending > 0 || force;
    }

    static bool should_send(unsigned int flags, int pending, bool force)
    {
        return (((flags & READY_MASK) == READY_MASK) && (pending > 0)) || force;
    }

    int main(void)
    {
        const unsigned int flag_values[] = { 0x0u, 0x6u, 0x7u };
        for (size_t f = 0; f < 3; f++) {
            for (int pending = 0; pending <= 1; pending++) {
                for (int force = 0; force <= 1; force++) {
                    bool a = should_send_dense(flag_values[f], pending, force);
                    bool b = should_send(flag_values[f], pending, force);
                    printf("flags=0x%x pending=%d force=%d  dense=%d intended=%d%s\n",
                           flag_values[f], pending, force, a, b, a != b ? "  <- differs" : "");
                }
            }
        }
        return 0;
    }
    ```

    GCC warns twice (`suggest parentheses around comparison in operand of '&'` and
    `suggest parentheses around '&&' within '||'`). The table of all twelve combinations:

    ```text
    flags=0x0 pending=0 force=0  dense=0 intended=0
    flags=0x0 pending=0 force=1  dense=1 intended=1
    flags=0x0 pending=1 force=0  dense=0 intended=0
    flags=0x0 pending=1 force=1  dense=1 intended=1
    flags=0x6 pending=0 force=0  dense=0 intended=0
    flags=0x6 pending=0 force=1  dense=1 intended=1
    flags=0x6 pending=1 force=0  dense=0 intended=1  <- differs
    flags=0x6 pending=1 force=1  dense=1 intended=1
    flags=0x7 pending=0 force=0  dense=0 intended=0
    flags=0x7 pending=0 force=1  dense=1 intended=1
    flags=0x7 pending=1 force=0  dense=1 intended=1
    flags=0x7 pending=1 force=1  dense=1 intended=1
    ```

    The dense version parses as `((flags & (READY_MASK == READY_MASK)) && (pending > 0)) || force`,
    so it tests bit 0 instead of the ready mask. Only one of the twelve rows exposes that, and
    a test that used `0x7` as its "ready" value would pass. Testing every branch means
    choosing inputs where each condition alone decides the result, including values where
    the mask matches but unrelated bits are clear.

## Common Mistakes

- Confusing precedence with evaluation order.
- Using logical and where a bit mask requires bitwise and, or vice versa.
- Shifting signed values or using an invalid shift count.
- Comparing signed and unsigned values without checking conversion.
- Relying on signed overflow to wrap.
- Modifying the same object multiple times in one expression.
- Adding casts merely to silence diagnostics.
- Forgetting that a macro argument can be evaluated more than once.
- Assuming both branches of a conditional expression execute.
- Assuming sizeof always preserves array type.

## Debugging Checklist

1. Add parentheses around operator boundaries whose grouping matters.
2. Inspect warnings for signedness, conversion, and sequencing issues.
3. Print operands in their actual types with correct format macros.
4. Reduce a dense expression to named intermediate variables.
5. Check shift operand type and count range.
6. Test zero, maximum, negative, wraparound, and mask-boundary cases.
7. For registers, verify atomic, write-one-to-clear, and read-modify-write requirements.
8. Inspect optimized behavior only after establishing defined source behavior.

## Related Topics

- [Language Fundamentals overview](./index.md)
- [Types, Values, And Objects](./types-values-and-objects.md)
- [Control Flow](./control-flow.md)
- [Semantics And Memory Model](../semantics-and-memory/index.md)
- [C Memory Model And Concurrency](../advanced-c/c-memory-model-and-concurrency.md)
- [C Programming](../index.md)

## References

- [ISO/IEC 9899 standards and drafts](https://open-std.org/jtc1/sc22/wg14/www/standards.html)
- [C11 public draft N1570](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n1570.pdf)
- [GCC warning options](https://gcc.gnu.org/onlinedocs/gcc/Warning-Options.html)
- [CERT C integer rules](https://wiki.sei.cmu.edu/confluence/display/c/INT30-C.+Ensure+that+unsigned+integer+operations+do+not+wrap)

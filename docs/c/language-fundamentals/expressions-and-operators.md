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

~~~c
if (temperature_c > limit_c && !sensor_fault) {
    start_heater();
}
~~~

Logical and and logical or evaluate left to right and short-circuit:

- a && b does not evaluate b when a is false;
- a || b does not evaluate b when a is true.

This can guard an access:

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

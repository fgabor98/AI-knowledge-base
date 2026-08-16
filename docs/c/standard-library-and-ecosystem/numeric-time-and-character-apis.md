---
status: draft
reviewed: false
domain: c
difficulty: intermediate
last_reviewed: null
---

# Numeric, Time, And Character APIs

Numeric, time, and character APIs look portable because their names are familiar, but their cost and behavior depend on representation, locale, floating-point hardware, clock source, and libc configuration. Embedded code must choose the right domain before selecting a convenience function.

## Learning Objectives

- Use math, time, character, and conversion functions with correct contracts.
- Distinguish wall-clock time, calendar time, processor time, and monotonic ticks.
- Handle floating-point range, rounding, exceptions, and code size.
- Use ctype functions with valid argument values.
- Understand locale and multibyte behavior.
- Choose fixed-point or integer arithmetic when the target requires it.

## Integer Conversion APIs

strtol-family functions provide range and end-pointer reporting:

~~~c
#include <errno.h>
#include <limits.h>
#include <stdlib.h>

int parse_port(const char *text, unsigned short *port)
{
    char *end;
    long value;

    if (text == NULL || port == NULL) {
        return -1;
    }

    errno = 0;
    value = strtol(text, &end, 10);
    if (errno == ERANGE || end == text || *end != '\0'
        || value < 0 || value > USHRT_MAX) {
        return -2;
    }

    *port = (unsigned short)value;
    return 0;
}
~~~

The end pointer identifies unconsumed input. Check errno and the destination range. atoi-style functions do not provide the same error detail and are poor choices for untrusted input.

For fixed-format protocols, a dedicated bounded parser can be smaller and more deterministic than a general locale-aware conversion.

## math.h

math functions operate in floating-point domains:

~~~c
#include <math.h>

float safe_norm(float x, float y)
{
    return sqrtf(x * x + y * y);
}
~~~

Before using libm on a microcontroller, check:

- hardware floating-point support and ABI;
- software-runtime size and timing;
- precision and rounding requirements;
- behavior for NaN, infinity, signed zero, and subnormal values;
- whether errno or floating exceptions are used;
- worst-case latency and interrupt restrictions.

Use float-specific functions when the data is float to avoid unnecessary double promotion, but verify the compiler and library behavior.

## fenv And Floating Exceptions

fenv.h exposes floating environment controls on implementations that support them:

~~~c
#include <fenv.h>

#pragma STDC FENV_ACCESS ON

int check_rounding_mode(void)
{
    return fegetround();
}
~~~

The pragma and functions are implementation-sensitive, may inhibit optimization, and may not be supported or useful on small targets. Do not assume floating exceptions behave like integer traps or hardware faults.

For safety-critical numerical code, define range, saturation, rounding, and exceptional-value policy explicitly. A floating-point result of infinity or NaN should not silently enter a hardware control path.

## Fixed-Point Arithmetic

Fixed point can make range and cost explicit:

~~~c
#include <stdbool.h>
#include <stdint.h>

int32_t q15_multiply(int16_t left, int16_t right)
{
    int32_t product = (int32_t)left * right;
    return product >> 15;
}
~~~

Document scaling, signed range, rounding, saturation, and overflow behavior. The right shift policy for negative values and the chosen representation are part of the implementation contract; add tests around negative and maximum values.

Use wider intermediates and checked saturation when a signal can exceed its nominal range.

## time.h And Clock Domains

time.h contains calendar and processor-time interfaces, not a universal real-time monotonic clock. An embedded project may instead use a hardware counter or RTOS tick:

~~~c
#include <stdbool.h>
#include <stdint.h>

bool timeout_expired(uint32_t now, uint32_t start, uint32_t timeout)
{
    return (uint32_t)(now - start) >= timeout;
}
~~~

Define the clock’s frequency, width, wrap period, reset behavior, and whether it is monotonic. Do not compare wall-clock timestamps for deadlines when the clock can be adjusted.

Calendar time can require timezone and locale data. Processor time measures implementation-defined CPU usage and is not necessarily elapsed wall time.

## Clock Resolution And Conversion

Converting ticks to milliseconds can overflow or lose precision:

~~~c
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

bool ticks_to_ms(uint32_t ticks, uint32_t frequency, uint32_t *milliseconds)
{
    if (milliseconds == NULL || frequency == 0u) {
        return false;
    }

    uint64_t scaled = (uint64_t)ticks * 1000u;
    if (scaled / frequency > UINT32_MAX) {
        return false;
    }

    *milliseconds = (uint32_t)(scaled / frequency);
    return true;
}
~~~

Check whether the target provides uint64_t efficiently. For long-running systems, define rounding and accumulated error rather than repeatedly truncating a conversion.

## Character Classification

ctype functions require either EOF or a value representable as unsigned char:

~~~c
#include <ctype.h>

int is_decimal_byte(unsigned char value)
{
    return isdigit(value) != 0;
}
~~~

Passing a negative signed char is undefined behavior on implementations where it is not EOF. Cast input bytes to unsigned char before classification:

~~~c
#include <ctype.h>

int is_space_byte(char value)
{
    return isspace((unsigned char)value) != 0;
}
~~~

Classification can depend on the active locale. Protocol parsers should often use explicit ASCII comparisons instead of locale-sensitive ctype behavior.

## Wide And Multibyte Characters

wchar.h and uchar.h support wide and multibyte character operations where implemented. The width and encoding of wchar_t are implementation-defined. UTF-8 byte sequences are not the same thing as a C string length in characters.

Choose an encoding at the interface boundary. For embedded protocols, an explicit byte encoding and bounded decoder are usually easier to audit than locale-dependent conversion.

## Locale

Locale affects character classification, case conversion, numeric parsing, and formatting:

~~~c
#include <locale.h>

void use_c_locale(void)
{
    setlocale(LC_ALL, "C");
}
~~~

setlocale changes process-wide state in many implementations and may not be appropriate in a concurrent or embedded environment. Do not let hidden locale state affect protocol parsing or persistent formats.

## Complex Arithmetic

complex.h can provide complex numeric types and operations, but it may add library footprint and compiler/runtime requirements:

~~~c
#include <complex.h>

double magnitude(double complex value)
{
    return cabs(value);
}
~~~

Use it when the mathematical model requires complex values and the target can support the cost. Otherwise represent real and imaginary components explicitly with documented scaling and overflow policy.

## Exercises

1. Parse bounded decimal input with strtol and test empty, trailing, negative, overflow, and maximum values.
2. Compare a monotonic tick deadline with a wall-clock deadline and explain which is appropriate for a timeout.
3. Implement saturating fixed-point multiplication and test both signs and extrema.
4. Measure math library size and timing for sqrtf, sinf, and a fixed-point alternative.
5. Test ctype calls with bytes above ASCII and negative signed-char values.
6. Define an encoding and locale policy for a command parser.
7. Test tick-to-time conversion for overflow, rounding, and counter wrap.

## Common Mistakes

- Using atoi when range and syntax errors matter.
- Treating time_t as a fixed-width wire format.
- Using wall-clock time for monotonic deadlines.
- Ignoring floating-point ABI and software-runtime cost.
- Letting NaN or infinity reach control logic.
- Shifting fixed-point negative values without a documented policy.
- Passing signed char directly to ctype functions.
- Relying on locale for protocol behavior.
- Assuming wchar_t or multibyte encodings have a portable width.
- Converting time units without checking overflow and accumulated error.

## Debugging Checklist

1. Identify numeric range, scaling, rounding, and exceptional-value policy.
2. Check the clock source, monotonicity, frequency, width, and wrap.
3. Inspect compiler options and linked libm or conversion routines.
4. Test ctype inputs through unsigned-char conversion.
5. Record locale and encoding state.
6. Compare target timing and code size with the mathematical requirement.
7. Add boundary tests for parser, tick, and fixed-point conversions.
8. Inspect floating-point status and exception behavior where relevant.

## Related Topics

- [Standard Library And Ecosystem overview](./index.md)
- [Types, Values, And Objects](../language-fundamentals/types-values-and-objects.md)
- [Expressions And Operators](../language-fundamentals/expressions-and-operators.md)
- [Embedded libc Implementations](./embedded-libc.md)
- [Embedded C And Hardware](../embedded-c-and-hardware/index.md)
- [C Programming](../index.md)

## References

- [ISO/IEC 9899 standards and drafts](https://open-std.org/jtc1/sc22/wg14/www/standards.html)
- [C11 public draft N1570](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n1570.pdf)
- [cppreference C math library](https://en.cppreference.com/w/c/numeric/math)
- [POSIX time interfaces](https://pubs.opengroup.org/onlinepubs/9799919799/basedefs/time.h.html)
- [CERT C character handling rules](https://wiki.sei.cmu.edu/confluence/display/c)

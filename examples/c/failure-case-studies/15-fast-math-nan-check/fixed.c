/* Case 15, fixed: classify by representation, which no math flag can remove. */
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

_Static_assert(sizeof(double) == sizeof(uint64_t), "double is not 64 bits");

/* IEEE 754 binary64: NaN has all exponent bits set and a non-zero mantissa. */
static bool is_nan_bits(double value)
{
    uint64_t bits;

    memcpy(&bits, &value, sizeof bits);
    return (bits & UINT64_C(0x7ff0000000000000)) == UINT64_C(0x7ff0000000000000)
        && (bits & UINT64_C(0x000fffffffffffff)) != 0;
}

__attribute__((noinline))
static double smooth(double sample, double previous)
{
    if (is_nan_bits(sample)) {
        return previous;
    }
    return 0.9 * previous + 0.1 * sample;
}

int main(int argc, char **argv)
{
    double sample = strtod(argc > 1 ? argv[1] : "nan", NULL);

    printf("smooth(%s, 20.0) = %f\n", argc > 1 ? argv[1] : "nan", smooth(sample, 20.0));
    return 0;
}

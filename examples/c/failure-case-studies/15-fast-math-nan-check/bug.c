/* Case 15: a NaN guard in a filter, built with -ffast-math. */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

__attribute__((noinline))
static double smooth(double sample, double previous)
{
    if (isnan(sample)) {            /* reject a corrupted reading */
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

/* Case 02: an overflow check that itself overflows. */
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>

/* Intended: true when a + b would overflow (callers pass b >= 0). */
__attribute__((noinline))
bool add_overflows(int a, int b)
{
    return a + b < a;
}

int main(void)
{
    volatile int a = INT_MAX;
    volatile int b = 1;

    printf("add_overflows(INT_MAX, 1) = %d\n", add_overflows(a, b));
    return 0;
}

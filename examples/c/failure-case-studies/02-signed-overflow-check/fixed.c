/* Case 02, fixed: test the operands, never the overflowed result. */
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>

__attribute__((noinline))
bool add_overflows(int a, int b)
{
    return (b > 0 && a > INT_MAX - b) || (b < 0 && a < INT_MIN - b);
}

int main(void)
{
    volatile int a = INT_MAX;
    volatile int b = 1;
    volatile int c = INT_MIN;
    volatile int d = -1;

    printf("add_overflows(INT_MAX, 1) = %d\n", add_overflows(a, b));
    printf("add_overflows(INT_MIN, -1) = %d\n", add_overflows(c, d));
    printf("add_overflows(1, 1) = %d\n", add_overflows(b, b));
    return 0;
}

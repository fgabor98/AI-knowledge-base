/* Two independent UB checks to show how UBSan reports and when it stops. */
#include <limits.h>
#include <stdio.h>

int main(int argc, char **argv)
{
    (void)argv;
    int big = INT_MAX - 1 + argc;       /* INT_MAX when run without arguments */
    int shift = 31 + argc;              /* 32 */

    int sum = big + 1;
    unsigned int mask = 1u << shift;
    printf("sum = %d, mask = %u\n", sum, mask);
    return 0;
}

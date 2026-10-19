#include <stdio.h>

int main(void)
{
    const int limit = 10;
    int *p = (int *)&limit;

    *p = 20;
    printf("%d %d\n", limit, *p);
    return 0;
}

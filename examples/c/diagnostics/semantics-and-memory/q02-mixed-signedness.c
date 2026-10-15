#include <stdio.h>

int main(void)
{
    int i = -1;
    unsigned int u = 1;

    printf("%d %d\n", i < u, i < (long)u);
    return 0;
}

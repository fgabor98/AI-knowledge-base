#include <stdio.h>

int x = 1;

int main(void)
{
    int x = 2;
    {
        extern int x;
        printf("%d\n", x);
    }
    return 0;
}

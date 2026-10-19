#include <stdio.h>

static int counter;

static int next(void)
{
    return ++counter;
}

int main(void)
{
    printf("%d %d\n", next(), next());
    return 0;
}

#include <stdio.h>

int main(void)
{
    printf("%d %zu\n", 010 + 0x10 + 10, sizeof("\x41" "1"));
    return 0;
}

#include <stdint.h>
#include <stdio.h>

int main(void)
{
    uint16_t a = UINT16_MAX;
    uint32_t b = UINT32_MAX;

    printf("%d %d\n", a + 1 > a, b + 1 > b);
    return 0;
}

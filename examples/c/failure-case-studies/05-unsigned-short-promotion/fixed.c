/* Case 05, fixed: choose the arithmetic type before multiplying. */
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

__attribute__((noinline))
bool product_is_large(uint16_t a, uint16_t b)
{
    uint32_t product = (uint32_t)a * b;

    return product >= 0x80000000u;
}

int main(void)
{
    volatile uint16_t a = 0xffff;
    volatile uint16_t b = 0xffff;

    printf("product_is_large(0xffff, 0xffff) = %d\n", product_is_large(a, b));
    return 0;
}

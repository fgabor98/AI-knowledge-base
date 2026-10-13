/* Case 05: two unsigned operands, one signed overflow. */
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

/* Intended: true when the 32-bit product has its top bit set. */
__attribute__((noinline))
bool product_is_large(uint16_t a, uint16_t b)
{
    uint32_t product = a * b;           /* a and b are promoted to int */

    return product >= 0x80000000u;
}

int main(void)
{
    volatile uint16_t a = 0xffff;
    volatile uint16_t b = 0xffff;

    printf("product_is_large(0xffff, 0xffff) = %d\n", product_is_large(a, b));
    return 0;
}

/* Case 08, fixed: copy the representation instead of aliasing it. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

_Static_assert(sizeof(float) == sizeof(uint32_t), "float is not 32 bits");

__attribute__((noinline))
float negate_bits(float value)
{
    uint32_t bits;

    memcpy(&bits, &value, sizeof bits);
    bits ^= UINT32_C(0x80000000);
    memcpy(&value, &bits, sizeof value);
    return value;
}

int main(void)
{
    float object = negate_bits(1.0f);

    printf("object holds %.1f\n", object);
    return 0;
}

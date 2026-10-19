/* Case 08: flipping a float's sign bit through a uint32_t pointer. */
#include <stdint.h>
#include <stdio.h>

/* Callers pass the same object twice: once as float, once as its bits. */
__attribute__((noinline))
float store_and_negate(float *value, uint32_t *bits)
{
    *value = 1.0f;
    *bits ^= UINT32_C(0x80000000);      /* flip the IEEE 754 sign bit */
    return *value;
}

int main(void)
{
    float object = 0.0f;
    float returned = store_and_negate(&object, (uint32_t *)&object);

    printf("returned %.1f, object holds %.1f\n", returned, object);
    return 0;
}

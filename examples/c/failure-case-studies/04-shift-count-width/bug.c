/* Case 04: a mask helper that is undefined for the widest mask. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/* Returns a mask with the low n bits set, for n in 0..32. */
uint32_t low_bits_mask(unsigned int n)
{
    return (UINT32_C(1) << n) - 1u;
}

int main(int argc, char **argv)
{
    unsigned int n = 32;

    if (argc > 1) {
        n = (unsigned int)strtoul(argv[1], NULL, 0);
    }
    printf("low_bits_mask(%u) at run time   = 0x%08x\n",
           n, (unsigned int)low_bits_mask(n));
    printf("low_bits_mask(32) constant-folded = 0x%08x\n",
           (unsigned int)low_bits_mask(32));
    return 0;
}

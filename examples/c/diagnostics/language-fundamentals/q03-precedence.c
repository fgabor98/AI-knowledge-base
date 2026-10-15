#include <stdio.h>

#define READY_MASK 0x6u

int main(void)
{
    unsigned int flags = 0x6u;

    printf("%u\n", flags & READY_MASK == READY_MASK);
    return 0;
}

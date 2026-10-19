#include <stdint.h>
#include <stdio.h>

int main(void)
{
    uint32_t value = 0x11223344u;
    const unsigned char *bytes = (const unsigned char *)&value;

    printf("%02x %02x\n", bytes[0], bytes[3]);
    return 0;
}

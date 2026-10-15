#include <stdint.h>

#ifndef SAMPLE_BUFFER_BYTES
#define SAMPLE_BUFFER_BYTES 4096
#endif

uint32_t boot_count = 1u;                       /* .data: LMA in FLASH, VMA in RAM */
const char firmware_name[] = "sensor-node";     /* .rodata: FLASH */
uint8_t sample_buffer[SAMPLE_BUFFER_BYTES];     /* .bss: RAM, no file payload */

int main(void)
{
    boot_count++;
    sample_buffer[0] = (uint8_t)firmware_name[0];
    return 0;
}

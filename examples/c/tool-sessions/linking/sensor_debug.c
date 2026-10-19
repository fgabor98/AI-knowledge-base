/* Diagnostics nobody calls in production builds: a large table and a dumper. */
#include <stdio.h>

const unsigned char sensor_calibration_dump[4096] = { 1 };

void sensor_debug_dump(void)
{
    for (unsigned int i = 0; i < sizeof sensor_calibration_dump; i++) {
        printf("%02x", sensor_calibration_dump[i]);
    }
    putchar('\n');
}

#include <stdio.h>
#include "sensor.h"

int main(void)
{
    printf("%s: %d\n", sensor_version(), sensor_read());
    return 0;
}

#include <stdio.h>

#include "sensor.h"

int main(void)
{
    printf("read %d\n", sensor_read());
    return 0;
}

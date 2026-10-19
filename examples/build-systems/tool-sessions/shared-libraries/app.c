#include <stdio.h>

#include "sensor.h"

int main(void)
{
    printf("read %d, average %d\n", sensor_read(), sensor_read_average(4));
    return 0;
}

#include "sensor.h"

/* Internal helper: should not be part of the library's exported surface. */
int sensor_scale(int raw)
{
    return raw * 10;
}

int sensor_read(void)
{
    return sensor_scale(42);
}

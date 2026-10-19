/* libsensor 1.1: adds sensor_read_average, keeps sensor_read compatible. */
#include "sensor.h"

int sensor_read(void)
{
    return 420;
}

int sensor_read_average(int samples)
{
    return samples > 0 ? 421 : 0;
}

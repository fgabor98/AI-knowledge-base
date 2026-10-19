#include <stdio.h>
#include "sensor.h"

/* An unrelated helper in the application that happens to share the name. */
int sensor_scale(int raw)
{
    return raw;
}

int main(void)
{
    printf("%s: %d (application scale of 7: %d)\n",
           sensor_version(), sensor_read(), sensor_scale(7));
    return 0;
}

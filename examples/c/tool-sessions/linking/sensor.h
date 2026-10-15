#ifndef SENSOR_H
#define SENSOR_H

/* The library's exported surface. Built with -fvisibility=hidden, nothing
 * else in the shared library is visible to other modules. */
#define SENSOR_API __attribute__((visibility("default")))

SENSOR_API int sensor_read(void);
SENSOR_API const char *sensor_version(void);

#endif

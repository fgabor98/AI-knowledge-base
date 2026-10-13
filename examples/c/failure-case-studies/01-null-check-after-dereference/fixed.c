/* Case 01, fixed: validate the pointer before the first use. */
#include <stddef.h>
#include <stdio.h>
#include <string.h>

struct device {
    int flags;
};

__attribute__((noinline))
int device_flags(const struct device *dev)
{
    if (dev == NULL) {
        return -1;
    }
    return dev->flags;
}

#ifndef NO_MAIN
int main(int argc, char **argv)
{
    struct device dev = { .flags = 0x5 };
    const struct device *target = &dev;

    if (argc > 1 && strcmp(argv[1], "null") == 0) {
        target = NULL;
    }
    printf("device_flags = %d\n", device_flags(target));
    return 0;
}
#endif

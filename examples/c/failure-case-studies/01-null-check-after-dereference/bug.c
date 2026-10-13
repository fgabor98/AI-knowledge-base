/* Case 01: a NULL check placed after the dereference it was meant to guard. */
#include <stddef.h>
#include <stdio.h>
#include <string.h>

struct device {
    int flags;
};

__attribute__((noinline))
int device_flags(const struct device *dev)
{
    int flags = dev->flags;     /* dereference first ...                  */

    if (dev == NULL) {          /* ... check second: GCC deletes this test */
        return -1;
    }
    return flags;
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

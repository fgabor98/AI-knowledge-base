#include <stdio.h>

#include "gpio.h"
#include "legacy.h"

const char *tool_version(void);

int main(void)
{
    printf("%s %s: line 3 = %d\n", LEGACY_BANNER, tool_version(), gpio_read(3));
    return 0;
}

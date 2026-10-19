#include <stdio.h>
#include "config.h"

int main(void)
{
    struct log_config config;

    config_load(&config);
    printf("sizeof = %zu level = %d flags = 0x%02x rate_limit = %u\n",
           sizeof config, (int)config.level, config.flags, config.rate_limit);
    return 0;
}

#include "stats.h"

static unsigned int rx_count;

void stats_count(void)
{
    rx_count++;
}

unsigned int stats_get(void)
{
    return rx_count;
}

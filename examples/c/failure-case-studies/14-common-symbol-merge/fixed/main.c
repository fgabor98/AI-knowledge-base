#include <stdio.h>
#include "stats.h"
#include "uart.h"

int main(void)
{
    stats_count();
    stats_count();
    uart_receive("HELLO", 5);
    printf("rx count = %u (0x%08x)\n", stats_get(), stats_get());
    return 0;
}

#include <stddef.h>
#include <stdio.h>

void uart_receive(const char *bytes, size_t length);
void stats_count(void);
unsigned int stats_get(void);

int main(void)
{
    stats_count();
    stats_count();
    uart_receive("HELLO", 5);
    printf("rx count = %u (0x%08x)\n", stats_get(), stats_get());
    return 0;
}

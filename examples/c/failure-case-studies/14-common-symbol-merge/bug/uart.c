/* Case 14: a receive buffer, defined without static or extern. */
#include <stddef.h>
#include <string.h>

unsigned char rx_buffer[16];

void uart_receive(const char *bytes, size_t length)
{
    if (length > sizeof rx_buffer) {
        length = sizeof rx_buffer;
    }
    memcpy(rx_buffer, bytes, length);
}

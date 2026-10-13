/* Case 14, fixed: module state has internal linkage. */
#include <string.h>
#include "uart.h"

static unsigned char rx_buffer[16];

void uart_receive(const char *bytes, size_t length)
{
    if (length > sizeof rx_buffer) {
        length = sizeof rx_buffer;
    }
    memcpy(rx_buffer, bytes, length);
}

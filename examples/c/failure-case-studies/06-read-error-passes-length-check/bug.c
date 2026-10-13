/* Case 06: read() returns -1 and the length check waves it through. */
#define _GNU_SOURCE
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

struct header {
    uint16_t type;
    uint16_t length;
};

int main(void)
{
    int fds[2];
    unsigned char buffer[256];

    if (pipe2(fds, O_NONBLOCK) != 0) {
        return 1;
    }

    /* Nothing has been written: read() fails with EAGAIN and returns -1. */
    ssize_t received = read(fds[0], buffer, sizeof buffer);

    if (received < sizeof(struct header)) {
        puts("short read rejected");
        return 0;
    }

    size_t payload_length = received - sizeof(struct header);
    printf("received = %zd accepted; payload_length = %zu\n",
           received, payload_length);
    return 2;
}

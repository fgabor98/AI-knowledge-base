/* Case 06, fixed: handle the error domain before the size domain. */
#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
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

    ssize_t received = read(fds[0], buffer, sizeof buffer);

    if (received < 0) {
        printf("read failed: %s\n", strerror(errno));
        return 0;
    }

    size_t length = (size_t)received;     /* non-negative: conversion is exact */

    if (length < sizeof(struct header)) {
        puts("short read rejected");
        return 0;
    }

    size_t payload_length = length - sizeof(struct header);
    printf("payload_length = %zu\n", payload_length);
    return 0;
}

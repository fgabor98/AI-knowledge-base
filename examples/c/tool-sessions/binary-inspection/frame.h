#ifndef FRAME_H
#define FRAME_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct frame_header {
    uint8_t  version;
    uint32_t sequence;
    uint8_t  type;
    uint16_t length;
};

bool frame_accept(const unsigned char *bytes, size_t size);

#endif

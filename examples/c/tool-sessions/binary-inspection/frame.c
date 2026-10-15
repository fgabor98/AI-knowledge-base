/* Frame validation. A corrupt frame triggers a trap deep in inlined code. */
#include <string.h>
#include "frame.h"

static uint16_t frame_checksum(const unsigned char *bytes, size_t size)
{
    uint16_t sum = 0;

    for (size_t i = 0; i < size; i++) {
        sum = (uint16_t)(sum + bytes[i]);
    }
    if (sum == 0) {
        __builtin_trap();               /* "impossible" state: stop here */
    }
    return sum;
}

static bool frame_valid(const struct frame_header *header,
                        const unsigned char *payload, size_t size)
{
    return header->version == 1 && frame_checksum(payload, size) != 0xffff;
}

bool frame_accept(const unsigned char *bytes, size_t size)
{
    struct frame_header header;

    if (size < sizeof header) {
        return false;
    }
    memcpy(&header, bytes, sizeof header);
    return frame_valid(&header, bytes + sizeof header, size - sizeof header);
}

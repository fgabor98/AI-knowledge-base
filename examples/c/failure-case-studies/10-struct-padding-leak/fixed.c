/* Case 10, fixed: serialize fields explicitly; the wire format owns its layout. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

struct telemetry {
    uint8_t  kind;
    uint32_t value;
    uint16_t channel;
};

enum { TELEMETRY_WIRE_SIZE = 7 };   /* kind:1, value:4 (LE), channel:2 (LE) */

__attribute__((noinline))
static void handle_session_key(void)
{
    volatile char key[64];

    for (size_t i = 0; i < sizeof key; i++) {
        key[i] = (char)('A' + i % 26);
    }
}

static void encode_telemetry(const struct telemetry *record,
                             unsigned char wire[TELEMETRY_WIRE_SIZE])
{
    wire[0] = record->kind;
    wire[1] = (unsigned char)(record->value);
    wire[2] = (unsigned char)(record->value >> 8);
    wire[3] = (unsigned char)(record->value >> 16);
    wire[4] = (unsigned char)(record->value >> 24);
    wire[5] = (unsigned char)(record->channel);
    wire[6] = (unsigned char)(record->channel >> 8);
}

__attribute__((noinline))
static void send_telemetry(void)
{
    struct telemetry record = { .kind = 1, .value = 0x11223344u, .channel = 7 };
    unsigned char wire[TELEMETRY_WIRE_SIZE];

    encode_telemetry(&record, wire);
    for (size_t i = 0; i < sizeof wire; i++) {
        printf("%02x ", wire[i]);
    }
    putchar('\n');
}

int main(void)
{
    handle_session_key();
    send_telemetry();
    return 0;
}

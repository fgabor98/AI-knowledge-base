/* Case 10: serializing a struct with memcpy() also sends its padding. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

struct telemetry {
    uint8_t  kind;          /* followed by 3 padding bytes */
    uint32_t value;
    uint16_t channel;       /* followed by 2 padding bytes */
};

/* Earlier work leaves sensitive bytes in the stack area the next call reuses. */
__attribute__((noinline))
static void handle_session_key(void)
{
    volatile char key[64];

    for (size_t i = 0; i < sizeof key; i++) {
        key[i] = (char)('A' + i % 26);
    }
}

__attribute__((noinline))
static void send_telemetry(void)
{
    struct telemetry record;
    unsigned char wire[sizeof record];

    record.kind = 1;
    record.value = 0x11223344u;
    record.channel = 7;
    memcpy(wire, &record, sizeof record);   /* "serialize" */

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

#include <stdio.h>
#include "frame.h"

void crash_record_install(void);

int main(int argc, char **argv)
{
    /* version 1, then a payload whose bytes sum to zero modulo 2^16 */
    unsigned char frame[16] = { 1 };

    (void)argv;
    crash_record_install();
    if (argc > 1) {
        frame[12] = 0x2a;               /* a "good" frame */
    }
    printf("accepted: %d\n", frame_accept(frame, sizeof frame));
    return 0;
}

/*
 * Freestanding startup for a firmware-style image. Built and linked with the
 * host compiler to exercise the linker script; the image is inspected, not run.
 */
#include <stddef.h>
#include <stdint.h>

extern unsigned char __data_load_start__[];
extern unsigned char __data_start__[];
extern unsigned char __data_end__[];
extern unsigned char __bss_start__[];
extern unsigned char __bss_end__[];

int main(void);
void Reset_Handler(void);

/* A vector-table stand-in: KEEP() in the script retains it. */
__attribute__((section(".isr_vector"), used))
static void (*const vector_table[])(void) = { Reset_Handler };

void Reset_Handler(void)
{
    size_t data_size = (size_t)(__data_end__ - __data_start__);

    for (size_t i = 0u; i < data_size; ++i) {
        __data_start__[i] = __data_load_start__[i];
    }
    for (unsigned char *p = __bss_start__; p != __bss_end__; ++p) {
        *p = 0u;
    }
    (void)main();
    for (;;) {
    }
}

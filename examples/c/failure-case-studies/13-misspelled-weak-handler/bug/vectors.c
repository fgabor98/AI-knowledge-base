/* Case 13: startup-file convention - every handler is a weak alias. */
#include <stdio.h>

void default_handler(void);

void default_handler(void)
{
    puts("default_handler: unexpected interrupt");
}

void uart1_irq_handler(void) __attribute__((weak, alias("default_handler")));
void timer0_irq_handler(void) __attribute__((weak, alias("default_handler")));

typedef void (*vector)(void);

const vector vector_table[] = {
    uart1_irq_handler,
    timer0_irq_handler,
};

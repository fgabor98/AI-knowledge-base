/* Application driver: meant to override uart1_irq_handler. Spot the typo. */
#include <stdio.h>

void uart1_irq_hander(void)
{
    puts("uart1: byte received");
}

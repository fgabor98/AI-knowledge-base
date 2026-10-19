#include <stdio.h>
#include "handlers.h"   /* a misspelled definition now has no prototype */

void uart1_irq_handler(void)
{
    puts("uart1: byte received");
}

#include "gpio.h"

int gpio_read(int line)
{
    return line % 2;
}

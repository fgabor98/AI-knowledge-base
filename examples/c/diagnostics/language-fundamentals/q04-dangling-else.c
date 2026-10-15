#include <stdio.h>

int main(void)
{
    int armed = 0;
    int fault = 1;

    if (armed)
        if (fault)
            puts("A");
    else
        puts("B");
    puts("end");
    return 0;
}

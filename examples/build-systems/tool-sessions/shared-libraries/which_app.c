#include <stdio.h>

const char *which_lib(void);

int main(void)
{
    printf("loaded libwhich from %s\n", which_lib());
    return 0;
}

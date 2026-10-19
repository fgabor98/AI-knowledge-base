#include <stdio.h>
#include <string.h>

static size_t parameter_size(char buffer[16])
{
    return sizeof buffer;
}

int main(void)
{
    char name[] = "sensor";

    printf("%zu %zu %zu\n", sizeof name, strlen(name), parameter_size(name));
    return 0;
}

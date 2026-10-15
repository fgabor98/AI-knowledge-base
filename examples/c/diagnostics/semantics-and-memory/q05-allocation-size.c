#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    size_t count = SIZE_MAX / 2 + 2;     /* e.g. a length field from the wire */
    void *a = malloc(count * 4);
    void *b = calloc(count, 4);

    printf("%zu %s %s\n", count * 4, a ? "non-null" : "null", b ? "non-null" : "null");
    free(a);
    free(b);
    return 0;
}

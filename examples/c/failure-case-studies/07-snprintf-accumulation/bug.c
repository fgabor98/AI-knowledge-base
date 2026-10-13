/* Case 07: summing snprintf() return values walks off the buffer. */
#include <stdio.h>
#include <string.h>

static size_t format_readings(char *out, size_t size,
                              const int *values, size_t count)
{
    size_t used = 0;

    for (size_t i = 0; i < count; i++) {
        used += snprintf(out + used, size - used, "sensor%zu=%d;", i, values[i]);
    }
    return used;
}

int main(void)
{
    struct {
        char text[32];
        char neighbour[16];
    } frame;
    const int values[] = { 1000, -2000, 3000, -4000, 5000 };

    strcpy(frame.neighbour, "NEIGHBOUR-OK");
    size_t used = format_readings(frame.text, sizeof frame.text, values, 5);
    printf("used = %zu (buffer is %zu)\n", used, sizeof frame.text);
    printf("neighbour = \"%.15s\"\n", frame.neighbour);
    return 0;
}

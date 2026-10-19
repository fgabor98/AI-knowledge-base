/* Case 07, fixed: treat a return value >= the remaining space as truncation. */
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

/* Returns false if the output had to be truncated. */
static bool format_readings(char *out, size_t size,
                            const int *values, size_t count)
{
    size_t used = 0;

    if (size == 0) {
        return false;
    }
    out[0] = '\0';
    for (size_t i = 0; i < count; i++) {
        int written = snprintf(out + used, size - used, "sensor%zu=%d;", i, values[i]);

        if (written < 0 || (size_t)written >= size - used) {
            return false;               /* out is still NUL-terminated */
        }
        used += (size_t)written;
    }
    return true;
}

int main(void)
{
    struct {
        char text[32];
        char neighbour[16];
    } frame;
    const int values[] = { 1000, -2000, 3000, -4000, 5000 };

    strcpy(frame.neighbour, "NEIGHBOUR-OK");
    bool complete = format_readings(frame.text, sizeof frame.text, values, 5);
    printf("complete = %d text = \"%s\"\n", complete, frame.text);
    printf("neighbour = \"%.15s\"\n", frame.neighbour);
    return 0;
}

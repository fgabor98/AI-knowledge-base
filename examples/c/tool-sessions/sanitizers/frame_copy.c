/* Off-by-one heap write: room for the payload, but not for the terminator. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *frame_to_string(const char *payload, size_t length)
{
    char *text = malloc(length);        /* should be length + 1 */

    if (text == NULL) {
        return NULL;
    }
    memcpy(text, payload, length);
    text[length] = '\0';
    return text;
}

int main(void)
{
    const char payload[] = { 'S', 'T', 'A', 'T', 'U', 'S', '=', 'O', 'K' };
    char *text = frame_to_string(payload, sizeof payload);

    printf("%s\n", text);
    free(text);
    return 0;
}

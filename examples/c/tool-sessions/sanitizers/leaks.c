/* Direct and indirect leaks, plus a leak inside "vendor" code to suppress. */
#include <stdlib.h>
#include <string.h>

struct session {
    char *buffer;
    size_t size;
};

static struct session *session_open(size_t size)
{
    struct session *session = malloc(sizeof *session);

    if (session == NULL) {
        return NULL;
    }
    session->buffer = malloc(size);
    session->size = size;
    return session;
}

/* Stands in for a third-party initializer that allocates once and never frees. */
void vendor_sdk_init(void)
{
    static char *registry;

    registry = malloc(64);
    strcpy(registry, "sdk");
    registry = NULL;
}

int main(void)
{
    vendor_sdk_init();
    struct session *session = session_open(256);
    session = NULL;                     /* lost: struct and its buffer leak */
    (void)session;
    return 0;
}

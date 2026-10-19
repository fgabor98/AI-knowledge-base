/* Case 11: a plain bool used as a "data ready" flag between threads. */
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>

#ifdef VOLATILE_FLAG
static volatile bool ready;     /* the folklore "fix" */
#else
static bool ready;
#endif
static int payload;

static void *producer(void *arg)
{
    (void)arg;
    usleep(100 * 1000);
    payload = 42;
    ready = true;
    return NULL;
}

int main(void)
{
    pthread_t thread;

    if (pthread_create(&thread, NULL, producer, NULL) != 0) {
        return 1;
    }
    while (!ready) {
        /* wait for the producer */
    }
    printf("payload = %d\n", payload);
    pthread_join(thread, NULL);
    return 0;
}

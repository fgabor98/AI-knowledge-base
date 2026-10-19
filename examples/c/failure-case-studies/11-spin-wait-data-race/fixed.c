/* Case 11, fixed: the flag is atomic and publishes the payload. */
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>

static atomic_bool ready;
static int payload;

static void *producer(void *arg)
{
    (void)arg;
    usleep(100 * 1000);
    payload = 42;                                   /* plain write ...   */
    atomic_store_explicit(&ready, true,
                          memory_order_release);    /* ... published here */
    return NULL;
}

int main(void)
{
    pthread_t thread;

    if (pthread_create(&thread, NULL, producer, NULL) != 0) {
        return 1;
    }
    while (!atomic_load_explicit(&ready, memory_order_acquire)) {
        /* a real program would block on a condition variable or futex */
    }
    printf("payload = %d\n", payload);              /* ordered after acquire */
    pthread_join(thread, NULL);
    return 0;
}

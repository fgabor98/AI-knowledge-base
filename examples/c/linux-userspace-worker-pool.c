#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

enum { QUEUE_CAPACITY = 8, WORKER_COUNT = 2, TASK_COUNT = 12 };

struct work_queue {
    int items[QUEUE_CAPACITY];
    size_t head;
    size_t tail;
    size_t count;
    int stopping;
    pthread_mutex_t mutex;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;
};

static void report_error(const char *operation, int error)
{
    fprintf(stderr, "%s: %s\n", operation, strerror(error));
}

static void *worker_main(void *argument)
{
    struct work_queue *queue = argument;

    for (;;) {
        int task;
        int error = pthread_mutex_lock(&queue->mutex);
        if (error != 0) {
            report_error("worker mutex lock", error);
            return NULL;
        }
        while (queue->count == 0U && !queue->stopping) {
            error = pthread_cond_wait(&queue->not_empty, &queue->mutex);
            if (error != 0) {
                report_error("worker cond wait", error);
                pthread_mutex_unlock(&queue->mutex);
                return NULL;
            }
        }
        if (queue->count == 0U && queue->stopping) {
            pthread_mutex_unlock(&queue->mutex);
            return NULL;
        }
        task = queue->items[queue->head];
        queue->head = (queue->head + 1U) % QUEUE_CAPACITY;
        --queue->count;
        pthread_cond_signal(&queue->not_full);
        pthread_mutex_unlock(&queue->mutex);

        printf("worker %lu processed task %d\n",
               (unsigned long)pthread_self(), task);
    }
}

int main(void)
{
    struct work_queue queue = {
        .mutex = PTHREAD_MUTEX_INITIALIZER,
        .not_empty = PTHREAD_COND_INITIALIZER,
        .not_full = PTHREAD_COND_INITIALIZER,
    };
    pthread_t workers[WORKER_COUNT];
    size_t created = 0U;

    for (; created < WORKER_COUNT; ++created) {
        int error = pthread_create(&workers[created], NULL, worker_main, &queue);
        if (error != 0) {
            report_error("pthread_create", error);
            break;
        }
    }

    for (int task = 1; task <= TASK_COUNT && created == WORKER_COUNT; ++task) {
        int error = pthread_mutex_lock(&queue.mutex);
        if (error != 0) {
            report_error("producer mutex lock", error);
            break;
        }
        while (queue.count == QUEUE_CAPACITY && !queue.stopping) {
            error = pthread_cond_wait(&queue.not_full, &queue.mutex);
            if (error != 0) {
                report_error("producer cond wait", error);
                queue.stopping = 1;
                break;
            }
        }
        if (!queue.stopping) {
            queue.items[queue.tail] = task;
            queue.tail = (queue.tail + 1U) % QUEUE_CAPACITY;
            ++queue.count;
            pthread_cond_signal(&queue.not_empty);
        }
        pthread_mutex_unlock(&queue.mutex);
        struct timespec delay = {.tv_sec = 0, .tv_nsec = 10000000L};
        nanosleep(&delay, NULL);
    }

    pthread_mutex_lock(&queue.mutex);
    queue.stopping = 1;
    pthread_cond_broadcast(&queue.not_empty);
    pthread_cond_broadcast(&queue.not_full);
    pthread_mutex_unlock(&queue.mutex);

    int result = EXIT_SUCCESS;
    for (size_t i = 0U; i < created; ++i) {
        int error = pthread_join(workers[i], NULL);
        if (error != 0) {
            report_error("pthread_join", error);
            result = EXIT_FAILURE;
        }
    }
    pthread_cond_destroy(&queue.not_full);
    pthread_cond_destroy(&queue.not_empty);
    pthread_mutex_destroy(&queue.mutex);
    return result;
}

/* Two threads take the same two mutexes in opposite orders. */
#include <pthread.h>
#include <stdio.h>

static pthread_mutex_t config_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t stats_lock = PTHREAD_MUTEX_INITIALIZER;
static int config_version;
static int stats_reads;

static void *reload_config(void *arg)
{
    (void)arg;
    pthread_mutex_lock(&config_lock);
    pthread_mutex_lock(&stats_lock);
    config_version++;
    pthread_mutex_unlock(&stats_lock);
    pthread_mutex_unlock(&config_lock);
    return NULL;
}

static void *report_stats(void *arg)
{
    (void)arg;
    pthread_mutex_lock(&stats_lock);
    pthread_mutex_lock(&config_lock);
    stats_reads++;
    pthread_mutex_unlock(&config_lock);
    pthread_mutex_unlock(&stats_lock);
    return NULL;
}

int main(void)
{
    pthread_t first;
    pthread_t second;

    pthread_create(&first, NULL, reload_config, NULL);
    pthread_join(first, NULL);          /* sequential: the deadlock never happens here */
    pthread_create(&second, NULL, report_stats, NULL);
    pthread_join(second, NULL);
    printf("version %d, reads %d\n", config_version, stats_reads);
    return 0;
}

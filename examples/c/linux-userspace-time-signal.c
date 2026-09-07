#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static volatile sig_atomic_t stop_requested;

static void on_interrupt(int signal_number)
{
    (void)signal_number;
    stop_requested = 1;
}

static void report_errno(const char *operation)
{
    int saved_errno = errno;

    fprintf(stderr, "%s: %s\n", operation, strerror(saved_errno));
}

int main(void)
{
    struct sigaction action = {
        .sa_handler = on_interrupt,
    };
    sigemptyset(&action.sa_mask);

    if (sigaction(SIGINT, &action, NULL) == -1) {
        report_errno("sigaction");
        return EXIT_FAILURE;
    }

    for (unsigned int tick = 1U; tick <= 10U && !stop_requested; ++tick) {
        struct timespec now;
        if (clock_gettime(CLOCK_MONOTONIC, &now) == -1) {
            report_errno("clock_gettime");
            return EXIT_FAILURE;
        }

        printf("tick=%u monotonic=%lld.%09ld\n", tick,
               (long long)now.tv_sec, now.tv_nsec);
        fflush(stdout);

        struct timespec delay = {.tv_sec = 0, .tv_nsec = 200000000L};
        while (nanosleep(&delay, &delay) == -1) {
            if (errno != EINTR) {
                report_errno("nanosleep");
                return EXIT_FAILURE;
            }
            if (stop_requested) {
                break;
            }
        }
    }

    puts(stop_requested ? "shutdown requested" : "completed normally");
    return EXIT_SUCCESS;
}

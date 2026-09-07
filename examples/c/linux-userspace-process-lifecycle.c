#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

static void report_errno(const char *operation)
{
    int saved_errno = errno;

    fprintf(stderr, "%s: %s\n", operation, strerror(saved_errno));
}

int main(void)
{
    pid_t child = fork();

    if (child == -1) {
        report_errno("fork");
        return EXIT_FAILURE;
    }

    if (child == 0) {
        printf("child pid=%ld ppid=%ld\n", (long)getpid(), (long)getppid());
        fflush(stdout);
        _exit(7);
    }

    int status;
    pid_t waited;
    do {
        waited = waitpid(child, &status, 0);
    } while (waited == -1 && errno == EINTR);

    if (waited == -1) {
        report_errno("waitpid");
        return EXIT_FAILURE;
    }

    if (WIFEXITED(status)) {
        printf("parent pid=%ld child=%ld exit=%d\n", (long)getpid(),
               (long)child, WEXITSTATUS(status));
        return EXIT_SUCCESS;
    }

    if (WIFSIGNALED(status)) {
        printf("child=%ld signal=%d\n", (long)child, WTERMSIG(status));
    }

    return EXIT_FAILURE;
}

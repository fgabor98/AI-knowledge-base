#define _GNU_SOURCE

#include <errno.h>
#include <fcntl.h>
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
    int pipe_fds[2];

    if (pipe2(pipe_fds, O_CLOEXEC) == -1) {
        report_errno("pipe2");
        return EXIT_FAILURE;
    }

    pid_t child = fork();
    if (child == -1) {
        report_errno("fork");
        close(pipe_fds[0]);
        close(pipe_fds[1]);
        return EXIT_FAILURE;
    }

    if (child == 0) {
        close(pipe_fds[1]);
        char buffer[64];
        ssize_t count = read(pipe_fds[0], buffer, sizeof buffer - 1U);
        if (count == -1) {
            report_errno("child read");
            close(pipe_fds[0]);
            _exit(EXIT_FAILURE);
        }
        buffer[count] = '\0';
        printf("child received %zd bytes: %s\n", count, buffer);
        fflush(stdout);
        close(pipe_fds[0]);
        _exit(EXIT_SUCCESS);
    }

    close(pipe_fds[0]);
    const char message[] = "descriptor ownership matters\n";
    size_t offset = 0U;
    while (offset < sizeof message - 1U) {
        ssize_t count = write(pipe_fds[1], message + offset,
                              sizeof message - 1U - offset);
        if (count > 0) {
            offset += (size_t)count;
        } else if (count == -1 && errno == EINTR) {
            continue;
        } else {
            report_errno("parent write");
            close(pipe_fds[1]);
            waitpid(child, NULL, 0);
            return EXIT_FAILURE;
        }
    }
    close(pipe_fds[1]);

    int status;
    if (waitpid(child, &status, 0) == -1) {
        report_errno("waitpid");
        return EXIT_FAILURE;
    }

    return WIFEXITED(status) && WEXITSTATUS(status) == EXIT_SUCCESS
               ? EXIT_SUCCESS
               : EXIT_FAILURE;
}

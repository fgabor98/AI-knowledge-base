#define _GNU_SOURCE

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

static void report_errno(const char *operation)
{
    int saved_errno = errno;

    fprintf(stderr, "%s: %s\n", operation, strerror(saved_errno));
}

static int close_pair(int listener, int peer, int child_status)
{
    close(listener);
    close(peer);
    return WIFEXITED(child_status) && WEXITSTATUS(child_status) == EXIT_SUCCESS
               ? EXIT_SUCCESS
               : EXIT_FAILURE;
}

int main(void)
{
    int listener = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
    if (listener == -1) {
        report_errno("socket");
        return EXIT_FAILURE;
    }

    int reuse = 1;
    if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof reuse) == -1) {
        report_errno("setsockopt");
        close(listener);
        return EXIT_FAILURE;
    }

    struct sockaddr_in address = {
        .sin_family = AF_INET,
        .sin_port = 0,
    };
    if (inet_pton(AF_INET, "127.0.0.1", &address.sin_addr) != 1 ||
        bind(listener, (struct sockaddr *)&address, sizeof address) == -1 ||
        listen(listener, 1) == -1) {
        report_errno("bind/listen");
        close(listener);
        return EXIT_FAILURE;
    }

    socklen_t address_length = sizeof address;
    if (getsockname(listener, (struct sockaddr *)&address, &address_length) == -1) {
        report_errno("getsockname");
        close(listener);
        return EXIT_FAILURE;
    }

    pid_t child = fork();
    if (child == -1) {
        report_errno("fork");
        close(listener);
        return EXIT_FAILURE;
    }
    if (child == 0) {
        int client = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
        if (client == -1 || connect(client, (struct sockaddr *)&address,
                                     sizeof address) == -1) {
            report_errno("client connect");
            if (client != -1) {
                close(client);
            }
            _exit(EXIT_FAILURE);
        }
        const char message[] = "loopback tcp frame\n";
        if (write(client, message, sizeof message) == -1) {
            report_errno("client write");
            close(client);
            _exit(EXIT_FAILURE);
        }
        close(client);
        _exit(EXIT_SUCCESS);
    }

    int peer = accept4(listener, NULL, NULL, SOCK_CLOEXEC);
    if (peer == -1) {
        report_errno("accept4");
        close(listener);
        waitpid(child, NULL, 0);
        return EXIT_FAILURE;
    }
    char buffer[64];
    ssize_t count = read(peer, buffer, sizeof buffer - 1U);
    if (count == -1) {
        report_errno("server read");
        close(peer);
        close(listener);
        waitpid(child, NULL, 0);
        return EXIT_FAILURE;
    }
    buffer[count] = '\0';
    printf("received %zd bytes: %s", count, buffer);

    int child_status;
    pid_t waited;
    do {
        waited = waitpid(child, &child_status, 0);
    } while (waited == -1 && errno == EINTR);
    if (waited == -1) {
        report_errno("waitpid");
        close(peer);
        close(listener);
        return EXIT_FAILURE;
    }
    return close_pair(listener, peer, child_status);
}

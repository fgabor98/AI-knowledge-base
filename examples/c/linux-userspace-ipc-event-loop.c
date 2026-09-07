#define _GNU_SOURCE

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/socket.h>
#include <unistd.h>

static void report_errno(const char *operation)
{
    int saved_errno = errno;

    fprintf(stderr, "%s: %s\n", operation, strerror(saved_errno));
}

int main(void)
{
    int sockets[2];
    if (socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, sockets) == -1) {
        report_errno("socketpair");
        return EXIT_FAILURE;
    }

    int wakeup = eventfd(0U, EFD_CLOEXEC | EFD_NONBLOCK);
    if (wakeup == -1) {
        report_errno("eventfd");
        close(sockets[0]);
        close(sockets[1]);
        return EXIT_FAILURE;
    }

    int epoll_fd = epoll_create1(EPOLL_CLOEXEC);
    if (epoll_fd == -1) {
        report_errno("epoll_create1");
        close(wakeup);
        close(sockets[0]);
        close(sockets[1]);
        return EXIT_FAILURE;
    }

    struct epoll_event socket_event = {.events = EPOLLIN, .data.fd = sockets[0]};
    struct epoll_event wakeup_event = {.events = EPOLLIN, .data.fd = wakeup};
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sockets[0], &socket_event) == -1 ||
        epoll_ctl(epoll_fd, EPOLL_CTL_ADD, wakeup, &wakeup_event) == -1) {
        report_errno("epoll_ctl");
        close(epoll_fd);
        close(wakeup);
        close(sockets[0]);
        close(sockets[1]);
        return EXIT_FAILURE;
    }

    const char message[] = "event-loop message";
    if (write(sockets[1], message, sizeof message) == -1) {
        report_errno("write");
        close(epoll_fd);
        close(wakeup);
        close(sockets[0]);
        close(sockets[1]);
        return EXIT_FAILURE;
    }
    uint64_t notification = 1U;
    if (write(wakeup, &notification, sizeof notification) == -1) {
        report_errno("eventfd write");
    }

    struct epoll_event events[2];
    int count = epoll_wait(epoll_fd, events, 2, 1000);
    if (count == -1) {
        report_errno("epoll_wait");
    } else {
        for (int i = 0; i < count; ++i) {
            if (events[i].data.fd == sockets[0]) {
                char buffer[64];
                ssize_t received = read(sockets[0], buffer, sizeof buffer - 1U);
                if (received > 0) {
                    buffer[received] = '\0';
                    printf("socket event: %s\n", buffer);
                }
            } else if (events[i].data.fd == wakeup) {
                uint64_t value;
                if (read(wakeup, &value, sizeof value) == sizeof value) {
                    printf("wakeup count: %llu\n",
                           (unsigned long long)value);
                }
            }
        }
    }

    close(epoll_fd);
    close(wakeup);
    close(sockets[0]);
    close(sockets[1]);
    return count == -1 ? EXIT_FAILURE : EXIT_SUCCESS;
}

// SPDX-License-Identifier: GPL-2.0
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define DEV "/dev/lab_poll"

int main(void)
{
	struct pollfd pfd;
	uint32_t v[16];
	ssize_t n;
	int fd, i, ret;

	fd = open(DEV, O_RDONLY | O_NONBLOCK);
	if (fd < 0) {
		perror(DEV);
		return 1;
	}

	/* Drain whatever is queued; the read after that must not block. */
	while ((n = read(fd, v, sizeof(v))) > 0)
		;
	if (n != -1 || errno != EAGAIN) {
		printf("FAIL nonblocking empty read: %zd %s\n", n, strerror(errno));
		return 1;
	}
	puts("ok   nonblocking empty read: EAGAIN");

	pfd.fd = fd;
	pfd.events = POLLIN;
	for (i = 0; i < 3; i++) {
		ret = poll(&pfd, 1, 5000);
		if (ret != 1 || !(pfd.revents & POLLIN)) {
			printf("FAIL poll: ret=%d revents=%#x\n", ret, pfd.revents);
			return 1;
		}
		n = read(fd, v, sizeof(v));
		if (n < (ssize_t)sizeof(v[0])) {
			printf("FAIL poll said readable, read returned %zd\n", n);
			return 1;
		}
		printf("ok   poll then read: sample %u\n", v[0]);
	}
	close(fd);

	fd = open(DEV, O_RDONLY);
	n = read(fd, v, sizeof(v[0]));
	if (n != sizeof(v[0])) {
		printf("FAIL blocking read: %zd\n", n);
		return 1;
	}
	printf("ok   blocking read: sample %u\n", v[0]);
	close(fd);
	return 0;
}

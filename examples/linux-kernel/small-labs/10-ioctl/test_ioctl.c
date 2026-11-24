// SPDX-License-Identifier: GPL-2.0
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "lab_ioctl.h"

static int failures;

static void expect(const char *what, int ret, int want_errno)
{
	int got = ret < 0 ? errno : 0;
	int ok = got == want_errno;

	printf("%-4s %s: %s\n", ok ? "ok" : "FAIL", what,
	       got ? strerror(got) : "success");
	failures += !ok;
}

int main(void)
{
	struct lab_ioctl_config cfg = {
		.version = LAB_IOCTL_ABI_VERSION,
		.flags = LAB_IOCTL_FLAG_FAST,
		.rate_hz = 250,
	};
	struct lab_ioctl_status status;
	int fd = open("/dev/lab_ioctl", O_RDWR);

	if (fd < 0) {
		perror("/dev/lab_ioctl");
		return 1;
	}

	expect("reset", ioctl(fd, LAB_IOCTL_RESET), 0);
	expect("set valid config", ioctl(fd, LAB_IOCTL_SET_CONFIG, &cfg), 0);
	expect("get status", ioctl(fd, LAB_IOCTL_GET_STATUS, &status), 0);
	if (status.rate_hz != 250 || status.set_count != 1) {
		printf("FAIL status: rate_hz=%u set_count=%llu\n",
		       status.rate_hz, (unsigned long long)status.set_count);
		failures++;
	}

	cfg.flags = 1U << 31;
	expect("unknown flag", ioctl(fd, LAB_IOCTL_SET_CONFIG, &cfg), EINVAL);
	cfg.flags = 0;
	cfg.rate_hz = 5000;
	expect("rate out of range", ioctl(fd, LAB_IOCTL_SET_CONFIG, &cfg), ERANGE);
	cfg.rate_hz = 10;
	cfg.reserved[3] = 1;
	expect("reserved not zero", ioctl(fd, LAB_IOCTL_SET_CONFIG, &cfg), EINVAL);
	expect("bad pointer", ioctl(fd, LAB_IOCTL_SET_CONFIG, (void *)8), EFAULT);
	expect("unknown command", ioctl(fd, _IO(LAB_IOCTL_MAGIC, 0xff)), ENOTTY);

	close(fd);
	return failures != 0;
}

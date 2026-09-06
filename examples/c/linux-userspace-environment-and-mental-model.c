#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <unistd.h>

static void print_errno(const char *operation)
{
    int saved_errno = errno;

    fprintf(stderr, "%s: %s\n", operation, strerror(saved_errno));
}

static void print_proc_link(const char *label, const char *path)
{
    char target[PATH_MAX];
    ssize_t length = readlink(path, target, sizeof target - 1U);

    if (length == -1) {
        fprintf(stderr, "%s (%s): ", label, path);
        print_errno("readlink");
        return;
    }

    target[length] = '\0';
    printf("%s: %s\n", label, target);
}

static void print_system_identity(void)
{
    struct utsname system_info;

    if (uname(&system_info) == -1) {
        print_errno("uname");
        return;
    }

    printf("sysname: %s\n", system_info.sysname);
    printf("release: %s\n", system_info.release);
    printf("machine: %s\n", system_info.machine);
}

static int probe_device_boundary(void)
{
    int fd = open("/dev/null", O_RDWR | O_CLOEXEC);

    if (fd == -1) {
        print_errno("open /dev/null");
        return EXIT_FAILURE;
    }

    printf("opened /dev/null as fd: %d\n", fd);

    if (close(fd) == -1) {
        print_errno("close /dev/null");
        return EXIT_FAILURE;
    }

    printf("closed fd: %d\n", fd);
    return EXIT_SUCCESS;
}

int main(void)
{
    printf("pid: %ld\n", (long)getpid());
    printf("ppid: %ld\n", (long)getppid());
    printf("uid/euid: %lu/%lu\n", (unsigned long)getuid(),
           (unsigned long)geteuid());
    printf("gid/egid: %lu/%lu\n", (unsigned long)getgid(),
           (unsigned long)getegid());
    print_system_identity();
    print_proc_link("executable", "/proc/self/exe");
    print_proc_link("mount namespace", "/proc/self/ns/mnt");

    if (probe_device_boundary() != EXIT_SUCCESS) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

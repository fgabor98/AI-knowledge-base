#define _GNU_SOURCE

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

static void report_errno(const char *operation)
{
    int saved_errno = errno;

    fprintf(stderr, "%s: %s\n", operation, strerror(saved_errno));
}

int main(void)
{
    int master = posix_openpt(O_RDWR | O_NOCTTY | O_CLOEXEC);
    if (master == -1) {
        report_errno("posix_openpt");
        return EXIT_FAILURE;
    }
    if (grantpt(master) == -1 || unlockpt(master) == -1) {
        report_errno("grantpt/unlockpt");
        close(master);
        return EXIT_FAILURE;
    }

    char *slave_name = ptsname(master);
    if (slave_name == NULL) {
        report_errno("ptsname");
        close(master);
        return EXIT_FAILURE;
    }
    int slave = open(slave_name, O_RDWR | O_NOCTTY | O_CLOEXEC);
    if (slave == -1) {
        report_errno("open slave");
        close(master);
        return EXIT_FAILURE;
    }

    struct termios settings;
    if (tcgetattr(slave, &settings) == -1) {
        report_errno("tcgetattr");
        close(slave);
        close(master);
        return EXIT_FAILURE;
    }
    cfmakeraw(&settings);
    if (tcsetattr(slave, TCSANOW, &settings) == -1) {
        report_errno("tcsetattr");
        close(slave);
        close(master);
        return EXIT_FAILURE;
    }

    const char message[] = "pty serial bytes\n";
    if (write(slave, message, sizeof message) == -1) {
        report_errno("write slave");
        close(slave);
        close(master);
        return EXIT_FAILURE;
    }

    char buffer[64];
    ssize_t count = read(master, buffer, sizeof buffer - 1U);
    if (count == -1) {
        report_errno("read master");
        close(slave);
        close(master);
        return EXIT_FAILURE;
    }
    buffer[count] = '\0';
    printf("slave=%s received=%zd bytes: %s", slave_name, count, buffer);

    close(slave);
    close(master);
    return EXIT_SUCCESS;
}

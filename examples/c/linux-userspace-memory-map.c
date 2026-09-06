#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>

static void report_errno(const char *operation)
{
    int saved_errno = errno;

    fprintf(stderr, "%s: %s\n", operation, strerror(saved_errno));
}

int main(void)
{
    const size_t length = 4096U;
    unsigned char *region = mmap(NULL, length, PROT_READ | PROT_WRITE,
                                 MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (region == MAP_FAILED) {
        report_errno("mmap");
        return EXIT_FAILURE;
    }

    region[0] = 0x5aU;
    printf("mapping=%p length=%zu first-byte=0x%02x\n", (void *)region,
           length, (unsigned int)region[0]);

    if (mprotect(region, length, PROT_READ) == -1) {
        report_errno("mprotect");
        munmap(region, length);
        return EXIT_FAILURE;
    }

    printf("protection changed to read-only\n");
    if (munmap(region, length) == -1) {
        report_errno("munmap");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

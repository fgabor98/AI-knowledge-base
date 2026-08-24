---
status: draft
reviewed: false
domain: c
difficulty: intermediate
last_reviewed: null
---

# POSIX And System Interfaces

POSIX defines interfaces for operating-system services such as files, processes, threads, signals, sockets, memory mapping, and terminal control. POSIX is not ISO C, and Linux adds further APIs and behavior. Embedded Linux applications can use these interfaces inside a process; bare-metal firmware generally cannot.

## Learning Objectives

- Distinguish ISO C streams and POSIX file descriptors.
- Use open, read, write, close, and poll with partial-result and errno rules.
- Understand feature-test macros and platform availability.
- Recognize process, thread, signal, socket, mmap, ioctl, and termios boundaries.
- Design cancellation, shutdown, and resource-lifetime behavior.
- Keep POSIX dependencies behind platform-specific modules.

## ISO C, POSIX, And Linux

| Layer | Examples | Portability |
| --- | --- | --- |
| ISO C | fopen, fread, malloc, memcpy | C implementations, subject to hosted/freestanding support |
| POSIX | open, read, poll, pthread_create, mmap | POSIX systems |
| Linux-specific | epoll, eventfd, signalfd, ioctl details, sysfs | Linux kernel and libc |
| Project/platform | device nodes, board services, vendor ioctl | Product image and kernel contract |

Compile and link options may expose or hide declarations. Feature-test macros must be defined before system headers:

~~~c
#define _POSIX_C_SOURCE 200809L
#include <unistd.h>
~~~

The supported value depends on the platform and target libc. Do not define a feature macro merely to silence a missing declaration without checking the requested API’s availability.

## File Descriptors

A file descriptor is a process-local integer resource:

~~~c
#define _GNU_SOURCE
#include <fcntl.h>
#include <unistd.h>

int open_device_read_only(const char *path)
{
    int fd = open(path, O_RDONLY | O_CLOEXEC);
    if (fd < 0) {
        return -1;
    }
    return fd;
}

int close_device(int *fd)
{
    if (fd == NULL || *fd < 0) {
        return 0;
    }

    int result = close(*fd);
    *fd = -1;
    return result;
}
~~~

A successful open returns a non-negative descriptor. Close invalidates it, but interrupted or unusual close behavior must be checked against the target platform. Never reuse a descriptor after close without assigning the new value deliberately.

File-descriptor ownership should be explicit. A wrapper that owns a descriptor should close it in its destructor or shutdown path; a borrowed descriptor should not be closed by the callee.

## read And write

POSIX read and write may transfer fewer bytes than requested:

~~~c
#include <errno.h>
#include <stddef.h>
#include <unistd.h>

ssize_t write_all(int fd, const void *data, size_t length)
{
    const unsigned char *bytes = data;
    size_t written = 0u;

    while (written < length) {
        ssize_t result = write(fd, bytes + written, length - written);
        if (result > 0) {
            written += (size_t)result;
            continue;
        }
        if (result < 0 && errno == EINTR) {
            continue;
        }
        return -1;
    }

    return (ssize_t)written;
}
~~~

A nonblocking descriptor may return EAGAIN or EWOULDBLOCK. A device or socket may return zero according to its semantics. Handle cancellation and deadlines; do not spin indefinitely on repeated interruptions or temporary errors.

Do not convert ssize_t to size_t before checking for a negative error result.

## poll And epoll

poll waits for readiness across descriptors:

~~~c
#include <poll.h>

int wait_for_input(int fd, int timeout_ms)
{
    struct pollfd entry = {
        .fd = fd,
        .events = POLLIN
    };

    return poll(&entry, 1u, timeout_ms);
}
~~~

A positive return indicates entries with events, zero is timeout, and negative is error. Inspect revents rather than assuming requested events are the only outcome; hangup, error, and invalid-descriptor flags matter.

Linux epoll scales to many descriptors but is Linux-specific. Model descriptor registration, removal, event edge/level semantics, and lifetime carefully. A closed or reused descriptor can create confusing stale events if shutdown ordering is wrong.

## Processes And Exec

fork duplicates a process address space and execution state; exec replaces the process image. These are powerful hosted/Linux mechanisms with substantial memory, signal, file-descriptor, and error semantics.

Embedded Linux services should define:

- which descriptors survive exec;
- close-on-exec policy;
- signal and child-reaping behavior;
- privilege and environment handling;
- allocation and copy-on-write cost;
- restart and watchdog policy;
- behavior when fork or exec fails.

Do not use fork/exec in a bare-metal or RTOS abstraction layer. Keep process management in a Linux-specific module.

## Threads

POSIX pthreads provide thread creation, synchronization, attributes, and cancellation. A thread function’s context must remain alive until it exits:

~~~c
#include <pthread.h>

static void *worker(void *context)
{
    struct worker_state *state = context;
    process_work(state);
    return NULL;
}

int start_thread(pthread_t *thread, struct worker_state *state)
{
    return pthread_create(thread, NULL, worker, state);
}
~~~

Define join/detach ownership, stack size, scheduling policy, cancellation points, signal masks, and shutdown order. Do not call pthread APIs from a signal handler or assume they map to an RTOS task with identical priority and timing behavior.

## Signals

Signals interrupt normal process execution asynchronously. Use a minimal handler and communicate with the main loop through a safe mechanism:

~~~c
#include <signal.h>

static volatile sig_atomic_t shutdown_requested;

static void handle_shutdown(int signal_number)
{
    (void)signal_number;
    shutdown_requested = 1;
}
~~~

On Linux, signalfd, a self-pipe, or an eventfd can move signal handling into a pollable main loop. The exact option depends on process architecture and portability requirements.

Never perform arbitrary locking, allocation, stdio, or device operations from a signal handler.

## Sockets

Socket APIs add address, protocol, blocking, and partial-operation contracts:

- check address-family and byte-order conversions;
- handle partial send and receive;
- distinguish connection close from transient error;
- set explicit timeouts and nonblocking policy;
- validate message lengths before parsing;
- close descriptors on every failure path;
- avoid treating network input as trusted.

Embedded Linux daemons should separate socket transport from protocol parsing so the parser can be fuzzed and host-tested without a live network.

## mmap

mmap maps files or device memory into a process address space:

~~~c
#include <sys/mman.h>
#include <unistd.h>

void *map_region(int fd, size_t length)
{
    void *region = mmap(NULL, length, PROT_READ | PROT_WRITE,
                        MAP_SHARED, fd, 0);
    return region == MAP_FAILED ? NULL : region;
}
~~~

Check page alignment, offset requirements, protection, cache behavior, synchronization, and lifetime. A mapping must be unmapped with munmap, and a device mapping requires a kernel and hardware contract. Do not use mmap as a portable C memory allocator.

## ioctl And Device Files

ioctl is a device-specific command boundary. Correctness depends on the kernel driver’s command numbers, structure layout, direction, size, compat ABI, and synchronization.

Wrap ioctl calls in a typed project API that validates inputs and translates errors. Do not pass a packed or uninitialized structure directly to a kernel interface. Initialize reserved fields and define 32-bit/64-bit compatibility.

## termios

Serial devices on Linux are configured through termios. A robust serial module defines baud, parity, stop bits, raw/canonical mode, read timeout, flow control, flush policy, and shutdown behavior. It must handle partial reads, hangups, and device removal.

Keep termios setup in a Linux adapter. The portable protocol layer should receive byte spans and return parser statuses.

## Shared Memory And Dynamic Loading

Shared memory requires synchronization, mapping lifetime, ABI layout, cache/coherency assumptions, and crash recovery. Use fixed-width fields, versioning, and robust ownership rules for shared structures.

dlopen and dlsym add runtime symbol and lifetime behavior. They are inappropriate for most safety-critical firmware unless the product explicitly needs dynamic modules and has a signed, versioned loading policy.

## Exercises

1. Write a write_all helper and test EINTR, partial writes, zero-length input, and nonblocking errors.
2. Build a poll loop that handles input, hangup, error, and timeout.
3. Wrap a device descriptor in an owning opaque type with close-on-destroy behavior.
4. Design a Linux serial adapter whose parser is independent of termios.
5. Compare signal handler, self-pipe, and signalfd shutdown designs.
6. Define an ioctl structure with size, version, reserved fields, and explicit error translation.
7. Test mmap and device removal paths without leaking descriptors or mappings.

## Common Mistakes

- Calling POSIX APIs from a portable ISO C or bare-metal layer.
- Treating file descriptors as streams without buffering and ownership analysis.
- Assuming read or write completes the requested length.
- Converting signed syscall results to unsigned before checking errors.
- Ignoring EINTR, EAGAIN, hangup, and partial operations.
- Performing unsafe work in signal handlers.
- Passing uninitialized or ABI-unstable structures to ioctl.
- Treating mmap as ordinary heap memory.
- Forgetting close, unmap, join, or child-reap paths.
- Trusting network or device input without bounds validation.

## Debugging Checklist

1. Record target OS, libc, feature macros, and compiler options.
2. Trace descriptor and mapping ownership from creation to shutdown.
3. Log syscall return values and errno immediately.
4. Test partial, interrupted, nonblocking, timeout, and device-removal cases.
5. Inspect poll revents and socket close behavior.
6. Check ioctl structure sizes, alignment, reserved fields, and compat ABI.
7. Use strace, lsof, gdb, and target logs where available.
8. Keep the protocol and policy layers independent from POSIX transport code.

## Related Topics

- [Standard Library And Ecosystem overview](./index.md)
- [I/O, Diagnostics, And Errors](./io-diagnostics-and-errors.md)
- [Embedded libc Implementations](./embedded-libc.md)
- [Embedded Linux](../platform-specific-c/embedded-linux.md)
- [Linux Userspace And System Programming](../../linux-userspace-and-system-programming/index.md)
- [C Programming](../index.md)

## References

- [POSIX.1-2024 base definitions](https://pubs.opengroup.org/onlinepubs/9799919799/basedefs/V1_chap01.html)
- [POSIX open](https://pubs.opengroup.org/onlinepubs/9799919799/functions/open.html)
- [POSIX read and write](https://pubs.opengroup.org/onlinepubs/9799919799/functions/read.html)
- [Linux man-pages project](https://www.kernel.org/doc/man-pages/)

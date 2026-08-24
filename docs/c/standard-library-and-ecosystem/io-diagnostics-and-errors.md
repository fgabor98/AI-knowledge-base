---
status: draft
reviewed: false
domain: c
difficulty: intermediate
last_reviewed: null
---

# I/O, Diagnostics, And Errors

Input/output is one of the most implementation-dependent parts of C. ISO stdio provides streams and formatted functions; POSIX provides file descriptors and system calls; embedded systems often replace both with bounded logging, UART drivers, trace buffers, or semihosting.

Diagnostics must respect the execution context, memory budget, failure policy, and security boundary.

## Learning Objectives

- Distinguish ISO streams, POSIX file descriptors, and project logging ports.
- Use formatted I/O with correct format and lifetime rules.
- Handle errno and return values without losing context.
- Choose assertions, logs, counters, traces, and fault records appropriately.
- Design bounded diagnostics for embedded and interrupt contexts.
- Prevent format-string, truncation, and secret-disclosure defects.

## Streams And File Descriptors

A hosted C stream is represented by FILE and accessed through stdio functions:

~~~c
#include <stdio.h>

int write_status(FILE *stream, unsigned int status)
{
    if (stream == NULL) {
        return -1;
    }

    if (fprintf(stream, "status=%u\n", status) < 0) {
        return -2;
    }

    return fflush(stream) == 0 ? 0 : -3;
}
~~~

A POSIX file descriptor is a non-negative integer returned by open, socket, pipe, or similar system calls. It is not interchangeable with FILE. A stream may buffer data and own a descriptor; closing one layer while using the other requires an explicit policy.

On bare metal, neither abstraction may exist. A project logger can expose only bounded write and flush operations and map them to a UART, SWO, RTT, ring buffer, or host stderr.

## Formatted Output

The format string defines how variadic arguments are interpreted:

~~~c
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>

void print_sample(uint32_t sequence, int16_t temperature)
{
    printf("sequence=%" PRIu32 " temperature=%" PRId16 "\n",
           sequence, temperature);
}
~~~

Use the correct format macros for fixed-width integers, size_t, pointers, and floating types. A mismatch is undefined behavior, not a cosmetic warning.

Keep format strings constant or validate them. Never treat user-controlled input as the format string:

~~~c
void unsafe_log(const char *message)
{
    printf(message);
}
~~~

Use printf("%s", message) or a typed logger when message is data.

## Input And Partial Results

Input functions can return partial results, end-of-file, errors, or conversion failures. Do not assume one call fills the requested buffer. POSIX read and write have similar partial-operation behavior.

For formatted input, prefer bounded forms and check the conversion count:

~~~c
#include <stdio.h>

int read_word(FILE *stream, char buffer[16])
{
    if (stream == NULL || buffer == NULL) {
        return -1;
    }

    if (fscanf(stream, "%15s", buffer) != 1) {
        buffer[0] = '\0';
        return -2;
    }

    return 0;
}
~~~

For protocol and device input, a byte-oriented state machine is often easier to bound and validate than scanf-family parsing.

## errno

errno is an error indicator associated with many hosted and POSIX interfaces. It is meaningful only when the called function documents that it sets it, and a subsequent library call may change it.

~~~c
#include <errno.h>
#include <stdio.h>
#include <string.h>

int report_open_error(const char *path)
{
    FILE *stream = fopen(path, "rb");
    if (stream != NULL) {
        fclose(stream);
        return 0;
    }

    int saved = errno;
    fprintf(stderr, "open %s failed: %s\n", path, strerror(saved));
    return saved;
}
~~~

Save errno immediately before calling another function. errno is not a universal ISO C error channel and may require thread-local or reentrancy support in the libc. Do not use it in an ISR unless the runtime explicitly supports that context.

## Assertions

Assertions document programmer invariants:

~~~c
#include <assert.h>
#include <stddef.h>

void queue_commit(size_t count, size_t capacity)
{
    assert(count <= capacity);
}
~~~

An assertion may disappear when NDEBUG is defined. Never use it as the only validation for external input, hardware state, or a recoverable allocation failure. In embedded builds, route assertion failures to a bounded fault record, reset policy, debugger breakpoint, or safe-state handler.

An assertion message should identify the invariant and module. Avoid evaluating side effects inside an assertion because release builds may remove them.

## Logging Levels And Policy

A production logger should define:

- severity levels and filtering;
- timestamp source and wrap behavior;
- module and event identifiers;
- formatting and encoding;
- queue capacity and overflow policy;
- blocking or non-blocking behavior;
- allowed execution contexts;
- redaction of credentials and sensitive data;
- behavior during boot, crash, and recovery;
- persistence and rate limiting.

Use numeric or static event identifiers when flash and timing matter:

~~~c
enum log_event {
    LOG_SENSOR_TIMEOUT = 1u,
    LOG_DMA_ERROR = 2u,
    LOG_CONFIG_REJECTED = 3u
};

struct log_record {
    uint32_t timestamp;
    uint16_t event;
    int16_t value;
};
~~~

A binary record can be stored in a ring buffer and formatted later in a host tool, reducing target overhead.

## Logging From Interrupts

An ISR logger should normally enqueue a small preformatted or structured record into preallocated storage:

~~~c
bool log_from_isr(uint16_t event, uint32_t value)
{
    struct log_record record = {
        .timestamp = timer_ticks(),
        .event = event,
        .value = value
    };

    return log_ring_try_push(&record);
}
~~~

The queue operation must be designed for the interrupt context. Do not call general malloc, printf, filesystem functions, or a potentially blocking transport from an ISR.

If the queue is full, choose a documented policy: drop newest, drop oldest, increment a loss counter, trigger a fault, or use a reserved emergency record.

## Fault Diagnostics

Fault paths have limited resources. Capture the smallest useful evidence:

- reset and fault reason;
- program counter and stack pointer when valid;
- active task or interrupt;
- module and event;
- recent state transition;
- watchdog and power status;
- a bounded trace buffer;
- integrity marker and version.

Do not assume the logger, heap, clocks, or filesystem are operational after a severe fault. Use a minimal path and defer formatting or upload until the next safe boot.

## Retargeting Output

A common embedded design is:

~~~c
int board_write(const char *data, size_t length)
{
    for (size_t i = 0u; i < length; ++i) {
        if (uart_putc((unsigned char)data[i]) != 0) {
            return -1;
        }
    }
    return 0;
}
~~~

The libc may call board_write through a syscall stub, or the project may bypass stdio and call a logger directly. Document whether the function blocks, can be interrupted, and is reentrant.

## Security And Diagnostics

Logs can become an information leak. Do not print passwords, keys, tokens, raw personal data, or memory addresses in production without a documented reason. Validate lengths and format specifiers. Treat diagnostic input as untrusted when it comes from a protocol or user.

## Exercises

1. Write tests for format widths, truncation, negative return values, and saved errno.
2. Design an ISR-safe structured logger with full-queue behavior.
3. Replace printf in a firmware path with event IDs and a host decoder.
4. Add an assertion policy for debug, production, and field-recovery builds.
5. Inject short writes, disconnected UART, and logger queue overflow.
6. Review logs for secrets, addresses, unbounded strings, and timing impact.

## Common Mistakes

- Treating FILE streams and file descriptors as the same abstraction.
- Ignoring partial reads and writes.
- Passing a non-constant format string to printf.
- Using the wrong format specifier for a variadic argument.
- Calling strerror or errno-dependent code in an unsupported context.
- Using assertions for input validation or required side effects.
- Calling printf, malloc, or blocking I/O from an ISR.
- Assuming logs always survive a crash.
- Emitting secrets or unbounded user-controlled strings.
- Ignoring logger queue overflow and loss accounting.

## Debugging Checklist

1. Identify the I/O layer: ISO stream, POSIX descriptor, RTOS port, or hardware driver.
2. Check return values, partial results, EOF, and errno rules.
3. Verify formats with compiler warnings and correct headers.
4. Record context, blocking, allocation, and locking behavior.
5. Test full queues, disconnected devices, and fault-path resource loss.
6. Inspect the map file for formatted-I/O and floating-point footprint.
7. Capture structured fault evidence before attempting recovery.
8. Review diagnostic output for sensitive data and unbounded input.

## Related Topics

- [Standard Library And Ecosystem overview](./index.md)
- [Standard Library Overview](./standard-library-overview.md)
- [Memory And String APIs](./memory-and-string-apis.md)
- [POSIX And System Interfaces](./posix-and-system-interfaces.md)
- [Embedded libc Implementations](./embedded-libc.md)
- [C Programming](../index.md)

## References

- [ISO/IEC 9899 standards and drafts](https://open-std.org/jtc1/sc22/wg14/www/standards.html)
- [C11 public draft N1570](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n1570.pdf)
- [POSIX errno and I/O specifications](https://pubs.opengroup.org/onlinepubs/9799919799/functions/V2_chap02.html)
- [GCC format warnings](https://gcc.gnu.org/onlinedocs/gcc/Warning-Options.html)
- [CERT C input/output rules](https://wiki.sei.cmu.edu/confluence/display/c)

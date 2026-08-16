---
status: draft
reviewed: false
domain: c
difficulty: intermediate
last_reviewed: null
---

# Error Handling

Error handling is part of the module contract, not an afterthought attached to a return statement. A useful error design preserves enough information for the caller to choose a safe response without exposing unstable implementation details.

Embedded systems need error policies for invalid input, unavailable hardware, timeouts, resource exhaustion, transient faults, permanent faults, and reset or recovery boundaries.

## Learning Objectives

- Choose status-code and error-enum designs.
- Separate success, absence, retryable failure, and fatal failure.
- Propagate errors without losing context.
- Define output validity on failure.
- Design bounded retry and timeout policies.
- Choose fail-safe, degraded, recovery, or reset behavior.

## Status Codes

A status type should communicate the operation’s result:

~~~c
enum sensor_status {
    SENSOR_OK = 0,
    SENSOR_BAD_ARGUMENT,
    SENSOR_NOT_READY,
    SENSOR_TIMEOUT,
    SENSOR_CRC_ERROR,
    SENSOR_IO_ERROR,
    SENSOR_NO_MEMORY
};
~~~

Keep values stable when they cross a module or process boundary. Do not assume enum storage width for a binary ABI; use a fixed-width integer representation when required.

Reserve one success value and make failure values distinguishable. Do not overload a valid measurement with a magic error value unless the representation and range make that impossible to confuse.

## Result And Output Contracts

Return the primary status and write outputs only under a documented rule:

~~~c
enum sensor_status sensor_read(int16_t *temperature)
{
    int16_t temporary;

    if (temperature == NULL) {
        return SENSOR_BAD_ARGUMENT;
    }
    if (!hardware_read(&temporary)) {
        return SENSOR_IO_ERROR;
    }

    *temperature = temporary;
    return SENSOR_OK;
}
~~~

The temporary prevents a failed operation from partially overwriting the caller’s previous result. If partial output is intentional, return a status that defines exactly which fields are valid.

For multiple outputs, use a result structure with validity rules rather than several unrelated out parameters.

## Error Propagation

Propagate errors at the layer that can make a decision:

~~~c
enum app_status application_start(void)
{
    enum sensor_status sensor_result = sensor_init();
    if (sensor_result != SENSOR_OK) {
        record_sensor_failure(sensor_result);
        return APP_SENSOR_FAILURE;
    }

    if (storage_mount() != 0) {
        sensor_deinit();
        return APP_STORAGE_FAILURE;
    }

    return APP_OK;
}
~~~

Do not collapse every failure into a generic error too early. Preserve the lower-level cause for diagnostics, but translate it into a stable domain-specific status at the boundary.

A useful error record contains operation, module, status, target identifier, timestamp, and recovery action. Avoid logging sensitive payloads or flooding a fault path.

## errno And Hosted Interfaces

The C and POSIX libraries commonly use errno or a return convention for errors. errno is thread-local in many hosted implementations, but it is not a universal ISO C error channel and may not exist in a freestanding environment.

Read or save errno immediately when an interface requires it:

~~~c
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

int open_read_only(const char *path)
{
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        int saved_errno = errno;
        log_open_failure(path, saved_errno);
        return -saved_errno;
    }
    return fd;
}
~~~

Do not use errno for a driver API merely because it is familiar. A typed status can be smaller, deterministic, and easier to test on a microcontroller.

## Retry And Timeout Policy

Retries need a bounded policy:

~~~c
enum io_status {
    IO_OK,
    IO_TIMEOUT,
    IO_PERMANENT_FAILURE
};

enum io_status read_with_retry(unsigned int attempts)
{
    for (unsigned int attempt = 0u; attempt < attempts; ++attempt) {
        if (device_read_once() == 0) {
            return IO_OK;
        }

        wait_backoff(attempt);
    }

    return IO_TIMEOUT;
}
~~~

Define maximum attempts, total time, backoff, cancellation, watchdog servicing, and the fault classes that are retryable. Retrying a permanent configuration or safety fault can make recovery slower or unsafe.

Use wrap-safe time comparisons and avoid unbounded loops. An API should expose whether timeout means no progress, a hardware deadline, or a caller cancellation.

## Fail-Safe And Degraded Behavior

When an operation fails, choose among:

- fail safe: disable outputs or move to a known safe state;
- degrade: continue with reduced capability;
- retry: only for a bounded and recoverable condition;
- isolate: stop one subsystem while keeping others active;
- recover: reinitialize hardware or restart a worker;
- reset: use when state corruption or hardware lockup requires a clean boundary;
- report and continue: only when the failure is non-critical.

The policy belongs to the layer with system context. A low-level driver should report precise facts and perform only the safety actions it owns; application policy should decide whether to retry, degrade, or reset.

## Recovery Versus Reset

A recovery path should define what state is discarded and what remains:

~~~c
enum recovery_result {
    RECOVERY_OK,
    RECOVERY_FAILED
};

enum recovery_result recover_link(void)
{
    link_stop();
    link_flush_queues();
    link_reset_peripheral();

    if (link_configure() != 0) {
        return RECOVERY_FAILED;
    }

    return link_start() == 0 ? RECOVERY_OK : RECOVERY_FAILED;
}
~~~

If recovery fails repeatedly, use a bounded escalation policy. Preserve the original cause and recovery count so a watchdog reset does not erase the diagnosis.

## Assertions And Validation

Assertions are useful for programmer invariants:

~~~c
#include <assert.h>
#include <stddef.h>

struct queue {
    size_t count;
    size_t capacity;
    int *items;
};

void queue_push(struct queue *queue, int value)
{
    assert(queue != NULL);
    assert(queue->count < queue->capacity);
    queue->items[queue->count++] = value;
}
~~~

Do not use an assertion as the only defense against external input, hardware faults, or conditions that can occur in a production build where assertions are disabled. Validate untrusted data and handle operational failures explicitly.

## Error Context

When translating errors, preserve cause and context:

~~~c
struct error_info {
    enum sensor_status status;
    unsigned int operation;
    unsigned int retry_count;
};

enum app_status read_for_application(struct error_info *error)
{
    int16_t value;
    enum sensor_status status = sensor_read(&value);

    if (status != SENSOR_OK) {
        if (error != NULL) {
            error->status = status;
            error->operation = 1u;
            error->retry_count = 0u;
        }
        return APP_SENSOR_FAILURE;
    }

    publish_temperature(value);
    return APP_OK;
}
~~~

Do not expose a pointer to a stack-local error object. Decide whether error information is thread-local, returned by value, passed by output pointer, or stored in an owned diagnostic record.

## Exercises

1. Define a status enum for a UART transaction with invalid input, busy, timeout, framing error, and hardware fault.
2. Implement a function that preserves its output on every failure path.
3. Add bounded retry with a total deadline and cancellation.
4. Translate low-level errors into application statuses while retaining diagnostic cause.
5. Design fail-safe behavior for a sensor, motor, and communication subsystem.
6. Inject each error at initialization, operation, and shutdown and verify recovery order.

## Common Mistakes

- Returning magic values that overlap valid results.
- Overwriting output before success is known.
- Collapsing every error into failure without cause.
- Retrying permanent or safety-critical failures.
- Using unbounded retry loops.
- Assuming errno exists or is stable in freestanding code.
- Disabling assertions and losing all input validation.
- Resetting without preserving the original fault context.
- Logging from an interrupt or fault path without a bounded strategy.
- Making a low-level module decide application policy it cannot know.

## Debugging Checklist

1. Enumerate every status and define its caller-visible meaning.
2. Check output validity on every return path.
3. Record operation, cause, context, retry count, and recovery action.
4. Test zero attempts, one attempt, timeout, cancellation, and permanent failure.
5. Verify retry time against watchdog and real-time budgets.
6. Fault-inject hardware, allocation, parsing, and shutdown failures.
7. Check that fail-safe actions execute before resource release.
8. Preserve diagnostic evidence across recovery and reset boundaries.

## Related Topics

- [Modular Design And APIs overview](./index.md)
- [APIs And Opaque Types](./api-and-opaque-types.md)
- [Ownership And Resource Lifetimes](./ownership-and-resource-lifetimes.md)
- [Correctness, Quality, And Security](../correctness-quality-and-security/index.md)
- [Embedded C And Hardware](../embedded-c-and-hardware/index.md)
- [C Programming](../index.md)

## References

- [ISO/IEC 9899 standards and drafts](https://open-std.org/jtc1/sc22/wg14/www/standards.html)
- [C11 public draft N1570](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n1570.pdf)
- [POSIX errno definitions](https://pubs.opengroup.org/onlinepubs/9799919799/basedefs/errno.h.html)
- [CERT C error-handling rules](https://wiki.sei.cmu.edu/confluence/display/c)

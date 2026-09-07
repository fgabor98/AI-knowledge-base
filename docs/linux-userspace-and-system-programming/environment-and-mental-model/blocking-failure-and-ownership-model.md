---
status: draft
reviewed: false
domain: linux-userspace
difficulty: beginner
last_reviewed: null
---

# Blocking, Failure, And Ownership Model

## What problem does this solve?

Most production failures are not caused by the successful path being impossible.
They happen when an operation waits longer than expected, returns partial progress,
is interrupted, loses its peer, exhausts a resource, or is cleaned up by the wrong
owner. A userspace design becomes reliable when every externally visible operation
has an explicit answer to four questions:

1. Can it block, and for how long?
2. How can it fail or make partial progress?
3. Who owns every object before and after the call?
4. What state and evidence remain after cancellation, timeout, crash, or restart?

## The operation contract

Use this table before implementing a call or protocol step:

| Dimension | Required decision |
| --- | --- |
| Preconditions | What must be true about the FD, buffer, state, credentials, mount, and peer? |
| Progress | Can the call return short success, wait, or make an externally visible partial change? |
| Blocking | Which event releases it; can a deadline be enforced; what wakes it during shutdown? |
| Failure | Which errors are permanent, transient, retryable, or evidence of a programming bug? |
| Interruption | What does `EINTR`, cancellation, disconnect, or signal delivery mean? |
| Ownership | Who closes, frees, joins, unlocks, commits, or rolls back? |
| Lifetime | Which objects must outlive the operation and which become invalid afterward? |
| Recovery | Retry, reconnect, reinitialize, degrade, fail closed, or terminate? |
| Observability | What operation ID, state, duration, count, and cause are logged? |

If the answer is “the library handles it,” identify the exact library contract. A
retry loop, timeout, or cleanup action hidden in a wrapper is still part of the
application’s design and must be documented there.

## Blocking is a state transition

Blocking is not merely “slow.” It means the thread cannot make progress on that call
until an event, resource, or timeout changes. Common blockers are:

- no bytes available on a pipe, socket, terminal, or device;
- a full pipe or socket send buffer;
- a mutex or file lock held by another actor;
- page faults, memory reclaim, or storage I/O;
- DNS, network connection, or peer response;
- a child process that has not exited;
- a device interrupt, firmware response, or bus transaction;
- filesystem metadata or durability work.

For every blocking point choose an architecture:

```text
blocking operation
    +--> bounded deadline and interruptible wait
    +--> readiness-driven event loop
    +--> dedicated worker with explicit queue bound
    +--> synchronous operation accepted by the product budget
```

“Put it in a thread” changes which thread is blocked; it does not create a timeout,
bound memory, or define shutdown. A worker still needs a queue limit, cancellation
policy, and join/termination deadline.

## Timeouts and deadlines

Use a monotonic deadline for elapsed-time budgets. A wall clock can jump because of
NTP, RTC correction, manual adjustment, or suspend policy. Prefer an absolute
deadline over repeatedly adding a relative timeout:

```text
deadline = monotonic_now() + budget
loop:
    remaining = deadline - monotonic_now()
    if remaining <= 0: timeout
    wait(remaining)
    if interrupted: continue with the same deadline
```

Do not blindly restart a call after `EINTR` if the operation may have produced a
side effect or partial result. Re-read the interface contract and preserve the
original deadline.

Timeout means “the caller stopped waiting.” It does not necessarily mean that the
kernel, driver, peer, or hardware stopped. If the operation can continue after the
caller times out, give it a cancellation or sequence mechanism and define how late
completion is discarded.

## Failure classes

| Class | Examples | Typical policy |
| --- | --- | --- |
| Programmer/precondition | `EINVAL`, invalid state, bad pointer | Fix code; do not retry blindly |
| Deployment/configuration | `ENOENT`, `EACCES`, missing mount, wrong ABI | Fail with actionable evidence; repair image or policy |
| Permanent resource | unsupported UAPI, missing device, read-only policy | Degrade or fail startup according to product requirements |
| Transient availability | `EAGAIN`, `EBUSY`, not ready | Retry only with a bound, backoff, and readiness reason |
| Interruption | `EINTR`, cancellation, shutdown request | Preserve state; unwind or resume according to the contract |
| Peer/lifecycle | `EPIPE`, `ECONNRESET`, child exit | Close or reconnect; invalidate dependent state |
| Capacity | `ENOMEM`, `EMFILE`, `ENFILE`, `ENOSPC`, queue full | Apply backpressure or shed work; emit high-value diagnostics |
| Data/protocol | malformed frame, range error, checksum failure | Reject safely; never advance state on unvalidated data |
| Hardware/transport | `EIO`, `ETIMEDOUT`, `ENODEV` | Record context; reinitialize or enter a degraded state |

An error number is a clue from one interface, not a complete diagnosis. Combine it
with operation, path/FD, state, duration, peer, sequence, and target evidence.

## Partial success is still success with obligations

For `read` and `write`, a positive count means that count of bytes was transferred;
it does not mean the requested length. For a stream protocol:

```c
while (sent < message_length) {
    ssize_t n = write(fd, message + sent, message_length - sent);
    if (n > 0) {
        sent += (size_t)n;
        continue;
    }
    if (n == -1 && errno == EINTR) {
        continue;
    }
    if (n == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
        /* Wait for writable readiness before trying again. */
        break;
    }
    /* Preserve sent and classify the operation as incomplete. */
    break;
}
```

For a file write, a successful `write` does not prove power-loss durability. For a
device configuration call, a successful command may mean only that the request was
queued. For a transaction, record whether the state is `not_started`, `in_progress`,
`committed`, `rolled_back`, or `unknown`; do not reduce it to a Boolean.

## Ownership and lifetime

Ownership answers who is responsible for release and who may mutate an object. A
simple convention is:

```text
creator owns resource
transfer is explicit in the API name or documentation
borrowed arguments remain valid for the documented call duration
returned objects are owned by the caller unless stated otherwise
one owner performs final close/free/join/unlock
```

Examples:

- An FD returned by `open` belongs to the caller. A function that takes it without
  closing it must say “borrowed”; a function that closes it must say “consumes.”
- A buffer passed to a synchronous `write` is usually borrowed for that call. An
  asynchronous queue must copy it or transfer its ownership and lifetime.
- A `pthread_t` must have a join/detach owner. “The thread will clean itself up” is
  incomplete if another thread still needs its result or stack resources.
- A mutex has an unlock obligation on every path after successful lock acquisition.
- A mapped region becomes invalid after `munmap`; storing its pointer elsewhere is a
  lifetime bug even if the virtual address is later reused.

Write ownership into types, names, comments, or wrapper APIs when C cannot express it:
`owned_fd`, `borrowed_buffer`, `take_connection`, `close_device`, and
`join_worker` communicate more than a bare `int` or `void *`.

## Cleanup is part of the failure path

Use one cleanup plan with reverse acquisition order:

```text
acquire config
acquire memory
acquire fd
acquire worker
    |
failure at any point
    v
stop worker -> join worker -> close fd -> free memory -> release config
```

Do not overwrite the original failure while cleaning up. In C, save it and report
cleanup failures separately when they matter:

```c
int saved_errno = errno;
if (close(fd) == -1) {
    fprintf(stderr, "close: %s\n", strerror(errno));
}
errno = saved_errno;
```

For durable state, cleanup is not enough. The design needs commit markers, atomic
replacement, fsync policy, or a recovery record. For a service, cleanup also includes
stopping child processes, unregistering event sources, and making the next startup
safe after a crash.

## A bounded state machine

Hardware-facing operations benefit from explicit states:

```text
DISCONNECTED
    | open succeeds
    v
OPEN
    | configure succeeds
    v
READY -- request --> BUSY -- completion --> READY
  |                   |
  | timeout/error     | disconnect/cancel
  v                   v
DEGRADED <-------- RECOVERING
  |                          |
  +------ retry/backoff -----+
```

Each transition should name:

- the event and precondition;
- the system calls and possible partial effects;
- the deadline and cancellation behavior;
- resources owned in the state;
- the next retry or terminal policy;
- evidence written to logs or metrics.

The state machine prevents a timeout from silently leaving a device half-configured
while another request assumes `READY`.

## Minimal contract exercise

Choose one operation from the probe or a real service and fill this in:

```text
Operation:
Owner before call:
Inputs and valid ranges:
May block because:
Deadline:
Success means:
Partial success means:
EINTR means:
Transient errors:
Permanent errors:
Cancellation action:
Resources acquired:
Resources released:
State after timeout:
State after process restart:
Log fields:
Test that proves each branch:
```

Then deliberately induce at least three branches: missing resource, timeout or
interruption, and cleanup after a partially completed setup.

## Common mistakes

- Retrying every error, including invalid arguments and unsupported operations.
- Retrying with a fresh timeout on every `EINTR`, extending the total wait forever.
- Assuming `close`, `fsync`, `munmap`, `pthread_join`, or cleanup calls cannot fail.
- Freeing or closing a resource while another thread or asynchronous operation uses it.
- Using an unbounded queue to hide a slow consumer.
- Treating timeout as cancellation without proving the lower layer stopped.
- Logging only “operation failed” without operation, state, duration, target, and cause.
- Performing blocking I/O in a signal handler or while holding an unrelated global lock.

## Debugging checklist

- Draw the resource ownership graph and mark every transfer.
- List every blocking point and the event/deadline that releases it.
- Test short I/O, `EINTR`, `EAGAIN`, peer death, full storage, missing device, and
  process termination.
- Check whether a timeout leaves an in-flight operation behind.
- Check cleanup in reverse order and preserve the first failure.
- Bound retries, queue depth, memory, descriptors, threads, and log volume.
- Capture monotonic durations and state transitions, not just wall-clock messages.
- Re-run after crash, restart, power cycle, and partial deployment.

## Related topics

- [Stage 0: Environment And Mental Model](index.md)
- [System-Call Contracts And Errors](../system-calls-files-and-file-descriptors/system-call-contracts-and-errors.md)
- [Blocking, Nonblocking, And Partial I/O](../system-calls-files-and-file-descriptors/blocking-nonblocking-and-partial-io.md)
- [Cancellation, Priority, And Realtime Scheduling](../threads-and-userspace-concurrency/cancellation-priority-and-realtime-scheduling.md)
- [Atomic Persistence And Schema Migration](../persistent-state-storage-and-power-loss/atomic-persistence-and-schema-migration.md)

## References

- [`read(2)`](https://man7.org/linux/man-pages/man2/read.2.html)
- [`write(2)`](https://man7.org/linux/man-pages/man2/write.2.html)
- [`errno(3)`](https://man7.org/linux/man-pages/man3/errno.3.html)
- [POSIX cancellation points](https://pubs.opengroup.org/onlinepubs/9799919799/functions/V2_chap02.html)
- [`clock_gettime(2)`](https://man7.org/linux/man-pages/man2/clock_gettime.2.html)

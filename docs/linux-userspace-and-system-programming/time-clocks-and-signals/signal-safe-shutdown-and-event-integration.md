---
status: draft
reviewed: false
domain: linux-userspace
difficulty: intermediate
last_reviewed: null
---

# Signal-Safe Shutdown And Event Integration

## What problem does this solve?

A process must stop safely while it may be blocked in I/O, holding resources, or
processing a protocol message. Calling ordinary shutdown code from a signal handler
is unsafe. The solution is to convert asynchronous notification into ordinary,
serialized control flow.

## The flag pattern

```text
signal handler: stop_requested = 1
main loop:      wake -> observe flag -> stop accepting work -> drain/cancel -> cleanup
```

The flag is sufficient only if the main loop wakes promptly. A thread blocked forever
in `read` will not observe it. Use a timeout, self-pipe, eventfd, `signalfd`, or a
descriptor that the shutdown owner can safely close according to the I/O contract.

## Self-pipe and `signalfd`

A self-pipe handler writes one byte to a pre-created nonblocking pipe; the event loop
reads and handles the signal in normal context. The write must be bounded and
async-signal-safe, and a full pipe needs a policy because multiple standard signals
can coalesce.

Linux `signalfd` turns blocked signals into records read through an FD:

```text
block signals in all relevant threads
create signalfd for the blocked set
add it to poll/epoll
read signalfd_siginfo records in the event loop
perform shutdown/reload/reap work outside a handler
```

The signal mask must be set before creating worker threads so signals do not race to
an unexpected handler thread. `signalfd` is Linux-specific and its FD has normal
ownership, nonblocking, close-on-exec, and namespace considerations.

## Shutdown state machine

```text
RUNNING
  | SIGTERM / operator request
  v
QUIESCING -- stop intake --> DRAINING -- all work complete --> CLEANUP --> EXIT
     |                              |
     +-- deadline ------------------+-- cancel/close --> FORCED_EXIT
```

Define what happens to in-flight device commands, queued requests, child processes,
temporary files, locks, sockets, and persistent transactions. A graceful shutdown
that waits forever is not graceful; it is a hang.

## Reload versus shutdown

Reload is a transaction, not “reread the file in a handler.” Parse and validate new
configuration off to the side, apply changes in a defined order, and retain the old
valid configuration if application fails. Assign a generation so requests can record
which configuration they used.

## Signals and threads

Centralize signal handling. One event-loop thread can own signal FDs while workers
receive commands through a queue or eventfd. Do not have every worker install a
handler and mutate shared state. Join workers after intake stops and before destroying
the state they access.

## Testing matrix

Inject shutdown:

- while blocked in each I/O source;
- during partial read/write or protocol parsing;
- while a child is being launched or waited for;
- while a worker owns a resource or queue item;
- during configuration reload and persistence commit;
- repeatedly and at the exact shutdown deadline.

Verify no FD, thread, child, lock, temporary file, or stale socket remains beyond its
documented lifetime. Test SIGKILL separately as an unclean path requiring startup
recovery.

## Common mistakes

- Calling `printf`, malloc, locks, or `close` indiscriminately from a handler.
- Setting a flag without waking an indefinitely blocked loop.
- Handling signals in arbitrary worker threads.
- Treating signalfd records as durable queued commands without a bound.
- Reloading configuration in place before validation completes.
- Destroying shared state before workers have joined.
- Assuming graceful cleanup occurs after SIGKILL or power loss.

## Debugging checklist

- Record signal source, thread, state, deadline, and shutdown transition.
- Inspect masks and signalfd/self-pipe ownership.
- Trace the event loop’s wakeup and drain behavior.
- Test every blocking source, partial operation, child, and persistence boundary.
- Check worker join, descriptor closure, child reaping, and final exit status.
- Preserve crash and forced-shutdown evidence before restart.

## Related topics

- [Stage 5: Time, Clocks, And Signals](index.md)
- [Signal Model And sigaction](signal-model-and-sigaction.md)
- [Event Loops, select, poll, And epoll](../ipc-and-event-driven-design/event-loops-select-poll-and-epoll.md)
- [Service Lifecycle, Readiness, And Restart](../services-init-and-systemd/service-lifecycle-readiness-and-restart.md)

## References

- [`signalfd(2)`](https://man7.org/linux/man-pages/man2/signalfd.2.html)
- [`signal-safety(7)`](https://man7.org/linux/man-pages/man7/signal-safety.7.html)
- [`sigwaitinfo(2)`](https://man7.org/linux/man-pages/man2/sigwaitinfo.2.html)
- [`eventfd(2)`](https://man7.org/linux/man-pages/man2/eventfd.2.html)

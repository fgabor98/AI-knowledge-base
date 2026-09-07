---
status: draft
reviewed: false
domain: linux-userspace
difficulty: intermediate
last_reviewed: null
---

# Signal Model And sigaction

## What problem does this solve?

Signals interrupt normal control flow and can arrive at inconvenient points. Their
default actions range from ignore to stop to process termination and core dump. A
reliable service defines dispositions and masks explicitly and keeps handlers tiny.

## Signal state

Each signal has:

- a disposition: default, ignore, or installed handler;
- a blocked mask, generally per-thread;
- pending state, with standard signals coalescing rather than forming an unbounded
  queue;
- a delivery target: process, thread, process group, or supervisor;
- default action and possibly an accompanying `siginfo_t` for realtime signals.

Signals do not provide a general durable message queue. If multiple ordinary signals
arrive before delivery, the application may observe one pending instance. Realtime
signals queue under limits and carry values, but still need a defined ownership and
backpressure policy.

## Install with `sigaction`

```c
static volatile sig_atomic_t stop_requested;

static void on_term(int signal_number)
{
    (void)signal_number;
    stop_requested = 1;
}

struct sigaction action = {
    .sa_handler = on_term,
};
sigemptyset(&action.sa_mask);
action.sa_flags = 0;
if (sigaction(SIGTERM, &action, NULL) == -1) {
    /* report failure before entering service state */
}
```

Use `sigaction`, not obsolete `signal`, so restart and mask semantics are explicit.
`SA_RESTART` can restart some interrupted calls, but not every interface and not
necessarily the behavior a deadline-sensitive service wants. Test the actual calls.

## Async-signal safety

Inside a handler, use only async-signal-safe operations. Setting a
`volatile sig_atomic_t` flag is a common pattern. Do not call `malloc`, `free`,
`printf`, most logging APIs, pthread locks, or non-reentrant library code. A handler
can interrupt code while it holds libc or application locks.

For fatal signals, a minimal handler may write a fixed diagnostic to a pre-opened FD
and restore/default-terminate; complex recovery in a corrupt process is unsafe. Let
the supervisor and core-dump system preserve evidence.

## Masks and delivery

`sigprocmask` is for single-threaded processes; use `pthread_sigmask` in threaded
programs. A common architecture blocks selected signals in all worker threads and
dedicates one thread or event loop to `sigwaitinfo`/`signalfd`.

Process-directed signals may be delivered to any eligible thread. Thread-directed
signals target a specific thread. A blocked signal remains pending until unblocked,
waited for, or handled according to its disposition.

Important service signals:

| Signal | Typical policy |
| --- | --- |
| `SIGTERM` | Graceful supervisor shutdown |
| `SIGINT` | Interactive shutdown; often same state transition as SIGTERM |
| `SIGHUP` | Reload only if the service defines safe reload semantics |
| `SIGCHLD` | Child lifecycle notification; reap through one owner |
| `SIGPIPE` | Ignore/block and handle `EPIPE`, or use a scoped socket option |
| `SIGALRM` | Avoid as a hidden global timer in complex services |
| `SIGSEGV`, `SIGBUS` | Fatal evidence path, not ordinary recovery |

`SIGKILL` and `SIGSTOP` cannot be caught, blocked, or ignored.

## Common mistakes

- Performing stdio, allocation, locking, or complex logging in a handler.
- Assuming standard signals queue one notification per event.
- Installing dispositions in one thread and assuming masks are process-wide.
- Using `SA_RESTART` without testing deadline and shutdown behavior.
- Treating SIGHUP as a universal reload protocol.
- Handling fatal signals by attempting to continue with corrupted state.
- Competing signal handlers or child reapers in different modules.

## Debugging checklist

- Inspect `/proc/<pid>/status` signal masks, pending, and caught/ignored sets.
- Record signal sender, target PID/TID/PGID, disposition, and process state.
- Test SIGTERM during every blocking call and state transition.
- Test SIGPIPE, SIGCHLD, SIGHUP, parent death, and fatal-signal evidence paths.
- Check `SA_RESTART`, masks, realtime signal limits, and supervisor policy.
- Keep handler work auditable and async-signal-safe.

## Related topics

- [Stage 5: Time, Clocks, And Signals](index.md)
- [Signal-Safe Shutdown And Event Integration](signal-safe-shutdown-and-event-integration.md)
- [Exit, Waiting, And Zombies](../processes-and-program-lifetime/exit-waiting-and-zombies.md)
- [Eventfd, timerfd, signalfd, And inotify](../ipc-and-event-driven-design/eventfd-timerfd-signalfd-and-inotify.md)

## References

- [`signal(7)`](https://man7.org/linux/man-pages/man7/signal.7.html)
- [`sigaction(2)`](https://man7.org/linux/man-pages/man2/sigaction.2.html)
- [`signal-safety(7)`](https://man7.org/linux/man-pages/man7/signal-safety.7.html)
- [`sigprocmask(2)`](https://man7.org/linux/man-pages/man2/sigprocmask.2.html)

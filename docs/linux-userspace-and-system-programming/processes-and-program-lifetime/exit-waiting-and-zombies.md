---
status: draft
reviewed: false
domain: linux-userspace
difficulty: beginner
last_reviewed: null
---

# Exit, Waiting, And Zombies

## What problem does this solve?

Process termination has two sides. The child stops executing, but the kernel retains
a small termination record until a parent collects it. If the parent never waits,
zombies accumulate. If a supervisor restarts a process without preserving the first
failure evidence, a crash loop becomes difficult to diagnose.

## Termination paths

| Path | Meaning |
| --- | --- |
| `return` from `main` | Equivalent to calling `exit` with the returned status |
| `exit(status)` | Runs `atexit` handlers and flushes stdio, then terminates the process |
| `_exit(status)` / `_Exit` | Terminates immediately without user-space exit handlers or stdio flushing |
| fatal signal | Kernel applies signal’s default action, possibly creating a core dump |
| supervisor kill | Process may be terminated by `SIGTERM`, deadline escalation, or `SIGKILL` |

Threads calling `pthread_exit` terminate only themselves. Calling `exit` from one
thread terminates the whole process, so thread shutdown ownership must be explicit.

## Waiting and status decoding

```c
int status;
pid_t result = waitpid(child_pid, &status, 0);
if (result == -1) {
    /* EINTR may mean retry; ECHILD means ownership or lifecycle changed. */
} else if (WIFEXITED(status)) {
    printf("exit status=%d\n", WEXITSTATUS(status));
} else if (WIFSIGNALED(status)) {
    printf("signal=%d core=%d\n", WTERMSIG(status),
           WCOREDUMP(status) != 0);
}
```

Use the macros, not bit shifts or assumptions about the integer representation. With
`WUNTRACED`/`WCONTINUED`, also handle stopped and continued children. `waitid` can
provide a `siginfo_t` result and `WNOWAIT` can inspect without consuming the record;
use it when the product needs that precision.

`waitpid(-1, ...)` means any child, while a positive PID selects one. A supervisor
must not accidentally reap a child owned by another subsystem unless that ownership
is intentional.

## Zombies and reparenting

A zombie has exited but still has a PID and status record because its parent has not
waited. It consumes a process-table slot, not the child’s address space. The parent
should wait promptly. If the parent exits, the child is reparented to an eligible
subreaper or PID 1, which then owns the wait obligation.

PID 1 has special responsibilities: it must reap orphaned children and handle signals
appropriately. A tiny init that does neither can eventually exhaust the process table
or fail to shut down its descendants. In a container, PID 1 is the container’s init,
not necessarily the host’s systemd.

## SIGCHLD strategies

Choose one owner and one strategy:

- block `SIGCHLD` and consume it through `sigwaitinfo` or `signalfd` in an event loop;
- install a handler that records minimal state and performs safe reaping later;
- use a supervisor API or `pidfd` plus a defined wait path;
- use `SA_NOCLDWAIT`/ignore semantics only when losing child status is acceptable.

Do not call complex, non-async-signal-safe code from a signal handler. Do not mix
multiple components that all believe they own child reaping.

## Shutdown and restart

A robust supervisor sequence is:

```text
send SIGTERM to the intended process/group
wait for graceful exit until monotonic deadline
capture status and final logs
send SIGKILL only to the intended remaining scope
reap every child
apply bounded backoff before restart
preserve the first failure in the crash record
```

`SIGKILL` cannot be handled and may leave external state half-complete. The next
start must detect stale locks, sockets, temporary files, transactions, and device
state. Restart policy must distinguish a clean administrative stop from a crash and
avoid infinite rapid restart storms.

## Common mistakes

- Forgetting to wait because the child “already exited.”
- Treating any nonzero exit status as a signal or vice versa.
- Calling `exit` in a post-fork child and flushing duplicated stdio buffers.
- Reaping children from more than one ownership domain.
- Sending `SIGTERM` to a PID while descendants continue running.
- Using `SIGKILL` immediately and losing the first failure evidence.
- Treating PID 1 in a container as a normal application process.

## Debugging checklist

- Inspect `ps` states for `Z` and parent relationships.
- Capture `waitpid`/`waitid` result and decoded status.
- Check core-dump policy, signal, backtrace, and final logs.
- Check process groups and descendants during shutdown.
- Check restart counters, backoff, and the first crash record.
- Test child exits before parent setup completes, child `exec` failure, signal death,
  parent death, and supervisor shutdown.

## Related topics

- [Stage 2: Processes And Program Lifetime](index.md)
- [fork, exec, And posix_spawn](fork-exec-and-spawn.md)
- [Sessions, Process Groups, And Job Control](sessions-process-groups-and-job-control.md)
- [Service Lifecycle, Readiness, And Restart](../services-init-and-systemd/service-lifecycle-readiness-and-restart.md)

## References

- [`wait(2)`](https://man7.org/linux/man-pages/man2/wait.2.html)
- [`waitpid(2)`](https://man7.org/linux/man-pages/man2/waitpid.2.html)
- [`waitid(2)`](https://man7.org/linux/man-pages/man2/waitid.2.html)
- [`signal(7)`](https://man7.org/linux/man-pages/man7/signal.7.html)

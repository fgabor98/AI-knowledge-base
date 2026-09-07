---
status: draft
reviewed: false
domain: linux-userspace
difficulty: advanced
last_reviewed: null
---

# Strace, Procfs, And Runtime Inspection

## `strace`

Trace the boundary relevant to the question:

```sh
strace -ff -ttt -T -yy -s 256 -o /tmp/app.trace ./app
strace -f -e trace=file,network -p "$PID"
```

Use timestamps and syscall durations to distinguish waiting from computation. `-ff` separates threads/processes; `-yy` resolves descriptor targets where possible. Filter aggressively when output is large, and remember that tracing changes timing, signal delivery, and sometimes scheduling.

Look for the first unexpected `ENOENT`, `EACCES`, `EPERM`, `EINTR`, short read/write, timeout, or descriptor reuse—not merely the final failure. Compare a good and bad trace around the divergence.

## `/proc` and `/sys`

Useful runtime evidence includes `/proc/PID/status`, `cmdline`, `fd/`, `maps`, `smaps_rollup`, `limits`, `mountinfo`, `cgroup`, `sched`, and namespace links. `/proc/$PID/fd` reveals what a descriptor actually references; `lsof` is a convenience wrapper, not a replacement for understanding the source.

Read procfs defensively: fields vary with kernel version and permissions, and a process can exit between listing and reading. For a service, capture cgroup pressure, memory events, OOM records, open descriptors, thread count, and mount visibility.

## Attach safely

Ptrace restrictions, capabilities, namespaces, and dumpability may prevent attaching. Prefer reproducing under a debugger or enabling targeted diagnostics rather than weakening production policy globally. Never treat an attached process as quiescent; reads and injected stops can affect the failure.

## Read a trace as a protocol

This is an illustrative trace fragment, not a command:

```text
connect(7, ..., ...) = -1 EINPROGRESS
poll([{fd=7, events=POLLOUT}], 1, 800) = 1
getsockopt(7, SOL_SOCKET, SO_ERROR, [0], ...) = 0
write(7, ..., 100) = 40
write(7, ..., 60) = -1 EAGAIN
```

Connection setup succeeded; only 40 bytes of the intended message were
accepted. The correct next action is to preserve the remaining bytes and
wait for writability using the original deadline. Retrying the complete
100-byte frame would duplicate its prefix.

A loader probing several nonexistent search paths can legitimately return
many `ENOENT` results. Find the first failure that changes expected behavior,
not the first negative syscall result. Long gaps outside traced syscalls can
be userspace execution, descheduling, or missing thread coverage; syscall
duration alone does not attribute the cause.

## Observe the intended process

`/proc/self` describes the program opening it. Running `cat /proc/self/status`
reports cat's state, not the service's. Use `/proc/$pid` and record its start
identity before and after a multi-file collection. `environ` is NUL-separated
and does not reliably reflect all later application environment changes.

Inspect `fdinfo` together with `fd`: an integer FD may have been closed and
reused. Pair a persistent pipe writer reference with a reader blocked awaiting
EOF to explain a shutdown hang. Trace attachment itself changes scheduling;
confirm the repaired behavior without attachment.

## Related topics

- [Stage 14 overview](index.md)
- [Testing and verification](../testing-and-verification/index.md)

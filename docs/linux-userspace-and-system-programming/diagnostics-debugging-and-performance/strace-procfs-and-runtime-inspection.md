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

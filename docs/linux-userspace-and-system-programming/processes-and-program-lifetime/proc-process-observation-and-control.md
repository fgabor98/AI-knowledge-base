---
status: draft
reviewed: false
domain: linux-userspace
difficulty: beginner
last_reviewed: null
---

# /proc Process Observation And Control

## What problem does this solve?

When a service is “running” but not working, `/proc` can distinguish a crash, sleep,
blocked I/O, excessive memory use, descriptor leak, signal wait, or wrong executable.
It is a live kernel view with race conditions and permission limits—not a consistent
database snapshot—so observations must be timestamped and correlated.

## High-value process entries

| Entry | Use |
| --- | --- |
| `status` | State, IDs, memory summary, groups, capabilities, signal masks |
| `cmdline` | Arguments as NUL-separated bytes; may be empty or intentionally changed |
| `environ` | Initial environment storage; not a reliable view of later `setenv`/`putenv` changes |
| `exe` | Link to executable file, if accessible |
| `cwd`, `root` | Process directory and root views |
| `fd/` | Descriptor targets; access may race with close/reuse |
| `fdinfo/` | Per-descriptor position, flags, and facility-specific details |
| `maps`, `smaps` | Mappings; `smaps` has memory accounting and costs more to read |
| `limits` | Resource limits |
| `stat`, `statm` | Kernel accounting; `stat` has parsing and field semantics traps |
| `task/` | Per-thread IDs and state |
| `ns/` | Namespace handles and identity |
| `oom_score`, `cgroup` | OOM and resource-control context |

```sh
pid=1234
cat "/proc/$pid/status"
tr '\0' ' ' < "/proc/$pid/cmdline"; echo
readlink "/proc/$pid/exe"
readlink "/proc/$pid/cwd"
ls -l "/proc/$pid/fd"
cat "/proc/$pid/limits"
cat "/proc/$pid/wchan" 2>/dev/null || true
```

Avoid exposing `environ`, command-line secrets, or memory maps in support bundles
without a redaction policy.

## Process states and blocked work

`status` reports a state such as running, sleeping, disk sleep, stopped, zombie, or
dead. A sleeping process is not necessarily broken: it may be waiting for an event.
An uninterruptible sleep can indicate storage or device work, but one snapshot is not
enough to establish a hang.

Correlate:

- repeated state and CPU observations;
- `wchan` and kernel stack evidence where permitted;
- `strace -p` for system-call waits;
- FD targets and readiness sources;
- logs, deadlines, watchdog events, and thread states.

## Memory and descriptors

`maps` shows what is mapped, not how much is resident. Use `smaps` or
`smaps_rollup` for RSS/PSS/private/shared details where available. A growing virtual
size may be harmless address reservation; growing RSS or dirty memory is a different
signal. `/proc/<pid>/fd` reveals descriptor leaks and deleted-but-open files.

Descriptor numbers can be reused immediately after close. Record the observation time
and inspect the owning process while it is still in the suspected state.

## Safe control versus observation

Some proc files are writable controls, such as selected `/proc/<pid>/oom_score_adj`
or `/proc/sys` entries. Writing them changes process or kernel policy and needs an
explicit privilege and product decision. Read-only observation should be preferred
in diagnosis; never use a diagnostic command that mutates scheduler, memory, or
sysctl state without recording the change.

## Tools around `/proc`

```sh
ps -e -o pid,ppid,stat,etime,%cpu,%mem,cmd
top -H -p "$pid"
pidstat -p "$pid" 1 5
lsns -p "$pid"
cat "/proc/$pid/mountinfo"
```

Tool availability varies on embedded images. The underlying `/proc` files are often
the portable fallback within Linux, but field formats and visibility are version and
policy dependent.

## Observation record

```text
timestamp (monotonic and wall):
pid / start identity:
executable / arguments:
state / CPU / RSS / FD count:
thread states:
current syscall or wchan:
credentials / namespaces / cgroup:
relevant logs and deadlines:
interpretation:
next non-mutating test:
```

Capture several records rather than one giant snapshot. A process can exit between
reading `status` and `fd`, and a new process can reuse the same PID.

## Common mistakes

- Parsing `/proc/<pid>/stat` without handling the parenthesized command name carefully.
- Treating `/proc` values as a stable transaction.
- Assuming empty `cmdline` means no arguments or a dead process.
- Reading sensitive `environ` and maps into unrestricted logs.
- Concluding “CPU idle means hung” or “sleeping means dead.”
- Acting on a PID after it may have been reused.
- Writing proc controls as part of a diagnostic script without rollback.

## Debugging checklist

- Capture PID start identity before collecting other files.
- Read `status`, `fd`, `limits`, `maps`, `task`, `ns`, and cgroup context.
- Sample state over time and correlate with system calls and logs.
- Check credentials and proc visibility restrictions.
- Use `strace` or debugger attachment only with operational approval and awareness of
  timing changes.
- Redact secrets and preserve the exact target/kernel version with the evidence.

## Related topics

- [Stage 2: Processes And Program Lifetime](index.md)
- [Process Model And Identifiers](process-model-and-identifiers.md)
- [System-Call Contracts And Errors](../system-calls-files-and-file-descriptors/system-call-contracts-and-errors.md)
- [Memory Pressure, OOM, And Realtime](../process-memory-and-mapping/memory-pressure-oom-and-realtime.md)

## References

- [`proc(5)`](https://man7.org/linux/man-pages/man5/proc.5.html)
- [Linux kernel `/proc` documentation](https://www.kernel.org/doc/html/latest/filesystems/proc.html)
- [`ps(1)`](https://man7.org/linux/man-pages/man1/ps.1.html)
- [`strace(1)`](https://strace.io/)

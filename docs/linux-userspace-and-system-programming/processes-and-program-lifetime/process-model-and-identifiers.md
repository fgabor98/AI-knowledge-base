---
status: draft
reviewed: false
domain: linux-userspace
difficulty: beginner
last_reviewed: null
---

# Process Model And Identifiers

## What problem does this solve?

Operational tools often say “the process,” but a Linux program can have multiple
threads, identifiers in nested PID namespaces, a parent that has already exited, and
children in separate process groups. Precise identity is necessary for signals,
waiting, permissions, `/proc` inspection, and supervisor integration.

## Program, process, task, and thread

- A **program** is an executable file and its metadata.
- A **process** is a running program image with an address space and process-wide
  resources.
- A **thread** is an execution stream sharing the process address space and many
  resources while having its own registers, stack, scheduling state, and signal mask.
- The kernel often calls a schedulable execution entity a **task**. A thread group is
  the kernel’s process-like grouping of threads.

One process may therefore have one PID visible as its thread-group ID and several
thread IDs visible under `/proc/<pid>/task/`. Do not assume a thread ID is a process
ID accepted by every interface.

## Identifier relationships

```text
session
  +-- process group (shell foreground job)
        +-- process (leader)
              +-- threads
              +-- children / grandchildren
```

Important identifiers:

| Identifier | Meaning |
| --- | --- |
| PID | Process ID as visible in the caller’s PID namespace |
| PPID | Parent process ID in the caller’s PID namespace |
| TGID | Thread-group ID; usually the process’s PID |
| TID | Individual thread ID |
| PGID | Process-group ID used for job-control signal delivery |
| SID | Session ID, grouping process groups and controlling terminal state |
| UID/GID | Credentials used by permission and ownership checks |
| Namespace inode | Identity of a namespace instance, visible through `/proc/*/ns` |

PIDs are small integers allocated and eventually reused. A stored PID is not a
permanent handle. If an operation must refer to a specific process safely across
time, use a supervisor-owned relationship, a pidfd where available, or verify a
start identity before acting.

## Process resources

Process-wide or inherited state includes:

- virtual address space and mappings;
- file descriptors and their open-file descriptions;
- current directory, root directory, umask, and namespace membership;
- credentials, capabilities, supplementary groups, and resource limits;
- signal dispositions, pending process-directed signals, and timers;
- environment and arguments;
- process group/session membership and controlling terminal;
- cgroup membership, scheduling policy, affinity, and accounting state.

Threads additionally have their own stack, registers, thread-local storage, signal
mask, pending thread-directed signals, and scheduling attributes. A “global variable”
is process-shared memory, not necessarily protected shared state.

## Parent and child relationships

The parent normally waits for a child and receives its termination result. If the
parent exits first, the child is reparented to an appropriate subreaper or PID 1.
Reparenting does not make the old parent’s application-level protocol disappear; a
child holding a pipe, lock, or device FD may still affect the system.

Use `getpid()` and `getppid()` for diagnostics, but log a start timestamp, executable
identity, and supervisor instance as well. A PID alone is insufficient evidence when
restart is fast.

## Inspecting identity

```sh
ps -e -o pid,ppid,pgid,sid,lstart,user,stat,cmd
cat /proc/$$/status
ls -l /proc/$$/ns
ls /proc/$$/task
readlink /proc/$$/cwd
umask
```

`/proc/<pid>/status` includes IDs, state, memory, groups, capabilities, and signal
masks. `/proc/<pid>/task` exposes threads. Information can be hidden by permissions,
`hidepid`, namespaces, or a service sandbox.

## Process identity and security

An executable’s mode bits are only one part of identity. At service start, capture or
verify UID/GIDs, capabilities, `NoNewPrivileges`-style policy, namespace membership,
working directory, root, environment, and inherited FDs. A program that is safe as an
unprivileged user can become dangerous when run with ambient privileges or a writable
device node.

## Common mistakes

- Treating a PID as a stable object handle.
- Calling every task in `/proc/<pid>/task` a separate process.
- Sending a signal to a PID when the intended target is a process group.
- Assuming PPID remains constant or that PID 1 is always the original boot init.
- Ignoring inherited descriptors, `cwd`, umask, environment, and namespaces.
- Diagnosing a restarted service using stale `/proc/<old-pid>` evidence.

## Debugging checklist

- Record PID, start time, PPID, PGID, SID, TIDs, executable, and supervisor instance.
- Inspect `/proc/<pid>/status`, `task`, `fd`, `cwd`, `root`, and `ns`.
- Compare the process view with the host view and service namespace.
- Check credentials, capabilities, limits, affinity, scheduling, and cgroup.
- Determine whether the issue is process-wide or isolated to one thread.
- Use pidfds or a verified start identity when acting on a long-lived process.

## Related topics

- [Stage 2: Processes And Program Lifetime](index.md)
- [fork, exec, And posix_spawn](fork-exec-and-spawn.md)
- [Sessions, Process Groups, And Job Control](sessions-process-groups-and-job-control.md)
- [Process Observation And Control](proc-process-observation-and-control.md)

## References

- [`getpid(2)`](https://man7.org/linux/man-pages/man2/getpid.2.html)
- [`proc(5)`](https://man7.org/linux/man-pages/man5/proc.5.html)
- [`pid_namespaces(7)`](https://man7.org/linux/man-pages/man7/pid_namespaces.7.html)
- [`pidfd_open(2)`](https://man7.org/linux/man-pages/man2/pidfd_open.2.html)

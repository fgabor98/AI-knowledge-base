---
status: draft
reviewed: false
domain: linux-userspace
difficulty: beginner
last_reviewed: null
---

# Stage 2: Processes And Program Lifetime

A process is more than an executable file. It is a running identity with an address
space, threads, credentials, signal state, namespaces, resource limits, open file
descriptors, and a relationship to a supervisor. This chapter explains how that
identity is created, replaced, terminated, observed, and controlled.

## The lifecycle model

```text
program file --exec--> process image --run--> exit/stop
                              |                 |
                         fork/spawn          wait/reap
                              |                 |
                         child process       supervisor records result
```

`fork` creates a new process from the caller’s current process state. `exec` replaces
the calling process’s program image while preserving selected process attributes.
Exit ends the process, but the parent must wait to collect the termination result and
release the kernel’s zombie record. A supervisor may restart the service, so a new
process with a new PID can represent the same product component.

## Learning materials

1. [Process Model And Identifiers](process-model-and-identifiers.md)
2. [fork, exec, And posix_spawn](fork-exec-and-spawn.md)
3. [Exit, Waiting, And Zombies](exit-waiting-and-zombies.md)
4. [Sessions, Process Groups, And Job Control](sessions-process-groups-and-job-control.md)
5. [/proc Process Observation And Control](proc-process-observation-and-control.md)

## Process identity is multi-dimensional

| Dimension | Examples | Why it matters |
| --- | --- | --- |
| PID identity | PID, PPID, thread ID, PID namespace | IDs can be reused and differ between namespaces |
| Image | executable, arguments, environment, libraries | `exec` changes the image without necessarily changing PID |
| Resources | address space, FDs, mappings, timers, locks | Inheritance and lifetime determine leaks and shutdown |
| Credentials | real/effective UID/GID, groups, capabilities | Access checks and privilege are process state |
| Scheduling | policy, priority, affinity, limits | A process can be alive but starved or blocked |
| Relationships | parent, process group, session, supervisor | Signals, terminal ownership, and reaping depend on them |
| Isolation | mount, user, network, IPC, cgroup namespaces | The same path, PID, or resource can have different views |
| Observation | `/proc`, logs, exit status, core dump | Diagnosis needs identity captured before PID reuse |

## Service lifecycle discipline

For an embedded service, define:

- who starts it and what readiness means;
- which PID is supervised and which children it may create;
- how SIGTERM becomes application shutdown;
- the shutdown deadline and escalation behavior;
- which exit statuses trigger restart, failover, or operator attention;
- how crash evidence is retained before restart;
- which resources must be closed, joined, reaped, or rolled back.

Avoid “daemon folklore” that detaches from a supervisor and hides failures. A modern
service normally remains in the foreground and lets PID 1 or a service manager own
restart, logging, cgroups, and dependency ordering.

## Stage lab

Build the process probe and observe it:

```sh
cc -std=c11 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -g \
    examples/c/linux-userspace-process-lifecycle.c \
    -o /tmp/process-lifecycle
/tmp/process-lifecycle
```

Then inspect a long-lived process:

```sh
sleep 30 & pid=$!
cat "/proc/$pid/status"
readlink "/proc/$pid/exe"
ls -l "/proc/$pid/fd"
wait "$pid"
```

## Completion criteria

You can complete this stage when you can:

- distinguish a program, process, thread, process group, session, and supervisor;
- explain what `fork`, `exec`, and `posix_spawn` preserve or replace;
- reap every child and decode exit, signal, core-dump, and stop information;
- explain zombies, orphan reparenting, PID 1, and subreapers;
- reason about terminal foreground groups and signal delivery;
- diagnose whether a process crashed, exited, hung, blocked, was killed, or restarted;
- capture `/proc` and supervisor evidence before a PID is reused.

## Related topics

- [Stage 1: Linux Runtime, Filesystem, And Rootfs](../linux-runtime-filesystem-and-rootfs/index.md)
- [Stage 3: System Calls, Files, And File Descriptors](../system-calls-files-and-file-descriptors/index.md)
- [Service Lifecycle, Readiness, And Restart](../services-init-and-systemd/service-lifecycle-readiness-and-restart.md)
- [PID 1, Init, And Early Userspace](../services-init-and-systemd/pid1-init-and-early-userspace.md)

## References

- [`process(7)`](https://man7.org/linux/man-pages/man7/process.7.html)
- [`proc(5)`](https://man7.org/linux/man-pages/man5/proc.5.html)
- [`credentials(7)`](https://man7.org/linux/man-pages/man7/credentials.7.html)
- [`systemd.service(5)`](https://www.freedesktop.org/software/systemd/man/latest/systemd.service.html)

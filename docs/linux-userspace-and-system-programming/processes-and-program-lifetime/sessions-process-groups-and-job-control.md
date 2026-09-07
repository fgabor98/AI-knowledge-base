---
status: draft
reviewed: false
domain: linux-userspace
difficulty: intermediate
last_reviewed: null
---

# Sessions, Process Groups, And Job Control

## What problem does this solve?

Signals and terminal behavior are often sent to a group, not one process. A shell
pipeline, a service with children, and a foreground interactive job need different
ownership boundaries. Understanding sessions and process groups prevents orphaned
helpers, terminal stops, and shutdown signals that reach the wrong process.

## The hierarchy

```text
session (SID)
  +-- controlling terminal
  +-- foreground process group (one PGID)
  +-- background process group
  +-- other process groups
```

A session leader can acquire a controlling terminal. Each process belongs to one
process group; a session contains one or more groups. The terminal tracks a foreground
group and sends terminal-generated signals such as `SIGINT` and `SIGTSTP` to it.

`setsid()` creates a new session when the caller is not already a process-group
leader. `setpgid()` places a process into a group, commonly before `exec` in a
pipeline. `tcsetpgrp()` gives a terminal’s foreground ownership to a group and is
normally controlled by the shell.

## Shell pipelines

For `producer | consumer`, the shell typically creates a process group for the
pipeline, connects FDs, and moves the group into the foreground. Ctrl-C then targets
the group. A program that launches children should use a deliberate group policy:

- keep all helper processes in the service’s group and terminate the group;
- create a private group for one job and track its lifetime;
- use a supervisor/cgroup when descendants may escape ordinary parent ownership.

Do not use `kill(-pid, signal)` without understanding that a negative PID targets a
process group. The intended group ID must be known and validated.

## Background and terminal signals

Background processes that read from the controlling terminal can receive `SIGTTIN`;
those that write can receive `SIGTTOU` depending on terminal settings. A service
should not depend on an interactive terminal. Redirect standard streams to deliberate
logs or `/dev/null`, set a known working directory, and let the service manager own
the session policy.

## Daemons and supervisors

The traditional double-fork daemon pattern creates a new session and detaches from a
terminal. It can be appropriate for a standalone legacy daemon, but it often harms a
modern embedded product by hiding the real PID and confusing restart, logging, and
child ownership. A supervised service should normally stay in the foreground and
declare its standard streams, process group, and shutdown behavior to the supervisor.

If detachment is required, document who becomes the parent, who reaps descendants,
where logs go, and how the supervisor learns readiness and death. “Fork into the
background” is not a supervision protocol.

## Signal targeting

| Call | Target |
| --- | --- |
| `kill(pid, sig)` with positive PID | One process ID in the caller’s PID namespace |
| `kill(0, sig)` | Caller’s process group |
| `kill(-pgid, sig)` | The specified process group |
| `kill(-1, sig)` | Broad set of permitted processes; dangerous without strict policy |
| `pthread_kill` | One thread within the calling process |

Signal delivery still depends on permissions, blocked masks, pending state, and
default/installed dispositions. A successful `kill` means delivery was accepted, not
that the target handled the signal or completed shutdown.

## Minimal inspection

```sh
ps -e -o pid,ppid,pgid,sid,tpgid,stat,tty,cmd
ps -o pid,ppid,pgid,sid,tpgid,stat,cmd -p "$$"
stty -a
```

For a service, inspect the actual PID and descendants. For an interactive pipeline,
compare PGID and foreground TPGID while it runs.

## Common mistakes

- Sending a group signal to a PID or a PID signal to a whole job unintentionally.
- Calling `setsid` from a process-group leader and ignoring its failure.
- Assuming a child’s PID identifies all of its descendants.
- Daemonizing under a supervisor and losing lifecycle ownership.
- Leaving terminal FDs or standard streams connected to an interactive shell.
- Treating successful signal delivery as successful application shutdown.

## Debugging checklist

- Record PID, PPID, PGID, SID, TPGID, controlling TTY, and descendants.
- Check signal masks, dispositions, and pending signals in `/proc/<pid>/status`.
- Reproduce Ctrl-C, Ctrl-Z, terminal close, supervisor stop, and parent death.
- Verify the shutdown signal reaches every intended process and no unrelated process.
- Check whether a detached child remains after the parent and who reaps it.

## Related topics

- [Stage 2: Processes And Program Lifetime](index.md)
- [Process Model And Identifiers](process-model-and-identifiers.md)
- [Exit, Waiting, And Zombies](exit-waiting-and-zombies.md)
- [Signal Model And sigaction](../time-clocks-and-signals/signal-model-and-sigaction.md)

## References

- [`process groups`](https://man7.org/linux/man-pages/man7/credentials.7.html)
- [`setpgid(2)`](https://man7.org/linux/man-pages/man2/setpgid.2.html)
- [`setsid(2)`](https://man7.org/linux/man-pages/man2/setsid.2.html)
- [`tcsetpgrp(3)`](https://man7.org/linux/man-pages/man3/tcsetpgrp.3.html)

---
status: draft
reviewed: false
domain: linux-userspace
difficulty: intermediate
last_reviewed: null
---

# Descriptor Inheritance And Redirection

## What problem does this solve?

A child process inherits open descriptors unless they are close-on-exec. One leaked
write end can prevent a reader from seeing EOF; one leaked secret FD can expose data
to a helper; one inherited device FD can keep hardware active after the parent thinks
it shut down. Redirection is therefore resource policy, not just shell syntax.

## Shell redirection as descriptor operations

```sh
command >out 2>err
command >combined 2>&1
```

The shell opens files, duplicates descriptors, and then executes the command. The
order matters: `2>&1` duplicates the current stdout, while `>combined 2>&1` sends
both streams to the file. Programmatic launchers must reproduce the intended graph
with `open`, `dup2`/`dup3`, and closes.

## Pipeline construction

For `producer | consumer`:

```text
pipe read end --> consumer stdin
pipe write end --> producer stdout
parent closes both ends
producer closes read end and execs
consumer closes write end and execs
```

Every process must close ends it does not use. If any process retains a write end,
the consumer may wait forever for EOF. The parent must also wait for both children and
define which exit status represents pipeline failure.

## Safe launch sequence

```text
create all pipes with pipe2(O_CLOEXEC)
fork child
  dup intended descriptors onto 0/1/2
  close all original and unused pipe descriptors
  exec absolute helper path
  _exit on setup/exec failure
parent closes child-side descriptors
parent reads an exec-error channel if needed
parent supervises/reaps the child
```

The error channel is a pipe whose write end is close-on-exec. The child writes a
small error record on setup failure; successful `exec` closes it automatically and
the parent observes EOF. This distinguishes “child exited 127” from “exec failed with
`ENOENT`.”

## Activation and intentional inheritance

Some supervisors intentionally pass listening sockets or other activation FDs. The
contract must specify descriptor numbers, `FD_CLOEXEC`, object type, ownership, and
what happens on restart. Do not accept arbitrary inherited descriptors from the
environment; enumerate and validate them.

## Security boundary

Inherited descriptors can bypass pathname permissions and expose already-open files,
device nodes, sockets, and directories. Set close-on-exec by default, use an explicit
allow-list for helpers, and inspect `/proc/<pid>/fd` in tests. A descriptor leak can
also keep a mount busy, keep a pipe alive, prevent a socket from closing, or retain a
deleted file’s storage.

## Standard streams

Services should define what happens to stdin, stdout, and stderr. Redirecting them to
`/dev/null`, a supervisor journal, or an application log is a deployment decision.
Do not depend on a terminal, shell expansion, current directory, or interactive
environment. Flush or use an appropriate logging design when a child exits.

## Common mistakes

- Creating descriptors without `O_CLOEXEC` in a multithreaded launcher.
- Closing only the parent’s copy of a pipe and forgetting child copies.
- Assuming `2>&1` means “merge later” rather than duplicate the current FD.
- Using `system()` where explicit argv/FD/environment control is required.
- Passing all inherited descriptors to a less-trusted helper.
- Treating exit 127 as a precise `exec` diagnosis.
- Closing a descriptor in a parent before a child has duplicated it.

## Debugging checklist

- Draw the descriptor graph for parent, each child, and post-exec program.
- Inspect `/proc/<pid>/fd` before and after exec.
- Test EOF, SIGPIPE, exec failure, child setup failure, and helper crash.
- Verify standard streams, activation FDs, and close-on-exec flags.
- Check mounts and deleted files held open by descendants.
- Reap every launched child and record its decoded status.

## Related topics

- [Stage 3: System Calls, Files, And File Descriptors](index.md)
- [File Descriptors And Open-File Descriptions](file-descriptors-and-open-file-descriptions.md)
- [fork, exec, And posix_spawn](../processes-and-program-lifetime/fork-exec-and-spawn.md)
- [Pipes, FIFOs, And Backpressure](pipes-fifos-and-backpressure.md)

## References

- [`pipe2(2)`](https://man7.org/linux/man-pages/man2/pipe.2.html)
- [`dup2(2)`](https://man7.org/linux/man-pages/man2/dup.2.html)
- [`exec(3)`](https://man7.org/linux/man-pages/man3/exec.3.html)
- [`systemd.socket(5)`](https://www.freedesktop.org/software/systemd/man/latest/systemd.socket.html)

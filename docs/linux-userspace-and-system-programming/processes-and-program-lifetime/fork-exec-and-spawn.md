---
status: draft
reviewed: false
domain: linux-userspace
difficulty: intermediate
last_reviewed: null
---

# fork, exec, And posix_spawn

## What problem does this solve?

Launching a helper looks simple until the child inherits descriptors, locks, signal
state, namespaces, and partially initialized memory. A robust launch design chooses
the smallest creation mechanism, closes or transfers resources deliberately, and
reports failures from both child setup and `exec`.

## `fork` creates; `exec` replaces

```text
parent image --fork--> parent image + child image (copy-on-write)
child image  --exec--> new program image, same process identity
```

After `fork`, parent and child have separate virtual memory mappings, initially
sharing physical pages copy-on-write. A write in either process can create a private
page. File descriptors in both processes refer to the same open-file descriptions,
so offsets and status flags may be shared. The child inherits credentials, signal
dispositions, current directory, umask, namespaces, and many other attributes.

`execve` replaces code, data, heap, stack, and mapped libraries. It retains the PID,
open descriptors not marked close-on-exec, credentials subject to exec rules, current
directory, umask, and selected process attributes. The new program begins at its
startup path; it does not return to the old code on success.

## The fork/exec child rule

In a multithreaded process, after `fork` only the calling thread exists in the child.
The child may be holding locks that belonged to vanished threads, and many libc
operations are unsafe before `exec`. Restrict the child to async-signal-safe setup,
`dup2`/`dup3`, `close`, `execve`, and `_exit`, or use `posix_spawn`.

Never call `exit()` in a failed child path after `fork`: it can flush inherited stdio
buffers and run inherited `atexit` handlers. Use `_exit(status)`.

## A controlled launch sequence

```text
parent creates pipes with O_CLOEXEC
parent forks
child duplicates intended stdin/out/err
child closes both sides of all private pipes
child optionally reports setup errors through a close-on-exec error pipe
child execve()s target
parent closes child-side descriptors
parent reads setup result and waits or supervises child
```

An error pipe solves the ambiguity where the child exits before `exec`: the parent can
receive `errno` from child setup. EOF means no error record was received: successful
`exec` closes the descriptor, but abrupt child death also does. Combine the channel
with wait status, and require an application readiness handshake when startup success
must be known. EOF alone cannot prove that the new program ran.

## `posix_spawn`

`posix_spawn` combines process creation and program replacement with a restricted set
of file actions and attributes. It can be preferable for a simple helper launch,
especially in a multithreaded process or on memory-constrained targets. Use file
actions to open, close, or duplicate descriptors and attributes for signal masks,
default dispositions, scheduling, or process groups as supported by the target.

It is not a universal replacement: complex child setup, namespaces, arbitrary
pre-exec logic, or detailed error channels may require another design. Check the
target libc implementation and document the supported options.

## Environment and path resolution

`execve` takes an explicit path and environment. `execvp` searches `PATH`, which is
convenient interactively but risky for privileged or supervised services. Prefer an
absolute installed path and a controlled environment. Validate arguments as data;
never combine them into a shell command unless shell interpretation is explicitly
required and safely encoded.

## Minimal example pattern

```c
pid_t child = fork();
if (child == -1) {
    /* parent: save errno and report */
}
if (child == 0) {
    /* Precondition for this abbreviated example: output_fd > STDERR_FILENO. */
    if (dup2(output_fd, STDOUT_FILENO) == -1) {
        _exit(127);
    }
    close(output_fd);
    execl("/usr/bin/helper", "helper", "--once", (char *)NULL);
    _exit(127);
}
/* parent owns child and must close its copy and wait/supervise */
```

The example omits a complete error pipe and signal policy for clarity; production
code should distinguish fork failure, child setup failure, `exec` failure, and helper
exit status.

## Descriptor inheritance

Use `O_CLOEXEC`, `pipe2(O_CLOEXEC)`, `dup3(..., O_CLOEXEC)`, and `fcntl` deliberately.
Setting close-on-exec after a separate `open` can race with another thread calling
`fork`/`exec`. The atomic creation flag closes that window. The descriptor used as
the child’s standard stream must have close-on-exec cleared or be duplicated to the
intended number with the correct semantics.

## Common mistakes

- Assuming `fork` copies kernel resources independently.
- Calling malloc, stdio, locks, or `exit` in a multithreaded post-fork child.
- Forgetting to close unused pipe ends, preventing EOF.
- Losing the child’s `exec` error and reporting only a generic exit status.
- Using `system()` for a fixed helper and inheriting shell parsing and environment risk.
- Setting `FD_CLOEXEC` non-atomically in a multithreaded launcher.
- Failing to wait for children and creating zombies.

## Debugging checklist

- Log parent PID, child PID, absolute executable, arguments, and sanitized environment.
- Inspect child FDs before `exec` and helper FDs after launch.
- Test missing executable, wrong interpreter, permission denied, invalid argument,
  signal termination, and timeout.
- Check close-on-exec with `/proc/<pid>/fd` and an intentionally launched helper.
- Verify parent closes every unused pipe end and waits for every child.
- Prefer `posix_spawn` when setup is simple and the target libc supports required
  actions.

## Related topics

- [Stage 2: Processes And Program Lifetime](index.md)
- [Exit, Waiting, And Zombies](exit-waiting-and-zombies.md)
- [Descriptor Inheritance And Redirection](../system-calls-files-and-file-descriptors/descriptor-inheritance-and-redirection.md)
- [Exec And Dynamic Linking](../linux-runtime-filesystem-and-rootfs/elf-executables-and-dynamic-linking.md)

## References

- [`fork(2)`](https://man7.org/linux/man-pages/man2/fork.2.html)
- [`execve(2)`](https://man7.org/linux/man-pages/man2/execve.2.html)
- [`posix_spawn(3)`](https://man7.org/linux/man-pages/man3/posix_spawn.3.html)
- [`_exit(2)`](https://man7.org/linux/man-pages/man2/_exit.2.html)

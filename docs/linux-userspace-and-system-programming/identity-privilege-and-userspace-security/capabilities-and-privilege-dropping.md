---
status: draft
reviewed: false
domain: linux-userspace
difficulty: advanced
last_reviewed: null
---

# Capabilities And Privilege Dropping

Capabilities split many traditional root powers into named privileges such as `CAP_NET_BIND_SERVICE`, `CAP_CHOWN`, and `CAP_SYS_ADMIN`. The last is intentionally broad and should not be used as a generic escape hatch.

## Capability sets

A thread has permitted, effective, inheritable, ambient, and bounding sets. The effective set is used for many checks; permitted is the ceiling for effective capabilities; inheritable participates in transitions; ambient capabilities can survive an `execve()` of a non-privileged executable when explicitly arranged; the bounding set limits what can be gained. File capabilities add another input during exec.

Inspect a process and an executable with `capsh`, `getcap`, and `/proc/PID/status`, but remember that a snapshot can change between inspection and use.

## A safe transition

Privilege reduction should happen as early as practical and be tested as a state transition:

1. Open only the resources that genuinely require privilege.
2. Validate configuration and establish the service identity.
3. Set `PR_SET_NO_NEW_PRIVS` when compatible with the design.
4. Reduce the bounding set while `CAP_SETPCAP` is still effective; keep any
   authority needed to finish the transition until its last use.
5. Clear ambient capabilities and set the intended supplementary groups while
   `CAP_SETGID` is available, then set real/effective/saved GIDs.
6. Set real/effective/saved UIDs, then explicitly clear remaining capability
   sets for a design that retains no privilege. Check every call and stop on failure.
7. Close or mark all unrelated inherited file descriptors.
8. Verify the resulting state, then exercise privileged and unprivileged paths.

Dropping the UID while retaining a powerful capability is not a drop. Conversely, dropping a capability before a required setup operation can make startup fail. Treat the sequence as part of the service contract, not as incidental cleanup.

## Exec and supervisor interactions

An `execve()` can transform capabilities depending on file mode, file capabilities, securebits, `no_new_privs`, and the caller's sets. A service manager may apply capability bounding and ambient settings before exec, so document both the unit configuration and the program's own transition logic.

Avoid passing secrets or privileged handles through the environment. If a privileged helper is needed, keep its IPC protocol narrow, authenticate the peer, validate every request, and return structured errors rather than exposing a general command runner.

## Verification

Capture the intended state in tests:

```sh
grep -E '^(Uid|Gid|Groups|Cap[A-Za-z]*|NoNewPrivs):' /proc/$PID/status
getpcaps "$PID" 2>/dev/null || true
```

Test file access, raw device access, bind-to-port behavior, signal permissions, and `execve()` transitions independently. A successful privileged operation is evidence only for that operation; it does not prove the whole sandbox is correct.

## Why the transition order matters

Dropping `CAP_SETPCAP` before reducing the bounding set can prevent that
reduction; dropping UID/GID authority before clearing supplementary groups can
leave unintended access. The sequence above assumes a single-threaded launcher
with the required initial authority. Every failed setup call terminates
startup before untrusted requests are admitted.

Changing only the effective UID can retain a saved privileged UID. For a
permanent drop, set all real/effective/saved IDs deliberately and verify them.
Do not mix raw credential syscalls and libc's pthread-aware wrappers: Linux
kernel credentials are per-thread, while libc coordinates changes to satisfy
the process-wide POSIX model.

A service that intentionally keeps capabilities across a UID change needs a
different, reviewed securebits/keepcaps and capability-set sequence. The simple
“drop everything” recipe cannot be modified by inserting one keepcaps call.
Prefer having the supervisor launch directly with the final identity when
privileged setup can be performed elsewhere.

## Bounding is not revocation

Removing a bit from the bounding set does not immediately remove that
capability from the current effective set. `no_new_privs` prevents selected
exec-time privilege gains; it neither clears existing capabilities nor closes
privileged descriptors. Ambient capabilities also have permitted/inheritable
constraints. Verify each dimension separately.
See [capabilities(7)](https://man7.org/linux/man-pages/man7/capabilities.7.html).

Use a sacrificial child to test that a permanent drop cannot regain the
previous privileged UID. Never perform a successful regain experiment in the
long-lived service and continue as though the boundary remained intact.
Record all IDs, groups, capability sets, securebits policy, and inherited FDs.

## Related topics

- [Stage 12 overview](index.md)
- [Service sandboxing](../services-init-and-systemd/service-sandboxing-and-resource-controls.md)

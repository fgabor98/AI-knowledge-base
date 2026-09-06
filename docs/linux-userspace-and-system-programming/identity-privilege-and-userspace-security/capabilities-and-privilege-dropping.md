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
4. Clear ambient and unnecessary effective/permitted/inheritable capabilities.
5. Set the capability bounding set to the smallest required set.
6. Change UID/GID and supplementary groups in the correct order.
7. Close or mark all unrelated inherited file descriptors.
8. Verify the resulting state, then exercise privileged and unprivileged paths.

Dropping the UID while retaining a powerful capability is not a drop. Conversely, dropping a capability before a required setup operation can make startup fail. Treat the sequence as part of the service contract, not as incidental cleanup.

## Exec and supervisor interactions

An `execve()` can transform capabilities depending on file mode, file capabilities, securebits, `no_new_privs`, and the caller's sets. A service manager may apply capability bounding and ambient settings before exec, so document both the unit configuration and the program's own transition logic.

Avoid passing secrets or privileged handles through the environment. If a privileged helper is needed, keep its IPC protocol narrow, authenticate the peer, validate every request, and return structured errors rather than exposing a general command runner.

## Verification

Capture the intended state in tests:

```sh
grep -E '^(Uid|Gid|Groups|Cap|NoNewPrivs):' /proc/$PID/status
getpcaps "$PID" 2>/dev/null || true
```

Test file access, raw device access, bind-to-port behavior, signal permissions, and `execve()` transitions independently. A successful privileged operation is evidence only for that operation; it does not prove the whole sandbox is correct.

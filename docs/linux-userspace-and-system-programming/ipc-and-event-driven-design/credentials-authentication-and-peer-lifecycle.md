---
status: draft
reviewed: false
domain: linux-userspace
difficulty: advanced
last_reviewed: null
---

# Credentials, Authentication, And Peer Lifecycle

## What problem does this solve?

Local IPC is not automatically trusted. A socket path can be replaced, a process can
run with unexpected credentials, and a peer can disappear after authentication. A
service must authorize the actual peer and keep that identity tied to the connection
and request lifecycle.

## Peer identity

For Unix-domain sockets, Linux can expose peer credentials such as PID, UID, and GID
through `SO_PEERCRED`; credential passing and namespace behavior need target-specific
review. Supplement this with filesystem ownership/mode, service-manager policy,
capabilities, SELinux/AppArmor labels where used, and protocol authentication when
the threat model requires it.

Credentials answer who the kernel associates with the endpoint. Authorization answers
what that identity may do. Do not authorize solely because a client reached a local
pathname.

## Connection lifecycle

```text
accepted -> authenticate -> negotiate -> active
     |          |             |
   close     reject        timeout/reset
                              |
                         reconnect/new generation
```

Capture credentials and protocol generation at accept/authentication. `SO_PEERCRED`
reports credentials captured at connection establishment (or socketpair creation),
not a live identity lookup on each request. Repeating it does not detect later UID
changes or descriptor delegation. If current message credentials are required,
design around `SO_PASSCRED`/`SCM_CREDENTIALS` and the socket type's documented
semantics. Apply request authorization even after connection authentication.

## Peer death and unknown outcomes

EOF, reset, timeout, and process disappearance end a connection but do not reveal
whether its last side effect completed. Mark outstanding requests unknown, close
dependent FDs, and reconcile through a state query or generation protocol before
retrying.

## FD passing

An FD received with `SCM_RIGHTS` is an authority-bearing capability. Validate its type,
access mode, owner context, and expected protocol before using it. Set close-on-exec,
limit the number accepted, and close on every error path. Do not pass a privileged FD
to a less-trusted process as a shortcut around authorization.

## Socket endpoint protection

Create service directories with controlled ownership, bind sockets with deliberate
mode, and clean stale paths only after proving they belong to the service. Abstract
socket names avoid filesystem cleanup but are visible within a network namespace and
still require credential checks.

## Common mistakes

- Treating local transport or a pathname as authentication.
- Reading peer credentials once and forgetting request authorization.
- Retrying an unknown side effect after disconnect.
- Accepting unlimited connections, FDs, or in-flight requests.
- Passing privileged descriptors without validating the recipient.
- Deleting a socket path that belongs to another instance.

## Debugging checklist

- Record endpoint, PID/UID/GID, namespace, protocol version, and authorization result.
- Inspect socket owner/mode and `SO_PEERCRED` evidence.
- Test unauthorized users, changed groups, stale paths, peer crash, reset, and retry.
- Test FD passing, descriptor leaks, exec inheritance, and connection limits.
- Verify unknown-outcome reconciliation before side-effect retry.

## Related topics

- [Stage 7: IPC And Event-Driven Design](index.md)
- [Pipes, socketpairs, And Unix Sockets](pipes-socketpairs-and-unix-sockets.md)
- [IPC Protocols And Versioning](ipc-protocols-and-versioning.md)
- [Linux Credentials, Permissions, And ACLs](../identity-privilege-and-userspace-security/linux-credentials-permissions-and-acls.md)

## References

- [`unix(7)`](https://man7.org/linux/man-pages/man7/unix.7.html)
- [`socket(7)`](https://man7.org/linux/man-pages/man7/socket.7.html)
- [`credentials(7)`](https://man7.org/linux/man-pages/man7/credentials.7.html)
- [`SCM_RIGHTS`](https://man7.org/linux/man-pages/man7/unix.7.html)

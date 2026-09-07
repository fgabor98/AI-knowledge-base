---
status: draft
reviewed: false
domain: linux-userspace
difficulty: advanced
last_reviewed: null
---

# Utility, Daemon, And Process Boundaries

## Choose the lifetime

A **utility** performs a bounded operation and reports a result. A **daemon** owns ongoing state, accepts requests, and survives individual clients. A **supervisor** owns lifecycle, restart policy, readiness, and resource containment. Do not turn a utility into a daemon merely to avoid designing a caller interface.

## When a process boundary helps

Separate processes when independent restart, privilege reduction, failure containment, incompatible dependencies, or a stable service contract is valuable. Keep components together when calls are frequent, state sharing is central, and the failure domain is intentionally shared. Process boundaries add serialization, IPC failure, startup ordering, observability, and deployment cost.

Define ownership of descriptors, threads, memory, persistent state, and device handles. Use narrow IPC with peer authentication, request IDs, bounded messages, deadlines, cancellation, and explicit version negotiation. Never expose a general shell or unrestricted file-descriptor broker as a convenience API.

The supervisor should know whether a process is alive, ready, degraded, or wedged. Readiness and health are different signals; restart policy must account for crash loops, dependency failures, intentional shutdown, and update rollback.

## Work through a device-service partition

Suppose several clients need one serial device whose commands must be serialized.
A daemon can own the port and protocol generation while short-lived utilities
connect through a local control socket. This centralizes device arbitration,
permission checks and recovery. The service manager owns daemon restart and
resource policy; each utility owns its request deadline and exit status.

Splitting every parser and calculation into another process would add protocol
and deployment costs without necessarily improving isolation. Conversely, a
third-party decoder processing untrusted data may justify a restricted worker
process that can be terminated independently.

Threads share the address space and generally the process's fatal-fault domain.
A thread segfault ordinarily terminates the process; it is not an independently
restartable worker failure. A thread stuck in an uncancellable device call
cannot safely be replaced while it still has access to shared state. A helper
process can make termination containment clearer, but externally submitted
device commands still need reconciliation.

## Own restart and inheritance

The service manager starts the foreground daemon. The daemon passes only explicit
descriptors to helpers, owns their wait/reaping obligations, and scopes their
shutdown. Clients reconnect to a new daemon generation and cannot assume old
in-flight commands failed just because the connection ended.

If the service owns exclusive hardware, two simultaneous instances must have a
defined rejection or supervisor-enforced singleton policy. A PID file alone
does not establish this ownership. Review process boundaries together with
descriptor and device ownership, not only as boxes in an architecture drawing.

## Related topics

- [Stage 17 overview](index.md)
- [IPC protocols and versioning](../ipc-and-event-driven-design/ipc-protocols-and-versioning.md)
- [Lifecycle and hardware tests](../testing-and-verification/lifecycle-update-and-hardware-in-the-loop-tests.md)

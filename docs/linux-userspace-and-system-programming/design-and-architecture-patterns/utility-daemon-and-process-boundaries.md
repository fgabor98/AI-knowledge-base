# Utility, Daemon, And Process Boundaries

## Choose the lifetime

A **utility** performs a bounded operation and reports a result. A **daemon** owns ongoing state, accepts requests, and survives individual clients. A **supervisor** owns lifecycle, restart policy, readiness, and resource containment. Do not turn a utility into a daemon merely to avoid designing a caller interface.

## When a process boundary helps

Separate processes when independent restart, privilege reduction, failure containment, incompatible dependencies, or a stable service contract is valuable. Keep components together when calls are frequent, state sharing is central, and the failure domain is intentionally shared. Process boundaries add serialization, IPC failure, startup ordering, observability, and deployment cost.

Define ownership of descriptors, threads, memory, persistent state, and device handles. Use narrow IPC with peer authentication, request IDs, bounded messages, deadlines, cancellation, and explicit version negotiation. Never expose a general shell or unrestricted file-descriptor broker as a convenience API.

The supervisor should know whether a process is alive, ready, degraded, or wedged. Readiness and health are different signals; restart policy must account for crash loops, dependency failures, intentional shutdown, and update rollback.

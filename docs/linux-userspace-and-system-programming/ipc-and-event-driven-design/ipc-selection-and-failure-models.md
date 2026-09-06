---
status: draft
reviewed: false
domain: linux-userspace
difficulty: intermediate
last_reviewed: null
---

# IPC Selection And Failure Models

## What problem does this solve?

IPC choices are often made from throughput alone. In a real product, peer identity,
message size, latency, restart behavior, queue bounds, observability, and security
matter just as much. The right mechanism makes failure behavior easier to prove.

## Selection questions

Ask in order:

1. Are endpoints related, unrelated, or remote in the future?
2. Is the data a byte stream, message, counter, event, or shared bulk buffer?
3. What are maximum message size, rate, burst, and queue depth?
4. Is delivery reliable, at-most-once, retryable, or intentionally lossy?
5. Does the peer need credentials, FD passing, or namespace crossing?
6. What happens when the peer is slow, crashes, upgrades, or disappears?
7. How will the operation be traced and tested without hardware?

## Mechanism tradeoffs

| Mechanism | Strength | Failure/complexity boundary |
| --- | --- | --- |
| Pipe | Simple ordered stream, natural backpressure | Related lifecycle, no peer credentials, byte framing required |
| `socketpair` | Bidirectional related-process channel | Same stream framing and FD ownership obligations |
| Unix stream socket | Local request/response, credentials, reconnect | Stream framing, socket path lifecycle, backlog/peer death |
| Unix datagram socket | Message boundaries and local endpoint | Queue limits, truncation, loss/ordering policy |
| Shared memory | Large payload with few copies | Synchronization, reclamation, stale peers, separate control path |
| POSIX message queue | Message priorities and boundaries | Kernel limits, portability/deployment overhead |
| `eventfd` | Compact wakeup/counter | Not a payload transport; count semantics must be defined |
| D-Bus | Discoverable service bus and typed messages | Runtime footprint, activation, policy, dependency complexity |

Do not use shared memory merely to avoid a small copy. Its synchronization and
recovery proof can cost more than the copy saved.

## Failure model

Define what each endpoint observes:

```text
peer not started     -> connect/open fails or waits by policy
peer overloaded      -> reject/backpressure/drop by message class
peer crashes         -> EOF/error; outstanding requests become unknown
peer restarts        -> new connection/generation; stale replies rejected
transport corrupt    -> framing/checksum error; close or resynchronize
local queue full     -> bounded admission decision
shutdown             -> stop intake, cancel/drain, close, join
```

An unanswered command is not automatically a failed command. If the peer could have
performed a side effect before dying, the result is `unknown`; retries require an
idempotency key or a query that can reconcile state.

## Testing without the peer

Use a fake peer that can delay, fragment, reorder where the mechanism permits, close
mid-frame, send invalid lengths, refuse connections, and crash after acknowledging a
request. Keep the real client protocol unchanged so the fake tests application logic.

## Common mistakes

- Choosing shared memory for small messages without a reclamation design.
- Assuming stream reads preserve messages.
- Treating EOF as proof that the last command was not applied.
- Retrying non-idempotent commands after an ambiguous disconnect.
- Leaving queue limits to kernel defaults.
- Using a socket path as authentication without checking peer credentials.
- Building a protocol that cannot be tested with a fake peer.

## Debugging checklist

- Record mechanism, endpoint, peer identity, generation, request ID, and queue depth.
- Capture connect/accept, read/write, readiness, timeout, and close events.
- Test slow, dead, restarted, malicious, and version-skewed peers.
- Inspect socket/pipe FDs and kernel queue/capacity state.
- Verify overload, cancellation, and unknown-outcome policy.
- Measure copy, latency, memory, and recovery cost before optimizing.

## Related topics

- [Stage 7: IPC And Event-Driven Design](index.md)
- [IPC Protocols And Versioning](ipc-protocols-and-versioning.md)
- [Credentials, Authentication, And Peer Lifecycle](credentials-authentication-and-peer-lifecycle.md)
- [Shared Memory And Zero-Copy IPC](shared-memory-and-zero-copy-ipc.md)

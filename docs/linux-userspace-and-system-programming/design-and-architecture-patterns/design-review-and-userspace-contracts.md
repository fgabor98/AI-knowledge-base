---
status: draft
reviewed: false
domain: linux-userspace
difficulty: advanced
last_reviewed: null
---

# Design Review And Userspace Contracts

## Contract contents

A useful contract specifies data representation and limits, ownership and lifetime, synchronization, ordering, timeout/cancellation behavior, errors and errno mapping, version negotiation, permissions, persistence, and observability. Include what happens when the peer is old, malformed, slow, restarted, or absent.

For a device or IPC operation, answer: who may call it, what state is required, what can change concurrently, which side owns buffers, whether partial completion is possible, how retries avoid duplication, and how an operator correlates the result with logs.

## Review artifacts

Use a small decision record containing requirements, alternatives, selected design, rejected options, assumptions, failure matrix, resource budgets, security boundaries, and test plan. Draw the process/resource ownership and state-transition diagrams when prose hides relationships.

Review the unhappy paths explicitly:

| Area | Question |
| --- | --- |
| Startup | What if configuration, device, dependency, or storage is absent? |
| Runtime | What if input is malformed, slow, duplicated, or too large? |
| Failure | What if a call blocks, returns short, or the peer dies? |
| Recovery | What state is safe after restart, reset, or power loss? |
| Security | Which identity and policy authorize each effect? |
| Operations | What evidence identifies the exact failure and build? |

An architecture is ready when its contracts can be implemented by an independent component, tested with a fake and a real boundary, and operated during degradation. Revisit the record when measurements or target constraints invalidate an assumption.

## Specify one operation completely

For an illustrative `set_output(request_id, channel, value)` API, define
channel identity, units/range, permitted callers, deadline, response meanings
and device generation. Distinguish accepted, applied, rejected and unknown.
State whether “applied” means a driver return, device acknowledgement or
verified physical readback.

A duplicate request ID needs an explicit retention scope. If deduplication
exists only in RAM, a daemon restart loses it. Persisting a request record and
performing a physical action are not automatically one atomic transaction:
a crash between them can still create an ambiguous outcome. Use device-level
idempotency, a state query, or a documented reconciliation procedure. Do not
promise exactly-once physical effects without proving the entire boundary.

## Trace requirements to tests and evidence

| Requirement | Design mechanism | Evidence |
| --- | --- | --- |
| No stale reply updates new session | Generation check and ownership release | Late-reply fixture |
| Bounded memory under overload | Maximum frames, peers and queue capacity | Saturation test and peak accounting |
| Shutdown completes within budget | Stop admission, wake waits, join/terminate policy | Stop during every blocked state |
| Committed settings survive supported interruption | Defined persistence protocol and medium | Crash and target power-cut campaign |
| Unauthorized client cannot actuate | Peer authentication plus per-command authorization | Negative tests under deployed identity |

For each row, state the assumptions: maximum device response time, storage
guarantees, kernel/libc versions, available privileges and hardware revisions.
Assign an owner to every assumption requiring validation.

Keep the decision record short enough to update. Record why a process boundary,
queue bound or retry policy was chosen, what measurements support it, and what
change would trigger reconsideration. A diagram is useful when it makes
ownership or event order unambiguous; it cannot substitute for error semantics.

## Related topics

- [Stage 17 overview](index.md)
- [IPC protocols and versioning](../ipc-and-event-driven-design/ipc-protocols-and-versioning.md)
- [Lifecycle and hardware tests](../testing-and-verification/lifecycle-update-and-hardware-in-the-loop-tests.md)

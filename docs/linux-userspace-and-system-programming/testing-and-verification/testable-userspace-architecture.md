---
status: draft
reviewed: false
domain: linux-userspace
difficulty: advanced
last_reviewed: null
---

# Testable Userspace Architecture

Separate policy from mechanisms and pure decisions from effects. A useful structure is a deterministic core that accepts events and returns commands, surrounded by adapters for clocks, files, sockets, processes, devices, and persistence.

## Dependency boundaries

Inject interfaces rather than calling global state throughout the core. Useful seams include:

- monotonic clock and timer scheduling;
- filesystem and durable-record operations;
- transport read/write and reconnect behavior;
- process supervision and signal delivery;
- device/UAPI access;
- randomness and identity providers.

The production adapter must still be tested against the real boundary. A fake that only returns success can make a design appear reliable while hiding partial writes, `EINTR`, `EPIPE`, delayed readiness, clock jumps, and resource exhaustion.

## Determinism

Pass time explicitly, control event ordering, seed randomness, bound retries, and make cleanup idempotent. Tests should be able to inspect emitted commands and state transitions without sleeping for real time. Use a virtual clock to test deadlines, backoff, and watchdog behavior.

## Contract ownership

Define which layer owns memory, descriptors, threads, cancellation, and retries. Assert ownership at API boundaries. A small number of clear adapters is easier to fake and review than a large abstraction that hides the actual kernel or device contract.

## Drive a state machine without sleeping

Consider a request with a 100 ms deadline. The test supplies `now=0`,
injects the request, and observes a send command plus a deadline. It advances
the fake clock to 99 ms: no timeout. At 100 ms, a timer event must produce
exactly one timeout result. A late reply carrying the retired request
generation must not turn it into success.

Keep clock advancement and timer delivery explicit. A fake clock alone is
insufficient if production code still waits on a real condition variable or
poll timeout. The adapter must translate scheduled deadlines into events,
and tests control when those events are delivered.

## Define adapter semantics before faking them

For a write adapter, record whether buffers are borrowed for the call or
retained asynchronously, what a positive short result means, and whether
failure can follow a partial side effect. A fake returning success after
copying all bytes is not equivalent to a nonblocking stream.

Test the semantic contract twice: deterministic fake tests cover decisions,
while a small real `socketpair` or temporary-directory test covers the adapter.
If the fake diverges from the real interface, fix the fake and review tests
whose assumptions depended on it.

Use a stable result vocabulary such as complete, pending, rejected and unknown
outcome. This allows tests to distinguish “no side effect occurred” from
“the caller timed out after submission.”

## Related topics

- [Stage 16 overview](index.md)
- [Evidence and failure classification](../diagnostics-debugging-and-performance/userspace-failure-taxonomy-and-evidence.md)

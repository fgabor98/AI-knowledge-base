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

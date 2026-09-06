# Host Fixtures, Fakes, And Simulators

Host tests should exercise real OS primitives where that improves confidence without requiring privileged hardware. Temporary directories test permissions and atomic replacement; `socketpair()` tests IPC; PTYs test serial-like framing; subprocesses test signals, exit status, and descriptor inheritance.

## Model failure, not just success

Fakes should be able to produce short reads/writes, `EINTR`, `EAGAIN`, disconnects, malformed frames, delayed events, clock jumps, full storage, permission failures, and device resets. A simulator can model protocol and timing, but it must document what it does not model: kernel scheduling, electrical timing, driver bugs, DMA, filesystem wear, and real power loss.

## Fixture hygiene

Give every test an isolated temporary root, unique ports or socket names, bounded cleanup, and a clear timeout. Avoid tests that depend on the host's account database, current locale, timezone, network, `/dev`, or service manager unless they are explicitly integration tests. Parallel tests must not share mutable state.

Capture logs and traces on failure, but redact secrets and cap artifact size. A fixture that leaves processes or mounts behind can contaminate later tests and create misleading failures.

## Fidelity ladder

Start with a fake for fast logic tests, then a protocol simulator, then a real unprivileged OS fixture, then a target or device test. Promote a test upward when the lower layer has repeatedly missed a class of defect; keep the lower test because it remains faster and more diagnostic.

---
status: draft
reviewed: false
domain: linux-userspace
difficulty: advanced
last_reviewed: null
---

# Host Fixtures, Fakes, And Simulators

Host tests should exercise real OS primitives where that improves confidence without requiring privileged hardware. Temporary directories test permissions and atomic replacement; `socketpair()` tests IPC; PTYs test serial-like framing; subprocesses test signals, exit status, and descriptor inheritance.

## Model failure, not just success

Fakes should be able to produce short reads/writes, `EINTR`, `EAGAIN`, disconnects, malformed frames, delayed events, clock jumps, full storage, permission failures, and device resets. A simulator can model protocol and timing, but it must document what it does not model: kernel scheduling, electrical timing, driver bugs, DMA, filesystem wear, and real power loss.

## Fixture hygiene

Give every test an isolated temporary root, unique ports or socket names, bounded cleanup, and a clear timeout. Avoid tests that depend on the host's account database, current locale, timezone, network, `/dev`, or service manager unless they are explicitly integration tests. Parallel tests must not share mutable state.

Capture logs and traces on failure, but redact secrets and cap artifact size. A fixture that leaves processes or mounts behind can contaminate later tests and create misleading failures.

## Fidelity ladder

Start with a fake for fast logic tests, then a protocol simulator, then a real unprivileged OS fixture, then a target or device test. Promote a test upward when the lower layer has repeatedly missed a class of defect; keep the lower test because it remains faster and more diagnostic.

## Choose a fixture by the claim

| Fixture | Useful claim | Important limit |
| --- | --- | --- |
| `socketpair` | Framing, partial reads, EOF, bounded output | No IP routing, loss or DNS |
| Loopback TCP | Connect/accept, address handling, stream lifecycle | No physical link failure |
| PTY | Line discipline, termios and serial parser | No actual baud or modem-line timing |
| Private temporary directory | File replacement, permissions, cleanup | Host filesystem is not target flash |
| Fake device adapter | Exact error and late-event scheduling | Cannot prove driver semantics |
| Separate child process | Exit, signals, crashes, inheritance | Requires explicit reaping and timeout ownership |

A small send buffer can create backpressure but cannot guarantee a particular
partial count on every run. To test parser fragmentation deterministically,
feed every split point directly into the parser. Use real sockets to verify
the adapter handles whatever short counts actually occur.

## Synchronize fixture readiness

Have the fixture report readiness through a pipe/socket after setup. Starting
a subprocess and sleeping for 100 ms creates a timing-dependent test. For TCP,
bind port zero, retrieve the assigned port with `getsockname`, and communicate
it while the socket remains open; reserving then closing a port creates a race.

Give cleanup a separate deadline. Stop input, request orderly exit, collect
evidence on timeout, terminate the owned process scope if necessary, and reap.
Keep failed test directories when they carry evidence and record their paths.
Do not let unrelated test workers share socket paths or service state.

See [socketpair(2)](https://man7.org/linux/man-pages/man2/socketpair.2.html)
and [PTYs](https://man7.org/linux/man-pages/man7/pty.7.html).

## Related topics

- [Stage 16 overview](index.md)
- [Evidence and failure classification](../diagnostics-debugging-and-performance/userspace-failure-taxonomy-and-evidence.md)

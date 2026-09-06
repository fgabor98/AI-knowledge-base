# Stage 16: Testing And Verification

Userspace verification must cover pure behavior, operating-system boundaries, target boot, hardware variation, and failure over time. A host unit-test suite is valuable, but it cannot prove that a service starts with the target loader, discovers the real device, survives power loss, or rolls back safely.

## Verification pyramid

Use fast deterministic unit tests for parsing and state transitions, host integration tests for files/IPC/processes, sanitizers and fuzzers for memory and input robustness, target tests for ABI/UAPI and boot integration, and hardware-in-the-loop tests for timing, power, thermal, and physical fault behavior. Keep a small set of end-to-end tests that represent user-visible requirements.

Every test should state its environment, oracle, timeout, cleanup, artifacts, and failure classification. A timeout without a thread dump, service state, kernel log, and device status is a weak diagnostic.

## Completion checklist

- [ ] Core logic runs without real hardware or global system state.
- [ ] Fakes model errors, delays, short I/O, reconnects, and malformed data.
- [ ] Sanitizers and fuzzers run continuously on supported toolchains.
- [ ] Target tests verify boot, packaging, permissions, devices, and services.
- [ ] Power-cycle, update, rollback, and long-run tests preserve useful evidence.

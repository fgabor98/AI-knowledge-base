---
status: draft
reviewed: false
domain: linux-userspace
difficulty: advanced
last_reviewed: null
---

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

## Stage exercise and evidence levels

Start from a framed request parser, transport adapter and timeout state machine.
Test valid input at every split point, malformed lengths, partial writes,
deadline expiry and late replies. Then run the same behavior through a real
host fixture and the packaged target service.

Label evidence by what ran: unit, host integration, instrumented build, target
boot, device integration or hardware interruption. Preserve unsupported and
skipped cases separately. The deliverable is a traceable set of behavioral
claims with reproducing commands, expected results, artifacts and known limits.

## Learning materials

1. [Testable Userspace Architecture](testable-userspace-architecture.md)
2. [Host Fixtures, Fakes, And Simulators](host-fixtures-fakes-and-simulators.md)
3. [Unit, Integration, Sanitizer, And Fuzz Testing](unit-integration-sanitizer-and-fuzz-testing.md)
4. [Target Boot And Device Integration Tests](target-boot-and-device-integration-tests.md)
5. [Lifecycle, Update, And Hardware-In-The-Loop Tests](lifecycle-update-and-hardware-in-the-loop-tests.md)

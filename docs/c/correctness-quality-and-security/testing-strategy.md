---
status: draft
reviewed: false
domain: c
difficulty: advanced
last_reviewed: null
---

# Testing Strategy

Testing C is an evidence-design problem. The objective is not to maximize a single coverage number; it is to exercise the relevant state space, boundaries, failures, timing, resource limits, and hardware interactions with tests that are repeatable and diagnostically useful.

## Learning Objectives

- choose unit, integration, system, target, and hardware-in-loop boundaries;
- design tests from requirements, state machines, contracts, and hazards;
- cover normal, boundary, malformed, concurrent, timing, power, and resource-failure behavior;
- use fakes, mocks, property tests, fuzzing, differential tests, and fault injection responsibly;
- measure coverage and test strength without treating metrics as proof;
- preserve target and release evidence.

## Test Pyramid For Embedded C

```text
                         field / acceptance
                    system and HIL tests
               target integration and timing
          host integration and contract tests
     unit, property, fuzz, sanitizer, and static checks
```

Lower layers should be fast and numerous; higher layers should validate real integration and hardware assumptions. A lower-layer test must not fake away the very contract it claims to verify. Conversely, a hardware test should not be used to discover simple parser boundary bugs that could have been found in milliseconds on a host.

## Test Design From Contracts

For each public operation, derive cases from:

- valid minimum, typical, maximum, and just-outside values;
- nullability, empty input, truncation, and malformed encoding;
- ownership transfer and repeated calls;
- every state transition and illegal transition;
- each error source and recovery path;
- concurrency interleavings and cancellation;
- timeouts, clock wrap, retry, and watchdog behavior;
- memory, stack, queue, flash, and power limits;
- reset, brownout, and interrupted-update behavior.

Example table for a frame decoder:

| Class | Example |
| --- | --- |
| nominal | valid frame with each supported command |
| boundary | zero payload, maximum payload, sequence wrap |
| malformed | bad magic, unsupported version, invalid length |
| truncated | every prefix length of a valid frame |
| resource | output full, queue full, allocation unavailable |
| temporal | timeout at one tick before/at/after deadline |
| recovery | reset after header, payload, checksum, and commit |

## Unit Tests And Fakes

Unit tests isolate deterministic logic and replace hardware, time, transport, and allocation with controlled seams. A fake should model the behavior relevant to the unit, including failures and ordering—not merely return success.

~~~c
#include <assert.h>
#include <stddef.h>

struct tx_port {
    int (*send)(void *context, const unsigned char *data, size_t length);
    void *context;
};

static int send_record(const struct tx_port *port,
                       const unsigned char *data,
                       size_t length)
{
    if (port == NULL || port->send == NULL || data == NULL) {
        return -1;
    }
    return port->send(port->context, data, length);
}

static void test_rejects_missing_transport(void)
{
    unsigned char byte = 0u;
    assert(send_record(NULL, &byte, 1u) < 0);
}
~~~

Do not test only that a fake was called. Assert the contract: exact bytes, length, order, retry behavior, timeout handling, and output state after failure.

## Integration And Contract Tests

Integration tests combine modules at a real boundary:

- parser with transport framing;
- driver with RTOS queue and interrupt path;
- storage layer with power-loss recovery;
- bootloader with image format and signature verifier;
- C API with a foreign-language caller.

Contract tests let multiple implementations satisfy the same suite. For example, a host UART fake, an RTOS queue adapter, and a Linux file-descriptor adapter should all pass tests for ordering, partial writes, timeout, and error semantics.

## Property-Based And Fuzz Testing

Property tests generate many inputs and check invariants instead of one expected output. Useful C properties include:

- decoding then encoding preserves canonical meaning;
- accepted lengths never exceed the configured bound;
- a failed operation leaves state unchanged;
- queue count remains within `[0, capacity]`;
- consuming output never creates bytes that were not supplied;
- processing the same idempotent command twice has the specified effect.

Fuzz harnesses should be small and deterministic:

~~~c
#include <stddef.h>

struct frame {
    unsigned char reserved;
};
int frame_decode(const unsigned char *data, size_t size, struct frame *out);

int LLVMFuzzerTestOneInput(const unsigned char *data, size_t size)
{
    struct frame frame;
    (void)frame_decode(data, size, &frame);
    return 0;
}
~~~

The harness must not depend on real time, persistent global state, random hardware, or unbounded resources. Seed it with valid corpus files, malformed examples, protocol versions, and previously fixed defects. Reproduce every crash with the minimized input and exact build.

## Differential Testing

Compare two implementations or versions on the same inputs:

- optimized and reference CRC implementation;
- old and new protocol decoder;
- host model and target implementation;
- hardware register model and silicon trace;
- independent image parser and bootloader parser.

Define which differences are intentional. A differential test that compares undefined behavior, unspecified ordering, or uninitialized padding can report noise rather than a defect.

## Timing, Resource, And Power Tests

Embedded correctness includes budgets:

- interrupt latency and maximum critical-section duration;
- task worst-case execution time and deadline miss behavior;
- stack high-water mark and interrupt nesting;
- heap fragmentation and allocation failure;
- queue occupancy and backpressure;
- flash erase/program time;
- current during sleep, wake, radio, and fault paths;
- watchdog margin and recovery time.

Measure on representative hardware with interrupts, caches, compiler optimization, and power states configured as in production. A debugger can alter timing and prevent low-power states, so include instrumentation that runs without the debugger.

## Hardware-In-The-Loop

HIL tests should control and observe the real boundary:

1. identify firmware version and hardware revision;
2. reset through the same mechanism used in production;
3. apply controlled clocks, inputs, faults, and power interruptions;
4. collect UART, trace, GPIO, bus, current, and reset evidence;
5. enforce timeouts so a hung target cannot stall the test farm;
6. preserve logs and the exact image for every failure;
7. cleanly recover boards between tests.

HIL is excellent for startup, drivers, DMA, interrupts, timing, power, and recovery. It is expensive and can be less deterministic than a host test, so keep the test case narrow and the setup observable.

## Fault Injection

Inject failures at every layer:

- invalid arguments and lengths;
- allocation and queue exhaustion;
- dropped, duplicated, delayed, and reordered messages;
- CRC/authentication failure;
- flash write interruption;
- sensor stuck, out-of-range, or delayed;
- clock failure, watchdog expiry, and brownout;
- DMA error, bus fault, and interrupt storm;
- corrupted configuration or rollback metadata.

Verify not only that the system detects the fault, but that it enters a defined safe or degraded state, records useful evidence, avoids retry storms, and recovers without violating ownership or security properties.

## Coverage And Test Evidence

Track more than line coverage:

- branch and decision coverage;
- MC/DC where the safety process requires it;
- state-transition coverage;
- boundary and equivalence-class coverage;
- fault and error-path coverage;
- requirements-to-test traceability;
- target/platform/configuration coverage;
- mutation or seeded-defect effectiveness where practical.

Coverage measures executed or analyzed structure; it does not prove the absence of defects. Review uncovered code, generated code, defensive branches, and unreachable claims. Make exclusions explicit and justified.

## Exercises

1. Build the frame-decoder test matrix and implement boundary and truncation tests.
2. Write a fake transport that injects partial writes, timeout, and failure.
3. Add a fuzz harness and seed it with valid, malformed, and regression inputs.
4. Compare an optimized implementation against a simple reference implementation.
5. Measure stack, timing, and queue occupancy on target hardware.
6. Interrupt a persistent update at every commit point and verify recovery.
7. Define a release test report that includes image identity, hardware revision, tools, logs, and coverage limits.

## Common Mistakes

- equating line coverage with test quality;
- mocking away timing, ownership, or hardware behavior;
- testing only valid inputs and nominal power;
- using nondeterministic random or wall-clock state in unit tests;
- failing to preserve minimized fuzz inputs and exact binaries;
- ignoring partial writes, retries, queue full, and allocation failure;
- running HIL without reset, timeout, and board-recovery controls;
- measuring timing only under a debugger or at the wrong optimization level;
- treating an unverified coverage exclusion as unreachable by assertion.

## Related Topics

- [Static Analysis](./static-analysis.md)
- [Sanitizers And Dynamic Analysis](./sanitizers-and-dynamic-analysis.md)
- [Debugging With GDB](./debugging-with-gdb.md)
- [Formal Methods](./formal-methods.md)
- [Bootloaders And Firmware Images](../embedded-c-and-hardware/bootloaders-and-firmware-images.md)

## References

- [LLVM libFuzzer documentation](https://llvm.org/docs/LibFuzzer.html)
- [Clang AddressSanitizer documentation](https://clang.llvm.org/docs/AddressSanitizer.html)
- [GCC instrumentation options](https://gcc.gnu.org/onlinedocs/gcc/Instrumentation-Options.html)
- [NIST Secure Software Development Framework](https://csrc.nist.gov/pubs/sp/800/218/final)

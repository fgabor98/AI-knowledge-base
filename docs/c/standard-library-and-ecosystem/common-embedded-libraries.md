---
status: draft
reviewed: false
domain: c
difficulty: intermediate
last_reviewed: null
---

# Common Embedded Libraries

Embedded projects rarely use ISO C alone. They combine a compiler runtime, vendor SDK, board-support package, RTOS, networking and storage stacks, security libraries, protocol code, and test tools. Each dependency introduces APIs, memory ownership, configuration, licensing, update, and integration risk.

## Learning Objectives

- Classify common embedded library families.
- Evaluate APIs for footprint, timing, ownership, and execution context.
- Integrate vendor HALs and CMSIS-style interfaces without leaking them upward.
- Compare RTOS, networking, TLS, USB, CAN, filesystem, serialization, and test libraries.
- Pin versions and record configuration and licensing.
- Build project wrappers and fakes around third-party dependencies.

## Vendor HALs And BSPs

A vendor HAL commonly provides:

- clock and reset setup;
- GPIO, UART, SPI, I2C, ADC, timer, and DMA access;
- interrupt registration;
- startup files and linker scripts;
- device headers and register definitions;
- board and pin configuration.

Treat the HAL as target-specific. Keep it below a project port so application and protocol modules do not depend on vendor naming, global state, or callback conventions.

Validate:

- register access width and volatile behavior;
- interrupt and DMA ownership;
- error and timeout semantics;
- reinitialization and shutdown;
- generated configuration;
- version and silicon errata coverage.

CMSIS-style interfaces can standardize Cortex-M core and peripheral access, but CMSIS itself is not a complete application architecture. Check the device pack, compiler, startup, and vendor implementation.

## RTOS APIs

An RTOS supplies tasks, queues, semaphores, mutexes, timers, event flags, memory pools, and ISR-safe variants. The API contract includes priority, scheduling, blocking, timeout units, interrupt context, and object lifetime.

Never substitute a task API for an ISR API because the names look similar. Record whether a call can block, whether a timeout is tick-based, whether priority inheritance exists, and what happens during scheduler startup and shutdown.

A project wrapper can keep application policy portable:

~~~c
struct event_queue_port {
    int (*send)(void *context, const void *event, unsigned int timeout_ticks);
    int (*send_from_isr)(void *context, const void *event);
    int (*receive)(void *context, void *event, unsigned int timeout_ticks);
    void *context;
};
~~~

The wrapper must preserve distinctions that matter, especially ISR-safe and blocking behavior.

## FreeRTOS And Zephyr

FreeRTOS is often selected for a small kernel and explicit object APIs. Zephyr provides a broader integrated platform with device model, build/configuration system, drivers, networking, storage, and kernel services. The right choice depends on product requirements, certification strategy, team expertise, ecosystem, and memory budget.

For either system, evaluate:

- task and interrupt stack sizing;
- tick or tickless timing;
- queue and synchronization semantics;
- static versus dynamic object allocation;
- heap implementation and failure hooks;
- tracing and observability;
- SMP or multicore support;
- power management;
- update and security model;
- upstream maintenance and vendor patches.

Avoid sprinkling RTOS calls across portable domain code. Centralize task, time, and synchronization adapters.

## Networking And lwIP

A TCP/IP stack such as lwIP introduces packet buffers, ownership, callbacks, thread/core locking, timers, checksum, DMA, cache, and configuration constraints. A packet buffer is not automatically safe to retain or modify after handing it to the stack.

Define:

- which context may call the stack;
- who owns pbuf-like objects at each callback;
- when data is copied versus referenced;
- how link and interface state changes;
- timeout and retransmission policy;
- buffer pool exhaustion behavior;
- security and update requirements.

Keep protocol parsing separate from transport and use fuzzing and host tests for malformed packets.

## TLS Libraries

TLS libraries such as mbed TLS or wolfSSL add cryptographic computation, entropy, certificate parsing, key storage, random generators, buffers, and callbacks. Evaluate:

- hardware crypto integration;
- entropy source quality;
- certificate and key lifetime;
- secure zeroization;
- maximum handshake memory;
- blocking and timeout behavior;
- session and renegotiation policy;
- configuration reduction;
- vulnerability response and update process.

Never replace certificate validation with a “temporary” accept-all callback in production. Keep secrets out of logs and define failure behavior for entropy or storage faults.

## USB, CAN, And Device Protocols

USB stacks require endpoint buffers, descriptor ownership, control-transfer state, and interrupt/DMA rules. CAN stacks require mailbox/filter ownership, bus-off recovery, timing configuration, and frame validation.

For every stack, document:

- buffer ownership at ingress and egress;
- maximum frame and queue sizes;
- callback context;
- error and recovery states;
- concurrency and locking;
- static memory budget;
- versioned wire formats;
- test and analyzer strategy.

Use typed project interfaces rather than letting third-party structures spread across the application.

## Filesystems

FatFs and similar embedded filesystem libraries are useful on removable or flash-backed storage but add media, block-cache, locking, power-loss, and wear assumptions. Check:

- reentrancy configuration;
- sector size and alignment;
- cache flush and sync;
- file-system corruption recovery;
- allocation and fragmentation;
- long filename and Unicode options;
- power-fail atomicity;
- read-only and recovery modes.

A filesystem API does not guarantee durable data until the medium and synchronization policy say so.

## Serialization Libraries

Serialization tools such as nanopb, CBOR, or project-specific encoders can reduce protocol defects, but generated code and schema evolution are part of the ABI:

- bound decoded sizes;
- reject unknown or oversized input according to policy;
- version fields and compatibility;
- avoid unbounded allocation;
- define endianness and canonical encoding;
- test malformed and truncated messages;
- review generated source and configuration.

For very small fixed packets, explicit encoding may be clearer and smaller than a general framework.

## Test And Mocking Frameworks

Embedded test stacks commonly combine host unit tests, fake HALs, hardware-in-the-loop, Unity/CMock-style frameworks, property tests, fuzzers, and target smoke tests.

A useful test boundary:

~~~c
struct flash_port {
    int (*read)(void *context, uint32_t address,
                void *buffer, size_t length);
    int (*write)(void *context, uint32_t address,
                 const void *buffer, size_t length);
    void *context;
};
~~~

The same service can run against a memory fake, a fault-injecting fake, and real flash. Keep fakes honest: they should model the timing, alignment, failure, and ownership facts that the production code depends on.

## Dependency Evaluation

Before adopting a library, record:

- license and redistribution obligations;
- source provenance and version pinning;
- supported compilers and targets;
- maintenance activity and vulnerability response;
- API and ABI stability;
- static RAM, flash, stack, and heap cost;
- worst-case execution time;
- concurrency and ISR rules;
- configuration surface and generated code;
- test coverage and hardware validation;
- ability to remove, replace, or fork it.

A small library with a clear contract can be safer than a larger framework even when the framework has more features.

## Integration Strategy

Wrap third-party dependencies at a project boundary:

~~~c
struct network_port {
    int (*send)(void *context, const uint8_t *data, size_t length);
    int (*receive)(void *context, uint8_t *data, size_t capacity,
                   size_t *length);
    void *context;
};
~~~

The wrapper translates error codes, ownership, timing, and types. Keep vendor headers out of portable modules where possible. Pin configuration and generated artifacts with the dependency version.

## Exercises

1. Evaluate a vendor HAL with an API, ownership, timing, and version checklist.
2. Wrap one RTOS queue API and provide a host fake.
3. Model network packet-buffer ownership across receive, parse, and transmit.
4. Build a reduced TLS configuration and measure handshake memory and time.
5. Test filesystem power-loss behavior and synchronization policy.
6. Generate a bounded serialization schema and fuzz malformed input.
7. Compare a third-party mock with a hardware-in-the-loop test for the same failure.

## Common Mistakes

- Letting vendor or RTOS types leak through all application interfaces.
- Calling blocking APIs from interrupts.
- Ignoring queue, packet, DMA, or filesystem ownership.
- Treating a callback as synchronous when it is deferred.
- Enabling every library feature without measuring footprint.
- Accepting insecure TLS or serialization defaults.
- Assuming a filesystem sync equals power-loss durability.
- Failing to pin versions and generated configuration.
- Ignoring licensing, vulnerability response, and update paths.
- Writing fakes that are more permissive than production hardware.

## Debugging Checklist

1. Identify the exact library version, configuration, compiler, and target.
2. Trace ownership and context at every boundary.
3. Check static RAM, heap, stack, flash, and worst-case timing.
4. Validate ISR, task, callback, and locking rules.
5. Inject allocation, queue, media, link, crypto, and hardware failures.
6. Compare fake behavior with production constraints.
7. Review generated code, schemas, linker sections, and license metadata.
8. Test upgrade, downgrade, recovery, and removal of the dependency.

## Related Topics

- [Standard Library And Ecosystem overview](./index.md)
- [Embedded libc Implementations](./embedded-libc.md)
- [POSIX And System Interfaces](./posix-and-system-interfaces.md)
- [Modular Design And APIs](../modular-design-and-apis/index.md)
- [Embedded C And Hardware](../embedded-c-and-hardware/index.md)
- [Correctness, Quality, And Security](../correctness-quality-and-security/index.md)
- [C Programming](../index.md)

## References

- [CMSIS documentation](https://arm-software.github.io/CMSIS_5/General/html/index.html)
- [FreeRTOS documentation](https://www.freertos.org/Documentation/02-Kernel/02-Kernel-features/00-Developer-docs)
- [Zephyr documentation](https://docs.zephyrproject.org/latest/)
- [lwIP documentation](https://www.nongnu.org/lwip/)
- [Mbed TLS documentation](https://mbed-tls.readthedocs.io/en/latest/)
- [FatFs documentation](https://elm-chan.org/fsw/ff/)
- [nanopb documentation](https://jpa.kapsi.fi/nanopb/)

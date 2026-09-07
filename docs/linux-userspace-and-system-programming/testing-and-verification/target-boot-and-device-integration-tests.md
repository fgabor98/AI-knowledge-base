---
status: draft
reviewed: false
domain: linux-userspace
difficulty: advanced
last_reviewed: null
---

# Target Boot And Device Integration Tests

## Boot acceptance

On a clean target, verify image identity, boot completion, mount topology, writable paths, service enablement, user/group creation, capabilities, sandbox policy, clock handling, network setup, and log availability. Check both a normal boot and a boot with an expected dependency absent.

## Device contract

Exercise discovery, permissions, open/close, initialization, normal I/O, short or delayed I/O, hotplug/reconnect, reset, suspend/resume, and shutdown. Assert the exact errno or structured error where it is part of the contract. Verify that device nodes, sysfs attributes, udev rules, and service ordering agree.

Use a matrix of board revisions, kernel/configuration versions, firmware versions, peripherals, storage sizes, and network conditions. Record serial output, kernel messages, service state, device identity, and application logs with a common timeline.

Tests must leave the target recoverable. Bound retries, avoid filling persistent storage, and provide a rescue or reflash path when an intentionally induced fault prevents normal boot.

## Make boot acceptance observable

Before boot, associate a run ID with the image digest and board identity.
Start serial capture before power-on, then wait for a bounded readiness
event. Network login, a PID and a started unit each prove different facts.
Perform an actual application transaction after the service reports ready.

Collect the effective service identity, mount source, configuration generation,
device identity, negotiated capabilities and first valid sample. A clean
test image must not rely on directories or permissions created manually
during development.

## Turn a device test into an oracle

For a temperature sensor, “read returned bytes” is weak. Define a reference
range or calibrated stimulus, units, maximum sample age, expected rate,
sequence continuity and tolerated error. For an actuator, observe the physical
effect where the requirement concerns physical output; a driver acknowledgement
alone cannot prove it.

Inject device absence at boot, unplug during a request and reconnect with a
new instance. Expect bounded failures, retired old requests, capability
renegotiation and a new generation. If topology changes, identify the replacement
before treating it as the intended device.

Record skipped tests explicitly with a reason. “No device present, skipped”
must not count as passing device integration. Separate fixture/network access
failures from application or hardware failures, while retaining evidence for both.

## Related topics

- [Stage 16 overview](index.md)
- [Evidence and failure classification](../diagnostics-debugging-and-performance/userspace-failure-taxonomy-and-evidence.md)

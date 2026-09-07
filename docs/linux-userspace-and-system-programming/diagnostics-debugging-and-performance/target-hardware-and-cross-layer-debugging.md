---
status: draft
reviewed: false
domain: linux-userspace
difficulty: advanced
last_reviewed: null
---

# Target Hardware And Cross-Layer Debugging

## Preserve the target story

Record board revision, bootloader, kernel, device tree/firmware, root filesystem, application build ID, configuration generation, power/network state, and connected peripherals. “Same software” is not a useful statement when the hardware revision or firmware differs.

Use a monotonic timestamp for event ordering and a synchronized wall clock for correlation with external systems. Capture userspace logs, service-manager state, kernel messages, device discovery, bus errors, watchdog events, thermal/power signals, and relevant hardware counters in one evidence bundle.

## Narrow the layer

Reproduce with a known-good peripheral or a loopback where possible. Compare raw device reads, ioctl results, sysfs attributes, and kernel logs before changing application logic. If the kernel reports a transport error, userspace should preserve that errno and context rather than replacing it with a generic timeout.

Serial consoles, controlled reboot/power-cycle equipment, trace probes, logic analyzers, and fault-injection fixtures are valuable when used with a written experiment. Make recovery safe: preserve logs before reset, bound retry storms, and avoid an experiment that can corrupt persistent state without a recovery image.

## Close the loop

Once the cause is known, add the missing observability at the boundary: a device identity, request ID, generation, errno, latency, state transition, or firmware status. Convert the field procedure into an automated test or runbook so the next failure produces evidence without ad hoc access.

## Correlate clocks before correlating causes

A userspace monotonic timestamp, kernel trace clock, device sample clock and
logic-analyzer timestamp can have different epochs and rates. Use a shared
observable event or calibrated conversion and record its uncertainty.
Ordering events more precisely than the clock relationship supports can
produce a false explanation.

For a serial timeout, distinguish these milestones: command queued in
userspace, bytes accepted by the driver, last bit transmitted, device response
begun, response bytes read, and frame validated. A logic analyzer showing a
prompt response while the application reports timeout shifts investigation
toward scheduling, buffering or parsing. An absent response requires protocol,
wiring, power and device-state evidence.

## Compare controlled substitutions

Change one dimension: same image on another board, same board with another
peripheral, or same captured bytes through a host parser. Document which
hypothesis each substitution can test. A host fake can rule in a parser bug;
its success cannot rule out driver or electrical faults.

Preserve the original failing image and calibration before reflashing. Record
whether instrumentation, slower bus rates or an attached debugger makes the
failure disappear. That timing sensitivity is itself evidence, and the final
fix must be checked again under the original operating conditions.

## Related topics

- [Stage 14 overview](index.md)
- [Testing and verification](../testing-and-verification/index.md)

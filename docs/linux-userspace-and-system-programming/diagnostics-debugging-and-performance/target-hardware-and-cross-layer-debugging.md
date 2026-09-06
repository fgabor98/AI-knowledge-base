# Target Hardware And Cross-Layer Debugging

## Preserve the target story

Record board revision, bootloader, kernel, device tree/firmware, root filesystem, application build ID, configuration generation, power/network state, and connected peripherals. “Same software” is not a useful statement when the hardware revision or firmware differs.

Use a monotonic timestamp for event ordering and a synchronized wall clock for correlation with external systems. Capture userspace logs, service-manager state, kernel messages, device discovery, bus errors, watchdog events, thermal/power signals, and relevant hardware counters in one evidence bundle.

## Narrow the layer

Reproduce with a known-good peripheral or a loopback where possible. Compare raw device reads, ioctl results, sysfs attributes, and kernel logs before changing application logic. If the kernel reports a transport error, userspace should preserve that errno and context rather than replacing it with a generic timeout.

Serial consoles, controlled reboot/power-cycle equipment, trace probes, logic analyzers, and fault-injection fixtures are valuable when used with a written experiment. Make recovery safe: preserve logs before reset, bound retry storms, and avoid an experiment that can corrupt persistent state without a recovery image.

## Close the loop

Once the cause is known, add the missing observability at the boundary: a device identity, request ID, generation, errno, latency, state transition, or firmware status. Convert the field procedure into an automated test or runbook so the next failure produces evidence without ad hoc access.

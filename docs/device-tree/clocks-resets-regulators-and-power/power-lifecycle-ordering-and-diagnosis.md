---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# Power Lifecycle, Ordering, And Diagnosis

Resource properties become useful only when the entire lifecycle is correct: cold boot, deferred probe, idle, runtime resume, system suspend, wake, shutdown, reboot, unbind/rebind, and failure rollback. Senior-level review follows the dependency graph through all of them.

## Build A Hardware Dependency Graph

For each device, inventory:

| Dependency | Questions |
|---|---|
| supplies | Which rail, upstream source, voltage, enable delay, and power-good evidence? |
| domain | Which switch/isolation/retention state, and who sequences it? |
| clocks | Which inputs, required parent/rate, and shared ancestors? |
| resets | Level or pulse, shared or exclusive, and required ordering? |
| pins/IRQs | Which sleep state and wake path remain powered? |
| OPPs | Which rates, voltages, domain states, and silicon variants are safe? |
| thermal | Which sensor protects the block, and which action reduces heat? |

Then identify the Linux owner for every transition. Two owners for one control create contention; no owner creates bootloader dependence.

## Enable And Disable Are Not Simple Reversals

A conceptual enable transaction might be:

```text
claim suppliers
  -> enable upstream and local rails
  -> request domain state
  -> configure parent/rate
  -> enable bus and functional clocks
  -> release isolation/reset
  -> verify readiness
```

Disable often reverses this, but exceptions matter. A reset may need assertion before clocks stop. A wake detector must stay powered. A firmware mailbox may need to remain accessible until the final domain command completes. Retention may preserve state without leaving the core clock active.

Document the real state machine in the driver or provider. DTS property order cannot encode it.

## Failure Rollback

Every step can fail. Correct drivers unwind only resources acquired so far and leave hardware in a safe state. Probe deferral deserves special care: a partially enabled rail or clock must not leak on every retry.

Review with fault injection or targeted provider failures where possible:

- regulator enable fails
- clock rate cannot be achieved
- reset deassert reports an error
- domain resume times out
- firmware rejects a performance state
- readiness polling times out

The most useful log reports the semantic resource and operation, not only an error number.

## Probe Deferral Is A Graph Signal

When a lookup returns `-EPROBE_DEFER`, trace the referenced supplier instead of changing initcall order. Determine:

1. does the runtime consumer property exist?
2. does its phandle resolve to the intended node?
3. is the provider and every ancestor available?
4. did the provider device get created and bind?
5. did it register the exact resource ID?
6. is a device-link cycle preventing progress?

An absent optional property is not the same as a present property whose provider is late. Optional getters must suppress absence, not malformed descriptions or provider errors.

## Runtime Suspend And Resume

Exercise repeated cycles under activity:

```sh
echo auto | sudo tee /sys/bus/platform/devices/DEVICE/power/control
cat /sys/bus/platform/devices/DEVICE/power/runtime_status
```

Use subsystem-appropriate workloads and tracing. Verify:

- register accesses stop before clocks or domains turn off
- DMA is quiesced before power loss
- interrupts cannot race with suspended register state
- context is saved/restored when retention is absent
- OPP and regulator votes are released and reacquired
- autosuspend does not oscillate under normal traffic

A stable device with runtime PM forced to `on` proves only the active path.

## System Suspend And Wake

System suspend crosses more layers: device PM callbacks, buses, PM domains, wake IRQ configuration, late/noirq phases, firmware, and platform sleep states. A `wakeup-source` device may require its interrupt controller, pin, clock, and always-on domain to remain functional.

Test both runtime-active and runtime-suspended entry into system sleep. Also test aborted suspend, repeated cycles, and wake storms. Compare expected rail/current reductions with measured system power.

## Shutdown, Reboot, And Kexec

Shutdown ordering can differ from suspend ordering. A consumer may need to stop DMA before its IOMMU or parent domain disappears. A regulator marked always-on may remain powered while the device is reset. Firmware may expect a defined state on reboot.

Unbind/rebind and kexec expose hidden bootloader assumptions because the second driver instance inherits state from Linux, not necessarily from reset firmware. If these flows are supported, make ownership and reset behavior explicit.

## Diagnostic Ladder

Use evidence in this order:

1. validate source and schema
2. inspect the final DTB and live tree
3. resolve every provider tuple and supply phandle
4. confirm provider devices and driver binding
5. inspect framework state: clocks, regulators, PM, OPP, thermal
6. trace probe and power transitions
7. read hardware status registers where safe
8. measure clocks, enables, rails, reset, and current draw
9. compare cold/warm boot and lifecycle transitions

Each layer answers a different question. Schema proves structural conformance; a scope proves electrical behavior; neither alone proves correct ownership over suspend/resume.

## Anti-Patterns That Hide Root Causes

- `clk_ignore_unused` or regulator-always-on as a permanent fix.
- Disabling runtime PM globally because one driver has a race.
- Adding arbitrary delays without measuring the required condition.
- Increasing probe priority instead of describing a supplier.
- Widening voltage constraints to satisfy an invalid OPP.
- Removing thermal mappings to recover benchmark performance.
- Relying on warm-boot firmware state during validation.

## Authoritative References

- [Linux device power-management basics](https://docs.kernel.org/driver-api/pm/devices.html)
- [Linux runtime power-management framework](https://docs.kernel.org/power/runtime_pm.html)
- [Linux device links](https://docs.kernel.org/driver-api/device_link.html)
- [Linux power-management tracepoints](https://docs.kernel.org/trace/events-power.html)

## Continue

Proceed to the [Integrated Power Bring-Up Lab](integrated-power-bring-up-lab.md).

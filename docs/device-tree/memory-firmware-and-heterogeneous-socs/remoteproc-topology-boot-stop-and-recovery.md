---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# Remoteproc Topology, Boot, Stop, And Recovery

The remoteproc framework represents an auxiliary processor that Linux may load and control. Its platform driver translates generic lifecycle operations into SoC-specific power, reset, memory, interrupt, and boot-vector operations.

## Model The Hardware Block, Not The Firmware Task

A remote processor node normally represents a physical core, core cluster, or platform control block as defined by its binding. Firmware applications and RPMsg services are runtime software, not child hardware nodes unless a binding explicitly says otherwise.

Typical fixed resources include:

- control registers and local SRAM
- interrupts or mailboxes
- clocks and resets
- power domains
- IOMMU or address-translation hardware
- reserved-memory carveouts
- cluster mode, core mode, or boot-control properties

The exact representation varies substantially among SoCs. Generic remoteproc knowledge is insufficient to invent a platform node.

## Lifecycle States

Reason about an explicit state machine:

```text
offline
  -> resources acquired
  -> firmware parsed and loaded
  -> address mappings installed
  -> clocks/power/reset prepared
  -> boot vector programmed
  -> core released
  -> running
  -> crashed or stopping
  -> quiesced and reset
  -> mappings/resources released
  -> offline
```

Every transition needs rollback. If reset release fails after power is enabled, the driver must not leave a partially executing core or leaked mapping.

Remoteproc has both object lifetime and power/boot references. A registered remoteproc object can remain present while the core is offline. Do not equate sysfs existence with execution.

## Start Ordering

A platform-specific start commonly needs to:

1. verify ownership and operating mode
2. power domains and enable required clocks
3. hold the core in reset
4. configure address translation, TCM/SRAM mapping, and firewalls
5. load or make firmware visible
6. initialize mailboxes and notification paths
7. program the boot address
8. apply required cache or memory barriers
9. release reset and wait for readiness

Hardware may require a different order. A core managed by secure firmware may replace several steps with a firmware call. Follow the driver and reference manual.

## Attach To Already-Running Firmware

Some platforms support “attach” where boot firmware has started the core and Linux discovers or adopts it. This is not equivalent to normal load/start.

Define:

- who authenticated and loaded the image
- whether Linux may stop or recover the core
- how carveouts and resource tables are discovered
- whether vrings already contain live state
- how cache, IOMMU, clock, and power references are synchronized
- what warm boot and kexec do

Resetting an attached safety or management core can destabilize the entire SoC. The binding and driver must explicitly support the ownership model.

## Stop Is A Protocol Plus A Hardware Action

An orderly stop may request firmware shutdown, wait for acknowledgment, prevent new messages, drain or revoke buffers, mask notifications, assert reset, and release resources. A forced stop skips cooperative steps only after the host has contained DMA and memory access.

Before returning carveouts or shared buffers:

- halt every remote bus master
- disable or invalidate IOMMU mappings as appropriate
- synchronize pending interrupts and work
- resolve buffer ownership
- perform required cache maintenance

“Core reset asserted” may not stop a separate DMA engine configured by that core.

## Crash Detection And Recovery

Crashes can be reported by watchdogs, fatal interrupts, mailbox errors, IOMMU faults, or explicit platform signals. Collect evidence before automatic recovery destroys it:

- crash type and timestamp
- program counter/register dump where available
- firmware trace
- resource table and firmware build identity
- IOMMU fault address and requester
- remote and host protocol state
- outstanding buffers
- power/reset/clock state

Recovery should be bounded and policy-driven. Reboot loops can repeatedly corrupt shared state or hide a deterministic incompatibility. Production policy should define retry count, backoff, service degradation, and system-level escalation.

## Suspend And System Lifecycle

Choose whether a running core:

- stops before suspend
- enters a coordinated low-power state
- remains active as a wake source
- remains independently managed by secure or safety firmware

Then model all required always-on resources and wake interrupts. Linux runtime PM of the remoteproc control device must not gate a clock or memory path still used by the running core.

Test cold boot, warm boot, suspend/resume, kexec, shutdown, watchdog reset, and host-driver unbind only where the platform declares them safe.

## Runtime Interfaces And Evidence

Common evidence includes:

```sh
ls -l /sys/class/remoteproc
for d in /sys/class/remoteproc/remoteproc*; do
        cat "$d/name" "$d/state" "$d/firmware" 2>/dev/null
done
dmesg | grep -Ei 'remoteproc|rproc|firmware|crash|watchdog'
```

Available attributes depend on kernel version and configuration. State changes through sysfs may be disabled or inappropriate in production. Do not stop a core until its product role and ownership are known.

## Failure Classification

| Symptom | Likely layer |
|---|---|
| no remoteproc object | DT match, provider dependency, driver configuration |
| firmware request fails | name, packaging, loader policy |
| segment load rejected | carveout or address translation |
| start timeout | reset, clock, power, boot address, firmware early fault |
| core runs, no virtio device | resource table |
| virtio exists, no service | firmware protocol/RPMsg announcement |
| works once, restart fails | teardown, stale mailbox, cache, ownership |

## Authoritative References

- [Linux remoteproc framework](https://docs.kernel.org/staging/remoteproc.html)
- [Linux remoteproc sysfs ABI](https://github.com/torvalds/linux/blob/master/Documentation/ABI/testing/sysfs-class-remoteproc)
- [Linux remoteproc binding directory](https://github.com/torvalds/linux/tree/master/Documentation/devicetree/bindings/remoteproc)
- [Linux runtime power management](https://docs.kernel.org/power/runtime_pm.html)

## Continue

Proceed to [RPMsg, Mailboxes, Virtqueues, And Shared Memory](rpmsg-mailboxes-virtqueues-and-shared-memory.md).

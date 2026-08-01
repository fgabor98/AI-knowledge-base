---
status: draft
reviewed: false
domain: device-tree
difficulty: intermediate
last_reviewed: null
---

# Runtime Inspection

Runtime inspection proves what Linux received and what Linux did with it. A node visible in the live tree is not yet a device; a device is not yet matched; a loaded module is not yet bound; a driver symlink is not proof that hardware works. Diagnose each transition with evidence from the tree, firmware-node association, device model, module metadata, probe logs, supplier state, and subsystem result.

## Learning Outcomes

After completing this module, you should be able to:

- distinguish the boot FDT blob, Linux's unflattened live tree, `/sys/firmware/devicetree/base`, and `/proc/device-tree`
- use `/sys/firmware/fdt` when available without assuming it reflects later live-tree overlays
- prove root model, compatible list, serial identity, `/chosen`, memory, and overlay results using binary-safe reads
- decode NUL-terminated strings, string lists, booleans, big-endian cells, multi-cell integers, phandle arrays, and byte arrays
- capture the live filesystem tree with `dtc -I fs` while understanding which original DTB bytes and reservation data cannot be reconstructed
- compare built, packaged, pre-handoff, boot-blob, and live-tree checkpoints semantically and by exact hash where valid
- map a live DT node to the Linux device whose `of_node` symlink references it
- explain why some available nodes create no platform device and why bus children belong to I2C, SPI, PCI, or another bus
- trace `compatible` through device modalias, module aliases, module loading, driver registration, matching, probe, and binding
- distinguish absent device, unmatched device, deferred probe, failed probe, successful bind, and functioning subsystem state
- inspect deferred-probe reasons and supplier/consumer links without assuming all relationships are visible in one interface
- correlate clocks, regulators, resets, power domains, IRQs, DMA, IOMMU, pinctrl, and firmware failures with the final DT relationship
- use bind/unbind or `driver_override` only when the bus and driver lifecycle make the experiment safe
- collect a reproducible runtime evidence bundle before rebooting, rebinding, applying overlays, or changing the system

## Prerequisites

Complete [Build And Diagnostic Tools](build-and-diagnostic-tools.md). You should be able to identify, hash, decode, and compare built, packaged, selected, and pre-handoff DTBs and understand how provider-consumer relationships drive probe behavior.

## Learning Path

1. [Runtime Tree Surfaces And Boot-FDT Identity](runtime-inspection/runtime-tree-surfaces-and-boot-fdt-identity.md)
2. [Binary-Safe Property Inspection And Decoding](runtime-inspection/binary-safe-property-inspection-and-decoding.md)
3. [Live-Tree Capture, Normalization, And Semantic Diffing](runtime-inspection/live-tree-capture-normalization-and-semantic-diffing.md)
4. [From Live Device Tree Node To Linux Device](runtime-inspection/from-live-device-tree-node-to-linux-device.md)
5. [Matching, Modaliases, Modules, And Bound Drivers](runtime-inspection/matching-modaliases-modules-and-bound-drivers.md)
6. [Probe Deferral, Supplier Links, And Resource State](runtime-inspection/probe-deferral-supplier-links-and-resource-state.md)
7. [Controlled Bind/Unbind And Runtime Forensics](runtime-inspection/controlled-bind-unbind-and-runtime-forensics.md)
8. [Runtime Device Tree And Probe Forensics Lab](runtime-inspection/runtime-device-tree-and-probe-forensics-lab.md)

## Runtime Evidence Ladder

```text
boot FDT identity and final properties
  -> live device_node exists
  -> node is available under its binding/bus rules
  -> Linux device object is created on the correct bus
  -> device modalias and registered driver can match
  -> driver probe runs
  -> all suppliers/resources become available
  -> probe returns success and driver symlink appears
  -> subsystem/class interface is registered
  -> real hardware operation succeeds
```

Test the ladder from top to bottom. Jumping from “DTS looks right” to “driver bug” skips most failure classes.

## Evidence Matrix

| Observation | Proves | Does not prove |
|---|---|---|
| node in live tree | Linux has that node | device object exists |
| `status` absent/okay | OF considers node available | parent bus populated it |
| device under `/sys/bus/.../devices` | device registered | driver matched/probed |
| modalias present | device can request a match key | matching module is installed |
| module in `lsmod` | module code loaded | its driver bound this device |
| `driver` symlink | binding/probe completed | hardware function is correct |
| class/subsystem node | driver registered interface | I/O path is healthy under load |
| clean current `dmesg` grep | no matching retained message | probe never failed earlier or logs were complete |

## Snapshot Before Mutation

Before bind/unbind, module reload, overlay apply/remove, or reboot, collect:

```text
kernel release/config/build identity
boot FDT and live-tree captures where available
root model/compatible and /chosen
target DT node files and decoded properties
Linux device path, bus, of_node, modalias, driver link
module metadata and loaded state
deferred-probe and device-link evidence
full relevant kernel log with timestamps
subsystem-specific state
build/package/U-Boot hashes from the previous module
```

Runtime state is ephemeral. A successful rebind can erase the original failure sequence.

## Completion Check

You are ready for [Security And Production Lifecycle](security-and-production-lifecycle.md) when you can:

- identify which runtime export is a raw boot blob and which is a live tree view
- read every common property class without corrupting NULs, endianness, or cell grouping
- state why a live-tree filesystem capture cannot be byte-compared directly with the original DTB
- prove the exact Linux device object associated with a target `of_node`
- determine whether failure occurred at availability, population, matching, module loading, probe, deferral, binding, or functional operation
- decode the supplier reference responsible for a persistent deferral
- use logs and sysfs snapshots to build a causal boot/probe timeline
- reject an unsafe bind/unbind experiment based on users, DMA, IRQ, power, console, storage, or teardown constraints
- locate the first semantic divergence between pre-handoff and runtime evidence
- package field evidence that another engineer can analyze without access to the unit

## Authoritative References

- [Linux Devicetree Kernel API](https://docs.kernel.org/devicetree/kernel-api.html)
- [Linux Devicetree usage model](https://docs.kernel.org/devicetree/usage-model.html)
- [Linux driver-core binding model](https://docs.kernel.org/driver-api/driver-model/binding.html)
- [Linux driver infrastructure](https://docs.kernel.org/driver-api/infrastructure.html)
- [Linux sysfs ABI documentation](https://docs.kernel.org/admin-guide/abi.html)

## Related Topics

- [Build And Diagnostic Tools](build-and-diagnostic-tools.md)
- [Driver Matching](driver-matching.md)
- [Provider-Consumer Relationships](provider-consumer-relationships.md)
- [Boot-Time Mutation And Ownership](boot-time-mutation-and-ownership.md)
- [Security And Production Lifecycle](security-and-production-lifecycle.md)

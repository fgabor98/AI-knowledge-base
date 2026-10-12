---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# DMA, IOMMU, Reserved Memory, And Remote Processor Integration

Memory ownership errors often look nondeterministic because one agent corrupts another agent's data. Before enabling DMA-heavy devices or remote processors, build one address-space map shared by boot firmware, Linux, IOMMU configuration, firmware images, and Device Tree.

## Name Every Address Space

Label CPU physical, DMA/bus, IOVA, remote-device, and firmware load addresses explicitly in design documents and logs ([Name Every Address Space](../memory-firmware-and-heterogeneous-socs/dma-addressing-coherency-and-iommu-topology.md#name-every-address-space)); the same numeric value can mean different spaces.

## Create A Memory Ownership Map

Example:

| Range | Size | Owner | DT representation | Attributes/lifecycle |
|---|---:|---|---|---|
| `0x80000000..0xbfffffff` | 1 GiB | Linux RAM | `/memory` | normal RAM |
| `0x90000000..0x90ffffff` | 16 MiB | remote core firmware | reserved-memory + `memory-region` | `no-map`, fixed |
| `0x91000000..0x917fffff` | 8 MiB | vrings/buffers | reserved-memory | shared mapping rules |
| `0xbe000000..0xbfffffff` | 32 MiB | CMA pool | shared-dma-pool | reusable if binding says so |
| secure carveout | hidden | trusted firmware | reservation/firmware contract | never Linux-owned |

Validate no overlap with kernel, initrd, FDT, bootloader relocation, framebuffer, crash log, or update buffers.

## Reservation Forms And Firmware Agreement

Represent each region with the mechanism its consumer binding requires—`/reserved-memory` children referenced by `memory-region`, the FDT reservation map, or a shared DMA pool ([Reserved Memory](../standard-nodes-and-properties/reserved-memory.md), [CMA, Shared DMA Pools, And Static Carveouts](../memory-firmware-and-heterogeneous-socs/cma-shared-dma-pools-and-static-carveouts.md)). Do not generalize one remoteproc driver's region order or `no-map` use to another.

Static DTS, boot-firmware fixups, firmware resource tables, driver allocation, CMA, and hypervisor assignment can all determine memory. Choose one authority per fact or define an explicit negotiation; a resource table requesting one carveout while DT points to another is a conflict, not redundancy ([Reconcile DT And Resource Tables](../memory-firmware-and-heterogeneous-socs/firmware-images-resource-tables-and-host-contracts.md#reconcile-dt-and-resource-tables)).

## Validate DMA Reachability And IOMMU Relationships

Complete the [translation worksheet](../memory-firmware-and-heterogeneous-socs/dma-addressing-coherency-and-iommu-topology.md#a-translation-worksheet) for each DMA master, then test buffers below and above meaningful boundaries, not only the first allocation that succeeds. Never copy an IOMMU stream ID from the reference board when routing, package, or hardware revision can change it; derive it from SoC interconnect documentation and, where supported, confirm in a safe lab test that an intentional invalid access produces a contained fault.

## Stage Remote Processor Bring-Up

1. Keep the remote core held in reset/off.
2. Validate all carveouts and ownership before Linux uses surrounding RAM.
3. Verify clocks, power domains, resets, mailbox/interrupts, and IOMMU.
4. Confirm firmware file identity and compatibility.
5. Parse/inspect the resource table if applicable.
6. Start once and capture host plus remote logs.
7. Verify firmware load addresses stay inside assigned regions.
8. Exercise IPC/virtio/rpmsg data integrity.
9. Stop/restart and confirm resources are released/reinitialized.
10. Test crash detection/recovery without corrupting Linux.

Do not auto-boot remote cores until memory safety and reset/recovery are proven.

## Inspect Evidence

```bash
cat /proc/iomem
dmesg --color=never | grep -Ei 'reserved|cma|dma|iommu|remoteproc|rproc|rpmsg|virtio'
find /sys/class/remoteproc -maxdepth 2 -type f -print 2>/dev/null
find /sys/kernel/iommu_groups -maxdepth 2 -type l -print 2>/dev/null
```

Platform debugfs may expose IOMMU or remoteproc details. Preserve crash traces and firmware hashes. Do not expose sensitive firmware memory in support bundles.

## Test Overlap Mechanically

Normalize all ranges to half-open intervals `[start, end)` and check:

```text
start < other_end && other_start < end  => overlap
```

Include alignment, integer width, and address-cell decoding. Validate both source artifacts and final runtime tree because firmware fixups may change memory.

## Common Failure Signatures

| Symptom | Likely boundary |
|---|---|
| random Linux crashes after remote start | overlapping carveout or uncontrolled DMA |
| firmware load fails | region size/address/ownership or firmware mismatch |
| IOMMU faults immediately | wrong stream ID, IOVA, permissions, or bypass expectation |
| rpmsg absent | resource table, vring, mailbox, interrupt, firmware boot |
| works below 4 GiB only | DMA mask/address-width/translation |
| restart fails after first stop | reset/power/resource cleanup lifecycle |
| reported RAM smaller than expected | deliberate reservation or wrong memory fixup |

## Stage Exit Gate

```text
[ ] one reviewed map covers every RAM/reserved/load/DMA range
[ ] address spaces and translations are labeled
[ ] no static or runtime reservation overlaps exist
[ ] DT and firmware resource tables agree by defined contract
[ ] each DMA master has validated reachability/coherency/IOMMU data
[ ] remote firmware identity is tied to release artifacts
[ ] start, IPC/data, stop, restart, and crash recovery pass
[ ] stress produces no unexplained IOMMU faults or memory corruption
[ ] security and support evidence avoids leaking protected contents
```

## Further Reading

- [Linux reserved-memory binding directory](https://www.kernel.org/doc/Documentation/devicetree/bindings/reserved-memory/)
- [Linux remoteproc framework](https://docs.kernel.org/staging/remoteproc.html)
- [Addressing And Bus Modeling](../addressing-and-bus-modeling.md)
- [Memory, Firmware, And Heterogeneous SoCs](../memory-firmware-and-heterogeneous-socs.md)
- [Board Revisions, Variants, Overlays, And Identity](board-revisions-variants-overlays-and-identity.md)

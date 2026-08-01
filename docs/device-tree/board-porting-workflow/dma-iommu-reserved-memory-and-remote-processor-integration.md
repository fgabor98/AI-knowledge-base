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

Distinguish:

```text
CPU physical address
bus/DMA address seen by a device
I/O virtual address translated by an IOMMU
remote-processor device address
firmware load address
DT parent/child bus address translated by ranges/dma-ranges
```

The same numeric value can mean different spaces. Label addresses in design documents and logs.

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

## Understand Reservation Forms

Device Tree can reserve memory through mechanisms such as the FDT reservation map and `/reserved-memory` child nodes. Exact binding semantics determine whether regions are statically addressed, dynamically allocated, reusable, or excluded from normal mapping.

Schematic example:

```dts
/ {
    reserved-memory {
        #address-cells = <2>;
        #size-cells = <2>;
        ranges;

        rproc_fw: remoteproc@90000000 {
            reg = <0x0 0x90000000 0x0 0x01000000>;
            no-map;
        };

        rproc_vring: vring@91000000 {
            reg = <0x0 0x91000000 0x0 0x00800000>;
            no-map;
        };
    };
};

&dsp0 {
    memory-region = <&rproc_fw>, <&rproc_vring>;
    firmware-name = "acme/axc300-dsp.bin";

    status = "okay";
};
```

The consumer binding defines region count/order/names and whether `no-map` is appropriate. Do not generalize this schematic example to another remoteproc driver.

## Reconcile Firmware And DT Sources

Memory may be determined by:

- static DTS
- bootloader or trusted-firmware fixup
- firmware resource table
- remoteproc driver allocation
- CMA/shared pool
- hypervisor assignment

Choose one authority per fact or define an explicit negotiation. A firmware resource table requesting one carveout while DT points to another is not redundancy; it is a conflict.

## Validate DMA Reachability

For each DMA master record:

- address width and mask
- coherent/non-coherent behavior
- bus `dma-ranges` translations
- IOMMU presence and stream/device IDs
- reserved/shared pool constraints
- alignment/segment/boundary limits
- secure versus non-secure access
- cache maintenance owner

Test buffers below and above meaningful boundaries, not only the first allocation that succeeds.

## Bring Up An IOMMU Relationship

```text
consumer iommus property
  -> provider phandle
  -> provider #iommu-cells interpretation
  -> stream/device ID from SoC interconnect documentation
  -> IOMMU provider device/driver
  -> domain attachment
  -> mappings and fault behavior
```

Never copy a stream ID from the reference board when routing, package, or hardware revision can change it. Validate that an intentional invalid access produces the expected contained fault in a safe lab test where supported.

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

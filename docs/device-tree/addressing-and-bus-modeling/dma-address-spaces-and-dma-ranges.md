---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# DMA Address Spaces And `dma-ranges`

CPU address translation and DMA address translation describe different transaction origins. `ranges` answers where the CPU reaches a child bus; `dma-ranges` answers how a DMA address originating below a bus corresponds to its parent's address space.

## Keep Address Roles Separate

For one buffer, at least three values may exist:

| View | Used by | Example |
|---|---|---:|
| CPU virtual address | kernel code | `ffff...` |
| CPU physical address | memory system | `0x88000000` |
| DMA address / IOVA | device descriptor | `0x08000000` |

The DMA API supplies the device-visible value. A driver must not derive it from a CPU virtual or physical address, even on a platform where the numbers happen to match.

## `dma-ranges` Entry Shape

Like `ranges`, each entry contains:

```text
child-bus DMA address + parent-bus address + length
```

The widths are:

| Field | Width source |
|---|---|
| child DMA address | bus node's `#address-cells` |
| parent address | bus node's parent's `#address-cells` |
| length | bus node's `#size-cells` |

Example:

```dts
/ {
        #address-cells = <2>;
        #size-cells = <2>;

        soc {
                #address-cells = <1>;
                #size-cells = <1>;
                ranges = <0x0 0x0 0x40000000 0x01000000>;
                dma-ranges = <0x00000000 0x00000000 0x80000000 0x10000000>;
        };
};
```

The MMIO window maps SoC child address zero to CPU physical `0x40000000`. Independently, DMA from devices below `soc` using address `[0, 0x10000000)` reaches parent memory `[0x80000000, 0x90000000)`. A buffer at CPU physical `0x88000000` is therefore visible at DMA address `0x08000000`, assuming no additional IOMMU translation.

This is not the direction of a CPU MMIO access. Read it from the initiating device outward toward memory.

## Empty And Absent DMA Ranges

An empty `dma-ranges;` represents identity between child and parent DMA address spaces. An absent property may be handled according to architecture, bus binding, and Linux firmware rules; do not rely on absence when the hardware has a real offset or aperture.

Explicit mappings are especially important when:

- a peripheral bus sees only a low-memory aperture
- inbound and outbound bridge windows differ
- a legacy device has fewer address bits than the CPU
- RAM appears at a device-specific offset
- firmware configures a remapping unit before boot

## DMA Masks And Reachability

`dma-ranges` describes bus translation, but the device can impose a narrower DMA mask. Linux combines firmware description, bus constraints, device masks, and possibly IOMMU capabilities when deciding whether an address is reachable.

A correct driver sets an appropriate mask through the DMA API and handles failure. Typical failure modes include:

- assuming 64-bit DMA because the CPU is 64-bit
- allocating a buffer outside the device aperture
- truncating a returned `dma_addr_t`
- programming the CPU physical address instead of the DMA address
- using a mask as a substitute for a missing bus translation

Bounce buffering may make an otherwise unreachable transfer work, but it is not proof that the Device Tree is correct and may have significant performance cost.

## IOMMU Translation

An IOMMU adds another mapping stage:

```text
device DMA / IOVA
        ↓ IOMMU page tables and requester identity
bus/parent physical address
        ↓ memory interconnect
RAM
```

`iommus` or an `iommu-map` associates requesters with an IOMMU, while `dma-ranges` describes address apertures across bus boundaries. They are complementary. Do not encode an IOMMU's dynamic IOVA allocation as a fixed `dma-ranges` window.

Security review must also establish whether every requester path is translated, whether firmware leaves bypass enabled, and which devices share an IOMMU group. A phandle is a description, not proof of isolation.

## Coherency Is Another Axis

`dma-coherent` says DMA transactions participate in the relevant cache-coherency domain. It does not define the DMA address, expand the device mask, or install IOMMU mappings. Conversely, a non-coherent device can use an IOMMU.

Drivers must use the DMA API in both cases. The API handles architecture-required mapping, synchronization, and address translation; coherent allocation does not make arbitrary stack or static memory suitable for DMA.

## Diagnose From Both Ends

When DMA corrupts memory or times out:

1. Record the CPU virtual, CPU physical if legitimately observable, and DMA addresses separately.
2. Decode each `dma-ranges` boundary from the final DTB.
3. Check the device DMA mask and hardware address-register width.
4. Inspect IOMMU attachment, groups, faults, and bypass state.
5. Confirm cache-coherency description and DMA API usage.
6. Check reserved-memory/CMA placement and whether buffers fall inside the reachable aperture.
7. Compare bridge window registers with Device Tree assumptions.

Useful Linux evidence varies by subsystem, but often includes boot logs, IOMMU fault logs, debugfs, tracing, and driver-specific descriptor dumps. Avoid logging buffer contents or addresses in production when that would weaken security.

## Senior Review Checklist

- Are CPU and DMA windows derived from hardware documentation independently?
- Can every supported RAM configuration be reached by each DMA master?
- Are 32-bit-only masters forced into a valid allocation zone?
- Does firmware own inbound bridge-window programming, and is it stable across upgrades?
- Are IOMMU requester IDs complete for multifunction and aliased devices?
- Are coherency, ordering, and ownership transitions tested under load?
- Does failure testing cover the aperture boundary and addresses above 4 GiB?

## Authoritative References

- [Devicetree Specification: `dma-ranges`](https://devicetree-specification.readthedocs.io/en/stable/devicetree-basics.html)
- [Linux DMA API HOWTO](https://docs.kernel.org/core-api/dma-api-howto.html)
- [Linux DMA API](https://docs.kernel.org/core-api/dma-api.html)
- [Linux DeviceTree DMA translation API](https://docs.kernel.org/devicetree/kernel-api.html)

## Next Step

Continue with [Interrupt Parents, Specifiers, And Routing](interrupt-parents-specifiers-and-routing.md).

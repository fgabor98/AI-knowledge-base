---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# DMA Addressing, Coherency, And IOMMU Topology

The CPU address printed in a linker map may not be the number a peripheral or remote core uses. Correct integration follows each bus master's path through bus offsets, interconnect windows, IOMMUs, and cache-coherency fabric.

## Name Every Address Space

Use distinct vocabulary:

| Address | Used by | Translation |
|---|---|---|
| CPU virtual address | Linux software | CPU page tables |
| CPU physical address | CPU memory system | physical interconnect |
| DMA/bus address | DMA master | host bridge/interconnect and possibly IOMMU |
| IOVA | IOMMU-facing device | IOMMU page tables |
| remote device address | remote firmware/processor | remote MMU or SoC windows |

Two spaces can be numerically identical on one board and different on another. Never cast a CPU pointer or `phys_addr_t` to a `dma_addr_t`; drivers must use the DMA API.

## `dma-ranges` Describes A Bus Translation

Decode `dma-ranges` with the relevant bus cell counts as shown in [DMA Address Spaces And `dma-ranges`](../addressing-and-bus-modeling/dma-address-spaces-and-dma-ranges.md). It does not create an IOMMU mapping and does not guarantee the device implements enough address bits for the translated result.

## DMA Masks Are Device Capability

The device's negotiated DMA mask can be narrower than the bus translation ([DMA Masks And Reachability](../addressing-and-bus-modeling/dma-address-spaces-and-dma-ranges.md#dma-masks-and-reachability)). Even with an IOMMU, the IOVA aperture, segment limits, alignment, and boundary restrictions still matter.

Review:

- implemented address width
- coherent versus streaming mask differences
- maximum segment size and count
- boundary rules
- accessible memory windows
- IOMMU aperture and reserved IOVA regions
- firmware address-field width

A DTS property should not claim a wider capability than the hardware merely to make allocation succeed.

## Coherency Is Not Contiguity

`dma-coherent` describes a device whose DMA participates in the platform's coherent view according to the binding and architecture. `dma-noncoherent` can explicitly override an inherited coherent default where allowed.

Coherency does not mean:

- physically contiguous
- ordered without memory barriers
- protected from another master
- visible to a remote core with unrelated caches
- safe after ownership changes

Coherent mappings still need correct producer/consumer ordering. Streaming mappings require the DMA API's map, unmap, and sync lifecycle. Shared memory with a remote core additionally requires an agreed cache policy and protocol.

## IOMMU Attachment

`iommus` and bus-level `iommu-map`/`iommu-map-mask` are introduced in [DMA, IOMMU, PHY, And Regulator Dependencies](../provider-consumer-relationships/dma-iommu-phy-and-regulator-dependencies.md#iommu-relationships); evaluate `iommu-map` against requester-ID routing rather than copying a peripheral example. An IOMMU protects memory only while correctly configured, does not stop masters that bypass it, and cannot repair firmware using an address from the wrong domain.

## Remoteproc Address Translation

Remoteproc firmware ELF segments and resource-table entries commonly use device addresses. The platform driver translates those to host-accessible addresses or installs IOMMU mappings. Ask for each region:

```text
ELF device address
  -> remote MMU/interconnect view
  -> IOVA or bus address, if any
  -> CPU physical backing
  -> host mapping used for loading/debug
```

The same numeric value in DT `reg`, an ELF program header, and firmware C code is meaningful only if the platform contract says the address spaces coincide.

## A Translation Worksheet

For every master, complete:

| Master | Address bits | Coherent? | IOMMU/offset | Reachable RAM | API owner |
|---|---:|---:|---|---|---|
| application CPU | platform | n/a | CPU MMU | OS map | Linux MM |
| R5 core | 32 | platform-specific | local RAT/MMU | windows | remoteproc + firmware |
| DMA engine | 32/40/... | yes/no | system IOMMU | aperture | DMA API |

Then walk one real buffer both directions. Include cache maintenance and ownership events, not only arithmetic.

## Runtime Diagnosis

```sh
dmesg | grep -Ei 'iommu|smmu|dma|coherent|fault'
cat /sys/kernel/debug/iommu/* 2>/dev/null
cat /proc/iomem
```

Debugfs layout varies by IOMMU driver and configuration. For a fault, capture requester/stream ID, IOVA, access type, page-table context, and active owner before restarting anything. Correlate those with the device's negotiated DMA mask and the address programmed into hardware.

Data corruption that vanishes when caches are disabled is evidence of a coherency or ownership defect, not a valid production fix.

## Authoritative References

- [Linux DMA API guide](https://docs.kernel.org/core-api/dma-api-howto.html)
- [Linux DMA API reference](https://docs.kernel.org/core-api/dma-api.html)
- [Linux generic IOMMU binding](https://github.com/torvalds/linux/blob/master/Documentation/devicetree/bindings/iommu/iommu.txt)
- [Devicetree Specification: `dma-ranges`](https://devicetree-specification.readthedocs.io/en/stable/devicetree-basics.html#dma-ranges)
- [Linux Devicetree kernel API](https://docs.kernel.org/devicetree/kernel-api.html)

## Continue

Proceed to [Firmware Images, Resource Tables, And Host Contracts](firmware-images-resource-tables-and-host-contracts.md).

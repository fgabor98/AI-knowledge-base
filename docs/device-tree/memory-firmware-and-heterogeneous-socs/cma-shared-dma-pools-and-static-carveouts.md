---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# CMA, Shared DMA Pools, And Static Carveouts

Contiguous memory can be permanently removed from Linux, loaned to the page allocator until needed, or managed as a DMA pool. These choices have different fragmentation, latency, isolation, and lifecycle properties even when their DTS nodes look similar.

## Choose By Allocation Contract

| Mechanism | Placement | Linux may use while idle | Typical use |
|---|---|---:|---|
| static carveout | fixed `reg` | no | firmware link address, hardware window |
| dynamic reservation | early allocation from `size` | depends on compatible/properties | placement-flexible dedicated region |
| shared DMA pool | fixed or dynamic | binding-dependent | device DMA allocations |
| CMA region | reserved early, movable pages while idle | yes | large contiguous allocations |
| ordinary DMA API | runtime | yes | normal coherent or streaming buffers |

Use the least restrictive mechanism that satisfies hardware and ABI constraints. Permanent carveouts reduce available RAM even when their device is unused. CMA improves utilization but cannot replace security isolation or guarantee every latency bound.

## Shared DMA Pool

The generic compatible is `shared-dma-pool`:

```dts
reserved-memory {
        #address-cells = <2>;
        #size-cells = <2>;
        ranges;

        video_pool: dma-pool@98000000 {
                compatible = "shared-dma-pool";
                reg = <0x0 0x98000000 0x0 0x04000000>;
                reusable;
        };
};

video@50000000 {
        memory-region = <&video_pool>;
};
```

The consumer binding and driver determine how the region is attached and allocated. “Shared” means a pool can serve compatible consumers; it does not automatically define concurrency, cache maintenance, buffer ownership, or a wire protocol.

`reusable` means the OS may use the region for other purposes while being able to reclaim it for the owning driver. It is incompatible with a contract where firmware can write at arbitrary times. A CPU page loaned elsewhere cannot safely remain remotely accessible.

## CMA

A `shared-dma-pool` node marked `linux,cma-default` can select the default CMA area:

```dts
linux_cma: linux,cma {
        compatible = "shared-dma-pool";
        reusable;
        size = <0x0 0x10000000>;
        alignment = <0x0 0x00200000>;
        linux,cma-default;
};
```

CMA reserves a physical range early and normally permits movable pages to occupy it until contiguous allocation requires migration. Allocation can still fail because of unmovable pages, pinning, sizing, alignment, or pressure. “Reserved at boot” therefore does not mean “always immediately available as one contiguous block.”

Treat command-line CMA settings, DT default CMA, and architecture defaults as competing configuration sources. Confirm the chosen area in the boot log.

## `linux,dma-default`

The shared DMA pool binding also defines `linux,dma-default` for the default coherent DMA pool on relevant Linux platforms. It is not another spelling of CMA. Read the binding and architecture behavior before using it, and avoid Linux-specific properties when a device-specific `memory-region` expresses the contract more accurately.

## Static Firmware Carveouts

Remote firmware frequently has fixed load addresses:

```dts
r5f_code: r5f-code@9d000000 {
        reg = <0x0 0x9d000000 0x0 0x00600000>;
        no-map;
};

r5f_vring: r5f-vring@9d600000 {
        reg = <0x0 0x9d600000 0x0 0x00010000>;
        no-map;
};
```

Do not derive sizes only from the current binary. Include alignment, vrings, trace, heaps, guard space, future growth, and any platform granularity. Conversely, an oversized carveout can conceal a firmware overrun until a later product variant places something adjacent.

Add build-time or deployment checks that compare:

- reserved region bases and ends
- ELF program-header load addresses and sizes
- resource-table carveouts and vrings
- linker-script symbols
- remote MMU mappings
- firewall or MPU windows

## One Large Region Or Several

One large region simplifies placement but weakens diagnosis and access control. Separate regions can distinguish:

- executable firmware text/data
- vrings
- RPMsg buffer pool
- crash dump
- trace
- application shared data

Choose according to the granularity of the consumer binding, IOMMU/MPU/firewall, cache attributes, and ownership transitions. Splitting DTS nodes without corresponding hardware or driver isolation provides organization, not enforcement.

## Capacity Engineering

For a pool, budget the worst credible simultaneous use:

```text
pool requirement =
    queued payload buffers
  + descriptor/vring storage
  + alignment and allocator overhead
  + in-flight buffers retained during recovery
  + trace/crash reserve
  + growth margin
```

Measure high-water marks under concurrent workloads. A camera, display, codec, and auxiliary core can each pass isolated tests while exhausting a shared CMA region together.

## Failure Patterns

### Allocation Works After Boot But Fails Later

Investigate CMA migration failures, long-term page pins, fragmentation, and changing concurrent demand. Increasing the region may hide rather than resolve unbounded ownership.

### Remote Core Corrupts Linux Pages

A `reusable` pool was exposed to a core that can access it while Linux considers pages reclaimable, or firmware exceeded its assigned range. Stop the core first; preserve evidence before retrying.

### Firmware Loads But Vrings Fail

The code carveout fits, but resource-table vring alignment, count, or device addresses disagree with DT and the remote view.

### Product RAM Appears Smaller

Account for every static reservation and boot-firmware exclusion. A large fixed carveout may have been copied from an evaluation kit with a different workload.

## Authoritative References

- [Upstream shared DMA pool binding schema](https://github.com/devicetree-org/dt-schema/blob/main/dtschema/schemas/reserved-memory/shared-dma-pool.yaml)
- [Linux reserved-memory binding](https://github.com/torvalds/linux/blob/master/Documentation/devicetree/bindings/reserved-memory/reserved-memory.txt)
- [Linux contiguous memory allocator source documentation](https://github.com/torvalds/linux/blob/master/kernel/dma/contiguous.c)
- [Linux DMA API guide](https://docs.kernel.org/core-api/dma-api-howto.html)

## Continue

Proceed to [DMA Addressing, Coherency, And IOMMU Topology](dma-addressing-coherency-and-iommu-topology.md).

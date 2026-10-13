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

The generic compatible is `shared-dma-pool`. The kernel picks one of two setups from the node's flags. `rmem_dma_setup()` in `kernel/dma/coherent.c` handles a non-`reusable` region as a per-device coherent pool. `rmem_cma_setup()` in `kernel/dma/contiguous.c` handles a `reusable` region without `no-map` as CMA.

The AM64x R5F vring pools are the first kind. From `k3-am642-sk.dts`:

```dts
main_r5fss0_core0_dma_memory_region: memory@a0000000 {
        compatible = "shared-dma-pool";
        reg = <0x00 0xa0000000 0x00 0x100000>;
        no-map;
};
```

The R5F remoteproc driver attaches it as index 0 of `memory-region`, and vring and RPMsg buffer allocations for that device come from this 1 MiB. “Shared” means the pool can serve DMA allocations for its consumers; it does not define concurrency, cache maintenance, buffer ownership, or a wire protocol.

`reusable` means the OS may use the region for other purposes and reclaim it for the owning driver. It is incompatible with firmware that can write at arbitrary times: a CPU page loaned elsewhere cannot safely remain remotely accessible. That is why the R5F pool is `no-map` and not `reusable`.

## CMA

BeaglePlay and the AM62x SK both declare a dynamically placed default CMA area:

```dts
/* global cma region */
linux,cma {
        compatible = "shared-dma-pool";
        reusable;
        size = <0x00 0x8000000>;
        linux,cma-default;
};
```

There is no `reg`, so the kernel places the 128 MiB itself. CMA reserves a physical range early and lets movable pages occupy it until a contiguous allocation needs them migrated away. Allocation can still fail because of unmovable pages, pinning, sizing, alignment, or pressure. “Reserved at boot” does not mean “always available as one contiguous block.”

Configuration sources compete. If the kernel command line sets `cma=`, `rmem_cma_setup()` logs `Reserved memory: bypass linux,cma node, using cmdline CMA params instead` and ignores this node. A CMA region whose base or size is not aligned to the CMA minimum is rejected with `incorrect alignment of CMA region`. Confirm the chosen area in the boot log: `Reserved memory: created CMA memory pool at ...`.

## `linux,dma-default`

The shared DMA pool binding also defines `linux,dma-default` for the default coherent DMA pool on relevant Linux platforms. It is not another spelling of CMA. Read the binding and architecture behavior before using it, and avoid Linux-specific properties when a device-specific `memory-region` expresses the contract more accurately.

## Static Firmware Carveouts

Remote firmware frequently has fixed load addresses. Each AM64x remote core gets a 15 MiB carveout right after its 1 MiB DMA pool (`k3-am64-ti-ipc-firmware.dtsi`):

```dts
main_r5fss0_core1_dma_memory_region: memory@a1000000 {
        compatible = "shared-dma-pool";
        reg = <0x00 0xa1000000 0x00 0x100000>;
        no-map;
};

main_r5fss0_core1_memory_region: memory@a1100000 {
        compatible = "shared-dma-pool";
        reg = <0x00 0xa1100000 0x00 0xf00000>;
        no-map;
};
```

The carveout also carries `compatible = "shared-dma-pool"`, but the R5F driver never uses it as a pool: it maps every region after index 0 with `devm_ioremap_resource_wc()` as a static carveout. The compatible string does not decide the role; the consumer does. The TI SDK firmware is linked to these addresses, and the resource table must sit at the carveout's base.

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

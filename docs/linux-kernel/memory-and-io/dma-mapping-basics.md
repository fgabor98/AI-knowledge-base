---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# DMA Mapping Basics

## What Problem Does This Solve?

DMA mapping APIs make CPU memory visible to devices and maintain ownership and cache-coherency rules.

DMA is not "give hardware a pointer." It is a contract between:

- CPU virtual memory
- physical memory
- the device bus master
- cache coherency rules
- the IOMMU, if present
- the driver that controls ownership transitions

If the driver skips the DMA API, a transfer may appear to work on one platform and corrupt data on another.

## Core Concepts

- `dma_alloc_coherent`
- streaming DMA mappings
- `dma_map_single`
- `dma_unmap_single`
- DMA direction
- cache coherency
- scatter-gather lists
- DMA mask
- IOMMU

## Mental Model

The CPU and device must not modify the same buffer at the same time unless the mapping contract allows it. Mapping and unmapping mark ownership transitions.

```text
CPU owns buffer
-> driver maps or syncs for device
-> device owns buffer while DMA is active
-> interrupt/completion reports transfer done
-> driver unmaps or syncs for CPU
-> CPU may read/write according to direction
```

The CPU pointer and DMA address are different objects.

```c
void *cpu_ptr;        /* CPU uses this */
dma_addr_t dma_addr; /* device uses this */
```

Never program hardware with `cpu_ptr`.

## Address Types

| Type | Meaning | Who Uses It |
| --- | --- | --- |
| `void *` | kernel virtual address | CPU |
| `dma_addr_t` | DMA/bus/IOMMU address | device |
| `phys_addr_t` | physical address | low-level platform/resource code |
| `struct page *` | kernel page object | MM/page APIs |
| `void __iomem *` | MMIO register mapping | CPU through I/O accessors |

On systems with an IOMMU, a `dma_addr_t` may be an I/O virtual address, not a physical address. Drivers must treat it as opaque and use only the DMA API.

## DMA Mask

Before allocating or mapping DMA memory, set the address range the device can access.

```c
ret = dma_set_mask_and_coherent(dev, DMA_BIT_MASK(32));
if (ret)
    return dev_err_probe(dev, ret, "no suitable DMA mask\n");
```

Use the hardware limit:

```text
24-bit DMA engine -> DMA_BIT_MASK(24)
32-bit DMA engine -> DMA_BIT_MASK(32)
64-bit capable device -> DMA_BIT_MASK(64), if hardware really supports it
```

If a 32-bit device receives an address above 4 GiB, it may wrap, hang, or corrupt memory.

Some devices have different addressing limits for streaming DMA and coherent allocations. If the driver needs that distinction, set both masks explicitly according to the hardware manual.

## Coherent DMA Memory

Use coherent DMA memory when CPU and device share a buffer over a longer time.

Common examples:

- descriptor rings
- command blocks
- status pages
- small buffers that hardware reads or updates repeatedly

Allocate:

```c
struct demo_ring {
    void *cpu;
    dma_addr_t dma;
    size_t size;
};

ring->size = sizeof(struct demo_desc) * DEMO_RING_SIZE;
ring->cpu = dma_alloc_coherent(dev, ring->size, &ring->dma,
                               GFP_KERNEL);
if (!ring->cpu)
    return -ENOMEM;
```

Managed allocation:

```c
priv->desc = dmam_alloc_coherent(dev, desc_size,
                                 &priv->desc_dma, GFP_KERNEL);
if (!priv->desc)
    return -ENOMEM;
```

Free manual allocation:

```c
dma_free_coherent(dev, ring->size, ring->cpu, ring->dma);
```

Coherent means CPU and device see each other's data without explicit cache sync operations. It does not remove all ordering requirements. If the CPU fills descriptors and then rings a doorbell, use the ordering required by the device:

```c
desc->addr = cpu_to_le64(buf_dma);
desc->len = cpu_to_le32(len);
desc->flags = cpu_to_le32(DEMO_DESC_OWN);
dma_wmb();
writel(DEMO_DOORBELL_TX, priv->regs + DEMO_REG_DOORBELL);
```

## Streaming DMA Mappings

Use streaming mappings for temporary transfers.

Transmit example:

```c
dma_addr_t dma;

memcpy(buf, packet, len);

dma = dma_map_single(dev, buf, len, DMA_TO_DEVICE);
if (dma_mapping_error(dev, dma))
    return -EIO;

demo_program_tx(priv, dma, len);
```

After completion:

```c
dma_unmap_single(dev, dma, len, DMA_TO_DEVICE);
```

Receive example:

```c
dma = dma_map_single(dev, buf, len, DMA_FROM_DEVICE);
if (dma_mapping_error(dev, dma))
    return -EIO;

demo_program_rx(priv, dma, len);
```

After the device finishes:

```c
dma_unmap_single(dev, dma, len, DMA_FROM_DEVICE);
demo_consume_rx_data(buf, len);
```

Between map and unmap, the device owns the buffer for that direction. Do not read or write it from the CPU unless you use the appropriate sync API.

## DMA Directions

| Direction | Meaning | CPU Rule |
| --- | --- | --- |
| `DMA_TO_DEVICE` | CPU prepared data; device reads it | CPU writes before map/sync-for-device |
| `DMA_FROM_DEVICE` | device writes data; CPU reads after completion | CPU reads after unmap/sync-for-CPU |
| `DMA_BIDIRECTIONAL` | both sides may need access phases | use only when truly needed |
| `DMA_NONE` | invalid/unmapped marker in some structures | not for active transfers |

Do not use `DMA_BIDIRECTIONAL` to avoid thinking. Direction tells the DMA API which cache maintenance and ownership operations are required.

## Syncing Active Streaming Mappings

Sometimes a streaming mapping stays active while ownership alternates.

CPU prepares data for device:

```c
memcpy(buf, data, len);
dma_sync_single_for_device(dev, dma, len, DMA_TO_DEVICE);
demo_start_dma(priv, dma, len);
```

Device writes data and CPU wants to inspect it without unmapping:

```c
dma_sync_single_for_cpu(dev, dma, len, DMA_FROM_DEVICE);
demo_check_result(buf, len);
```

If the device will use the buffer again after CPU inspection, sync it back for the device:

```c
dma_sync_single_for_device(dev, dma, len, DMA_FROM_DEVICE);
```

Use sync APIs only for mappings that remain active. If the transfer is complete and the mapping is no longer needed, unmap.

## Coherent Versus Streaming

| Need | Prefer |
| --- | --- |
| descriptor ring shared for device lifetime | coherent DMA |
| one packet transmit | streaming mapping |
| one receive buffer reused with ownership flips | streaming mapping plus sync |
| small status block continuously updated by device | coherent DMA |
| large data path buffers | streaming mappings or subsystem buffer APIs |
| userspace-visible DMA buffers | subsystem-specific APIs or `dma_mmap_coherent()` where appropriate |

Coherent memory can be convenient but is not automatically faster. Streaming mappings often fit high-throughput data buffers better.

## Mapping Errors

Always check mapping errors:

```c
dma = dma_map_single(dev, buf, len, DMA_TO_DEVICE);
if (dma_mapping_error(dev, dma))
    return -EIO;
```

Scatter-gather mappings return a count:

```c
mapped = dma_map_sg(dev, sgl, nents, DMA_TO_DEVICE);
if (mapped == 0)
    return -EIO;
```

Do not program hardware with a mapping that failed.

## Map/Unmap Pairing

Every successful streaming mapping needs a matching unmap on all paths.

```text
map succeeds
-> hardware start succeeds
-> completion path unmaps

map succeeds
-> hardware start fails
-> error path unmaps

map succeeds
-> timeout
-> terminate hardware safely
-> unmap after device no longer owns buffer
```

Timeout handling is a common leak or corruption path. Do not unmap while the device may still be writing to the buffer.

## DMA And CPU Cache Coherency

On coherent systems, hardware keeps CPU and device caches coherent.

On non-coherent systems, the DMA API performs cache maintenance when used correctly.

This is why the same broken driver can work on x86 and fail on ARM:

```text
broken driver skips sync/unmap
x86 coherent cache hides the bug
non-coherent SoC exposes stale or dirty data
```

Write drivers as if they must work on non-coherent systems unless the subsystem explicitly says otherwise.

## IOMMU Effects

With an IOMMU:

- DMA addresses can be remapped
- physically non-contiguous memory may appear contiguous to the device in some cases
- device access may be isolated
- mapping can fail due to IOMMU resource limits

Driver rules do not change:

```text
use DMA API addresses
do not infer physical addresses
check mapping errors
unmap what you map
```

## DMA From `vmalloc()` Or Stack Memory

Do not use stack memory for DMA.

Wrong:

```c
u8 cmd[32];

dma = dma_map_single(dev, cmd, sizeof(cmd), DMA_TO_DEVICE);
```

The stack is not a suitable DMA buffer.

Do not treat `vmalloc()` memory as a single physically contiguous buffer. If a subsystem supports vmalloc-backed DMA through a specific API, follow that subsystem's rules. Otherwise use pages/scatter-gather or another supported allocation model.

## Reserved Memory And Coherent Pools

Embedded platforms may describe reserved memory for DMA in Device Tree.

```dts
reserved-memory {
    dma_pool: buffer@90000000 {
        compatible = "shared-dma-pool";
        reusable;
        reg = <0x0 0x90000000 0x0 0x1000000>;
    };
};

device@... {
    memory-region = <&dma_pool>;
};
```

This is a platform integration choice, not a generic replacement for the DMA API. The driver should still use the appropriate DMA/subsystem APIs.

## DMA And Userspace

Do not DMA directly from a raw userspace pointer.

Wrong:

```c
dma = dma_map_single(dev, user_ptr, len, DMA_TO_DEVICE);
```

Userspace pointers are not kernel DMA buffers.

Options depend on subsystem and requirements:

- copy data into a kernel buffer and map that
- use subsystem buffer management such as V4L2, DRM, ALSA, or IIO
- pin user pages only when the subsystem and lifetime rules justify it
- expose coherent buffers through a controlled mmap ABI when appropriate

Pinning userspace pages is advanced work with memory-management side effects. Do not start there for beginner drivers.

## DMA Debugging

Enable DMA API debugging in development kernels where practical:

```text
CONFIG_DMA_API_DEBUG
CONFIG_DMA_API_DEBUG_SG
```

It can catch:

- missing unmaps
- wrong direction
- double unmaps
- mapping memory that should not be mapped
- incorrect scatter-gather usage

Other useful tools:

- dynamic debug around map/start/complete/unmap
- ftrace for IRQ and completion paths
- IOMMU fault logs
- hardware bus error logs
- KASAN/KCSAN for adjacent lifetime and race bugs

## Common Failure Modes

| Symptom | Likely Cause | First Checks |
| --- | --- | --- |
| corrupted data | wrong direction or missing sync | map direction and ownership |
| works on desktop, fails on SoC | non-coherent cache assumptions | DMA API usage |
| device writes wrong memory | bad DMA mask or stale DMA address | mask and map result |
| IOMMU fault | device used unmapped/wrong address | unmap timing and descriptor |
| transfer timeout | descriptor not visible or doorbell ordering wrong | `dma_wmb()` and MMIO |
| memory leak | missing unmap on error/timeout | all exit paths |
| data stale after receive | CPU read before unmap/sync | ownership transition |

## Practice Exercises

### Exercise 1: Address Audit

For a DMA driver, list every:

```text
CPU pointer
DMA address
physical resource address
MMIO pointer
```

Explain who is allowed to use each one.

### Exercise 2: Direction Audit

For each DMA transfer, write:

```text
direction
who writes before map
who reads after completion
where unmap happens
timeout path
```

### Exercise 3: Coherent Ring

Allocate a coherent descriptor ring, fill one descriptor, add the required memory barrier, and ring a doorbell register.

## Debugging Checklist

- Check DMA direction.
- Check DMA mask setup.
- Check map and unmap pairing.
- Check cache maintenance expectations on non-coherent platforms.
- Check that hardware receives `dma_addr_t`, not CPU pointers.
- Check that the CPU does not touch streaming buffers while the device owns them.
- Check IOMMU fault logs.
- Enable DMA API debugging in lab kernels.
- Check timeout paths for safe device termination before unmap.

## Related Topics

- [DMA Basics](../driver-interfaces/dma-basics.md)
- [Kernel Memory Allocation](kernel-memory-allocation.md)
- [Reserved Memory](../remoteproc-rpmsg/reserved-memory.md)
- [MMIO And Register Access](mmio-and-register-access.md)
- [Single-Buffer DMA](single-buffer-dma.md)
- [Scatter-Gather DMA](scatter-gather-dma.md)

## Official References

- [DMA API HOWTO](https://docs.kernel.org/core-api/dma-api-howto.html)
- [DMA API](https://docs.kernel.org/core-api/dma-api.html)
- [DMA attributes](https://docs.kernel.org/core-api/dma-attributes.html)

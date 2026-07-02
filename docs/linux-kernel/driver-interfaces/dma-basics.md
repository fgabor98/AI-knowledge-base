---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# DMA Basics

## What Problem Does This Solve?

DMA lets devices transfer data directly to or from memory, reducing CPU copy overhead for suitable workloads.

This page gives the driver-interface view. The deeper memory ownership rules are covered in [DMA Mapping Basics](../memory-and-io/dma-mapping-basics.md).

## Core Concepts

- DMA-capable device
- DMA controller
- DMAengine
- DMA mask
- coherent memory
- streaming mappings
- cache coherency
- ownership transfer
- DMA direction
- scatter-gather
- IOMMU
- reserved memory
- descriptor rings

## Mental Model

DMA is a contract between the CPU, memory, device, and bus/IOMMU fabric.

```text
CPU owns buffer
-> driver maps buffer for DMA
-> device owns buffer while DMA is active
-> DMA completes
-> driver unmaps or syncs
-> CPU owns buffer again
```

The driver must not let CPU and device modify the same buffer at the same time unless the mapping contract allows it.

## Direct DMA Versus DMAengine

Direct DMA:

```text
device contains its own DMA engine
driver programs device DMA registers
driver uses DMA mapping API for buffers
```

DMAengine:

```text
separate DMA controller moves data
client driver requests DMA channel
client submits descriptors through DMAengine API
```

Examples:

| Hardware | Likely Model |
| --- | --- |
| Ethernet controller | direct DMA |
| PCIe device | direct DMA |
| UART using SoC DMA controller | DMAengine |
| SPI controller using SoC DMA | controller driver may use DMAengine |
| Audio interface | often DMAengine or subsystem-managed DMA |

## DMA Mask

Set the address range the device can access:

```c
ret = dma_set_mask_and_coherent(dev, DMA_BIT_MASK(32));
if (ret)
    return dev_err_probe(dev, ret, "no suitable DMA mask\n");
```

If a 32-bit device receives a DMA address above 4 GiB, transfers fail or corrupt memory. Always match the hardware.

## Coherent Memory

Use coherent memory for descriptors or buffers shared continuously between CPU and device:

```c
priv->desc = dmam_alloc_coherent(dev, desc_size,
                                 &priv->desc_dma, GFP_KERNEL);
if (!priv->desc)
    return -ENOMEM;
```

The CPU pointer:

```c
priv->desc
```

The DMA address for the device:

```c
priv->desc_dma
```

Do not give the device the CPU virtual address.

## Streaming Mapping

For a temporary transfer:

```c
dma_addr_t dma;

dma = dma_map_single(dev, buf, len, DMA_TO_DEVICE);
if (dma_mapping_error(dev, dma))
    return -EIO;

demo_start_dma(priv, dma, len);
```

After completion:

```c
dma_unmap_single(dev, dma, len, DMA_TO_DEVICE);
```

Directions:

| Direction | Meaning |
| --- | --- |
| `DMA_TO_DEVICE` | CPU prepared data, device reads it. |
| `DMA_FROM_DEVICE` | Device writes data, CPU reads after completion. |
| `DMA_BIDIRECTIONAL` | Both directions, use only when necessary. |

## Syncing Streaming Buffers

If a streaming mapping stays active and ownership changes:

```c
dma_sync_single_for_device(dev, dma, len, DMA_TO_DEVICE);
/* device uses buffer */
dma_sync_single_for_cpu(dev, dma, len, DMA_FROM_DEVICE);
/* CPU reads buffer */
```

Use the correct direction and ownership transitions. On non-coherent platforms, this controls cache maintenance.

## Scatter-Gather

For non-contiguous buffers:

```c
ret = dma_map_sg(dev, sglist, nents, DMA_TO_DEVICE);
if (ret == 0)
    return -EIO;

/* program ret mapped entries */

dma_unmap_sg(dev, sglist, nents, DMA_TO_DEVICE);
```

The number returned by `dma_map_sg()` may differ from the original entry count because mappings can be merged.

## DMAengine Client Shape

High-level flow:

```c
chan = dma_request_chan(dev, "rx");
if (IS_ERR(chan))
    return dev_err_probe(dev, PTR_ERR(chan), "failed to get rx dma\n");
```

Prepare descriptor:

```c
desc = dmaengine_prep_slave_single(chan, dma_addr, len,
                                   DMA_DEV_TO_MEM, flags);
if (!desc)
    return -EIO;

desc->callback = demo_dma_done;
desc->callback_param = priv;

cookie = dmaengine_submit(desc);
dma_async_issue_pending(chan);
```

This is only a shape. Actual DMAengine usage depends on the controller, peripheral, and subsystem.

## Device Tree DMA Channels

Example:

```dts
dmas = <&dma 5>, <&dma 6>;
dma-names = "rx", "tx";
```

Driver:

```c
rx = dma_request_chan(dev, "rx");
tx = dma_request_chan(dev, "tx");
```

If channels are optional, handle absence explicitly and provide a PIO fallback if appropriate.

## IOMMU And Address Translation

With an IOMMU, the DMA address may be an I/O virtual address, not a physical address.

Drivers should not assume:

```text
DMA address == physical address
```

Use DMA API return values and do not hand-roll physical address conversions.

## Reserved Memory And Coherent Pools

Some embedded systems reserve memory for DMA:

```dts
reserved-memory {
    dma_pool: buffer@90000000 {
        compatible = "shared-dma-pool";
        reusable;
        reg = <0x0 0x90000000 0x0 0x1000000>;
    };
};
```

A device may reference reserved memory with:

```dts
memory-region = <&dma_pool>;
```

Use this only when the platform design requires it.

## DMA And Cache Coherency

On coherent systems, CPU and device caches are kept coherent by hardware.

On non-coherent systems, the DMA API performs necessary cache maintenance when used correctly.

Do not skip mapping/syncing because a test happened to work once. Cache bugs are often intermittent and data-dependent.

## Error Handling

Always check mapping errors:

```c
dma = dma_map_single(dev, buf, len, DMA_FROM_DEVICE);
if (dma_mapping_error(dev, dma))
    return -EIO;
```

Always unmap on all completion/error paths:

```c
dma_unmap_single(dev, dma, len, DMA_FROM_DEVICE);
```

Do not leak DMA mappings across probe failures, transfer timeouts, or remove.

## Common Failure Modes

| Symptom | Likely Cause | First Checks |
| --- | --- | --- |
| corrupted data | wrong direction or missing sync | mapping direction |
| works on x86, fails on ARM | cache coherency assumptions | DMA API usage |
| device sees wrong address | DMA mask or IOMMU issue | mask, mapped address |
| transfer hangs | descriptor/bus address wrong | hardware registers |
| memory leak | mapping not unmapped | error paths |
| SG transfer corrupt | used original nents after mapping | return from `dma_map_sg()` |

## Common Mistakes

- Passing CPU virtual addresses to hardware.
- Assuming DMA address equals physical address.
- Using `DMA_BIDIRECTIONAL` to avoid thinking about direction.
- Forgetting `dma_mapping_error()`.
- Not unmapping streaming mappings.
- CPU touching a buffer while device owns it.
- Ignoring DMA mask setup.
- Debugging DMA before verifying simple PIO path where possible.

## Practice Exercises

### Exercise 1: Identify DMA Model

For a driver, decide whether it uses direct DMA, DMAengine, subsystem-managed DMA, or no DMA.

### Exercise 2: Coherent Descriptor Ring

Allocate a small coherent descriptor ring and print CPU pointer and DMA address.

### Exercise 3: Streaming Buffer

Map a buffer for `DMA_TO_DEVICE`, check mapping errors, and unmap it on every path.

## Debugging Checklist

- Is the device actually DMA-capable?
- Is the DMA mask correct?
- Is the direction correct?
- Are mappings checked for errors?
- Are mappings unmapped on all paths?
- Are CPU/device ownership transitions respected?
- Is cache coherency handled by the DMA API?
- Does an IOMMU or reserved memory region affect addresses?
- Can the transfer be tested with a simpler non-DMA path first?

## Related Topics

- [DMA Mapping Basics](../memory-and-io/dma-mapping-basics.md)
- [Single-Buffer DMA](../memory-and-io/single-buffer-dma.md)
- [Scatter-Gather DMA](../memory-and-io/scatter-gather-dma.md)
- [Kernel Memory Allocation](../memory-and-io/kernel-memory-allocation.md)
- [Reserved Memory](../remoteproc-rpmsg/reserved-memory.md)

## Official References

- [DMA API HOWTO](https://docs.kernel.org/core-api/dma-api-howto.html)
- [DMA API](https://docs.kernel.org/core-api/dma-api.html)
- [DMAengine client documentation](https://docs.kernel.org/driver-api/dmaengine/client.html)

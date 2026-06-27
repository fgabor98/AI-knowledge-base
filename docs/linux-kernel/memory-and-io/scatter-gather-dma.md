---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# Scatter-Gather DMA

## What Problem Does This Solve?

Scatter-gather DMA lets a device transfer data across multiple memory segments without copying them into one physically contiguous buffer first.

## Core Concepts

- scatterlist
- segment
- `dma_map_sg`
- `dma_unmap_sg`
- DMA direction
- DMAengine descriptor
- cookie
- completion callback

## Mental Model

Scatter-gather describes a logical buffer as a list of physical segments the DMA API maps for the device.

## Practice Skeleton

- Build a small scatterlist.
- Map it for DMA.
- Submit a DMAengine scatter-gather transaction.
- Unmap every mapped segment on completion or failure.

## Debugging Checklist

- Check the mapped segment count.
- Check max segment size and alignment constraints.
- Check unmap pairing.
- Check cache coherency on non-coherent systems.

## Related Topics

- [DMA Mapping Basics](dma-mapping-basics.md)
- [Single-Buffer DMA](single-buffer-dma.md)
- [Kernel Memory Allocation](kernel-memory-allocation.md)

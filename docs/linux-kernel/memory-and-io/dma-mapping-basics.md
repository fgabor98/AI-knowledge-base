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

## Practice Skeleton

- Allocate a coherent buffer.
- Map and unmap a streaming buffer.
- Add error handling for mapping failures.
- Check DMA address width against hardware capability.

## Debugging Checklist

- Check DMA direction.
- Check DMA mask setup.
- Check map and unmap pairing.
- Check cache maintenance expectations on non-coherent platforms.

## Related Topics

- [DMA Basics](../driver-interfaces/dma-basics.md)
- [Kernel Memory Allocation](kernel-memory-allocation.md)
- [Reserved Memory](../remoteproc-rpmsg/reserved-memory.md)

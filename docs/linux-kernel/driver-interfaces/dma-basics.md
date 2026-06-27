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

## Core Concepts

- DMA controller
- DMA-capable device
- DMA mask
- coherent memory
- streaming mappings
- cache coherency
- scatter-gather
- DMAengine

## Mental Model

DMA is a contract between the CPU, memory, device, and IOMMU or bus fabric. The driver must map memory correctly and respect ownership transitions.

## Practice Skeleton

- Identify whether the device uses direct DMA or DMAengine.
- Allocate coherent memory for a simple descriptor ring.
- Map a streaming buffer.
- Confirm ownership transitions around device access.

## Debugging Checklist

- Check DMA masks and addressability.
- Check cache maintenance requirements.
- Confirm mappings are unmapped on all paths.
- Check IOMMU and reserved-memory interactions.

## Related Topics

- [DMA Mapping Basics](../memory-and-io/dma-mapping-basics.md)
- [Kernel Memory Allocation](../memory-and-io/kernel-memory-allocation.md)
- [Reserved Memory](../remoteproc-rpmsg/reserved-memory.md)

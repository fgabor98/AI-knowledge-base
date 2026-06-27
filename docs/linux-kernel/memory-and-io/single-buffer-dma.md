---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# Single-Buffer DMA

## What Problem Does This Solve?

Single-buffer DMA maps one contiguous buffer for a device transfer.

## Core Concepts

- streaming mapping
- DMA direction
- `dma_map_single`
- `dma_unmap_single`
- mapping error
- completion callback
- cache ownership
- DMAengine overview

## Mental Model

Map the buffer before the device uses it, do not touch it from the CPU while the device owns it, then unmap or synchronize before reading results.

## Practice Skeleton

- Allocate a test buffer.
- Map it for device access.
- Submit a memory-to-memory DMA transaction where supported.
- Wait for completion and verify the destination buffer.

## Debugging Checklist

- Check mapping errors.
- Check DMA direction.
- Check callback completion.
- Check CPU/device ownership transitions.

## Related Topics

- [DMA Mapping Basics](dma-mapping-basics.md)
- [DMA Basics](../driver-interfaces/dma-basics.md)
- [Wait Queues And Completions](../execution-and-concurrency/wait-queues-and-completions.md)

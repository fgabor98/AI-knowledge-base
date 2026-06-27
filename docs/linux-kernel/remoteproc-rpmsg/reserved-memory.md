---
status: draft
reviewed: false
domain: linux-kernel
difficulty: advanced
last_reviewed: null
---

# Reserved Memory

## What Problem Does This Solve?

Reserved memory prevents Linux from using memory regions needed by firmware, DMA, shared buffers, or remote cores.

## Core Concepts

- `/reserved-memory`
- carveouts
- CMA
- shared memory
- no-map
- DMA pools
- remoteproc memory
- address alignment

## Mental Model

Reserved memory is a board-level memory contract. Linux and remote firmware must agree on addresses, sizes, caching, and ownership.

## Practice Skeleton

- Inspect reserved-memory nodes.
- Map remoteproc carveouts to firmware expectations.
- Confirm Linux excludes no-map regions.
- Test overlap detection through review.

## Debugging Checklist

- Check address ranges against RAM.
- Check alignment and size.
- Check cacheability assumptions.
- Check bootloader memory reservations.

## Related Topics

- [Device Tree](../../device-tree/index.md)
- [DMA Mapping Basics](../memory-and-io/dma-mapping-basics.md)
- [Remoteproc Framework](remoteproc-framework.md)

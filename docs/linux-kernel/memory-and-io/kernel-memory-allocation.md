---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# Kernel Memory Allocation

## What Problem Does This Solve?

Drivers allocate kernel memory for state, buffers, descriptors, and data structures while respecting context and reclaim constraints.

## Core Concepts

- `kmalloc`
- `kzalloc`
- `devm_kzalloc`
- `kcalloc`
- `vmalloc`
- slab caches
- `GFP_KERNEL`
- `GFP_ATOMIC`
- zeroing

## Mental Model

Allocation choice depends on size, lifetime, alignment, contiguity, and whether the current context may sleep.

## Practice Skeleton

- Allocate per-device state with `devm_kzalloc`.
- Allocate a small dynamic buffer with `kmalloc`.
- Replace open-coded multiplication with `kcalloc`.
- Audit allocation flags by context.

## Debugging Checklist

- Check NULL returns.
- Check overflow-safe allocation helpers.
- Check context before using `GFP_KERNEL`.
- Use KASAN and kmemleak where available.

## Related Topics

- [Resource Lookup And Managed Allocation](../fundamentals/resource-lookup-and-devm.md)
- [Sleepable Vs Atomic Code](../execution-and-concurrency/sleepable-vs-atomic-code.md)
- [DMA Mapping Basics](dma-mapping-basics.md)

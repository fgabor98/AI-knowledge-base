---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# Kernel Virtual Memory And VMAs

## What Problem Does This Solve?

Some drivers need virtually contiguous memory, mappings into userspace, or awareness of process virtual memory areas.

## Core Concepts

- `vmalloc`
- virtually contiguous memory
- physical contiguity
- VMA
- `mmap`
- page mapping overview
- page faults overview
- cache attributes

## Mental Model

`kmalloc` memory is physically contiguous for small allocations. `vmalloc` memory is virtually contiguous. Userspace mappings involve VMAs and page-level ownership rules.

## Practice Skeleton

- Allocate memory with `vmalloc`.
- Compare it with `kmalloc` for contiguity assumptions.
- Inspect a VMA list in a lab module.
- Sketch what an `mmap` path would need to validate.

## Debugging Checklist

- Do not DMA from arbitrary `vmalloc` memory.
- Check mapping lifetime.
- Check page alignment.
- Check cacheability and access permissions.

## Related Topics

- [Kernel Memory Allocation](kernel-memory-allocation.md)
- [DMA Mapping Basics](dma-mapping-basics.md)
- [Userspace Copy And ioctl ABI](userspace-copy-and-ioctl-abi.md)

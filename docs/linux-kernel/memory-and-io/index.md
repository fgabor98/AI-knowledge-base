---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# Kernel Memory And I/O

This track covers how drivers allocate memory, access registers, interact with userspace memory, and prepare buffers for devices.

## Learning Materials

1. [Kernel Memory Allocation](kernel-memory-allocation.md)
2. [Kernel Virtual Memory And VMAs](kernel-virtual-memory-and-vmas.md)
3. [MMIO And Register Access](mmio-and-register-access.md)
4. [Userspace Copy And ioctl ABI](userspace-copy-and-ioctl-abi.md)
5. [DMA Mapping Basics](dma-mapping-basics.md)
6. [Single-Buffer DMA](single-buffer-dma.md)
7. [Scatter-Gather DMA](scatter-gather-dma.md)

## Mental Model

Kernel code cannot treat memory, pointers, registers, and device-owned buffers like normal userspace data. The access path determines which helpers, barriers, and lifetime rules apply.

## Completion Criteria

- Choose allocation helpers and flags appropriate to context.
- Explain when `kmalloc`, `vmalloc`, and VMA handling apply.
- Access MMIO through kernel accessors.
- Copy data to and from userspace safely.
- Explain DMA coherent memory and streaming mappings.

## Related Topics

- [Kernel Execution And Concurrency](../execution-and-concurrency/index.md)
- [DMA Basics](../driver-interfaces/dma-basics.md)
- [Character Device Basics](../fundamentals/character-device-basics.md)

---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# Kernel Memory And I/O

This track covers how drivers allocate memory, access registers, interact with userspace memory, and prepare buffers for devices.

It assumes you already know:

- [Linux Device Driver Fundamentals](../fundamentals/index.md)
- [Common Driver Interfaces](../driver-interfaces/index.md)
- [Kernel Execution And Concurrency](../execution-and-concurrency/index.md)

## What Problem Does This Solve?

Driver code touches several kinds of "memory-like" things that are not interchangeable:

- normal kernel memory
- userspace memory
- MMIO register windows
- coherent DMA memory
- streaming DMA mappings
- vmalloc areas
- userspace VMAs
- IOMMU-visible DMA addresses

A normal C pointer model is not enough. The driver must know which address space it is touching, who owns the bytes, whether access may fault or sleep, and whether the CPU or device currently owns the buffer.

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

Start every memory/I/O decision with these questions:

```text
What address space is this?
Who owns it right now?
Can this access sleep or fault?
Does the device need to see it?
Does userspace need to see it?
Does the access have ordering requirements?
What frees or unmaps it?
```

## Address Space Map

| Thing | Example Type | Access With | Common Mistake |
| --- | --- | --- | --- |
| normal kernel memory | `void *`, `struct demo_priv *` | normal C loads/stores | using after free |
| userspace pointer | `void __user *` | `copy_to_user()`, `copy_from_user()`, `get_user()`, `put_user()` | direct dereference |
| MMIO register | `void __iomem *` | `readl()`, `writel()`, related accessors | direct dereference |
| DMA address | `dma_addr_t` | program into device registers/descriptors | treating as CPU pointer |
| physical address | `phys_addr_t` | low-level resource description | passing to CPU code as pointer |
| vmalloc memory | `void *` | normal CPU access, special mapping rules | assuming physical contiguity |
| userspace VMA | `struct vm_area_struct *` | VMA helpers and mmap rules | mapping wrong lifetime or cache mode |

The annotations `__user` and `__iomem` are not decoration. They mark pointers that need special access rules.

## Ownership Map

| Flow | Meaning |
| --- | --- |
| CPU owns normal buffer | CPU may read/write normally. |
| CPU copies to/from userspace | access may fault and must use usercopy helpers. |
| CPU touches MMIO | use I/O accessors and respect ordering. |
| CPU maps streaming DMA buffer | DMA API prepares the buffer for device access. |
| device owns streaming DMA buffer | CPU must not touch until sync/unmap. |
| CPU unmaps or syncs DMA buffer | CPU may inspect results according to direction. |
| userspace maps driver memory | VMA lifetime and permissions define access. |

Most memory bugs come from crossing one of these ownership boundaries without the matching helper.

## Common Driver Examples

Per-device state:

```c
priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
```

Register window:

```c
priv->regs = devm_platform_ioremap_resource(pdev, 0);
```

Userspace copy:

```c
if (copy_to_user(buf, &value, sizeof(value)))
    return -EFAULT;
```

Coherent DMA descriptor ring:

```c
priv->ring = dmam_alloc_coherent(dev, size, &priv->ring_dma,
                                 GFP_KERNEL);
```

Streaming DMA buffer:

```c
dma = dma_map_single(dev, buf, len, DMA_TO_DEVICE);
```

Each example has a different pointer/address type and a different teardown rule.

## Completion Criteria

You are ready to move on when you can:

- choose `kmalloc()`, `kcalloc()`, `devm_kzalloc()`, `kvzalloc()`, or `vmalloc()` for a specific allocation
- choose `GFP_KERNEL`, `GFP_NOWAIT`, or `GFP_ATOMIC` based on context
- explain why userspace pointers are never directly dereferenced
- design an ioctl structure that is ABI-stable
- map platform MMIO resources and access registers through I/O helpers
- explain why `void __iomem *` is not a normal pointer
- explain why `dma_addr_t` is not a CPU pointer
- set a DMA mask before allocating or mapping DMA buffers
- choose coherent DMA memory versus streaming DMA mappings
- map, unmap, and synchronize streaming DMA buffers correctly
- explain the difference between original SG entries and mapped SG entries
- keep CPU/device ownership transitions explicit in error and timeout paths

## Common Mistakes

- Using userspace pointers as normal C pointers.
- Passing CPU virtual addresses to hardware.
- Assuming DMA addresses are physical addresses.
- Assuming `vmalloc()` memory is physically contiguous.
- Using `GFP_KERNEL` from atomic context.
- Copying internal kernel structs directly to userspace.
- Exposing uninitialized structure padding in ABI data.
- Dereferencing MMIO pointers directly.
- Forgetting that MMIO writes may be posted.
- Touching a streaming DMA buffer while the device owns it.
- Forgetting to unmap DMA mappings on error and timeout paths.
- Debugging DMA before verifying basic register access and interrupts.

## Related Topics

- [Kernel Execution And Concurrency](../execution-and-concurrency/index.md)
- [DMA Basics](../driver-interfaces/dma-basics.md)
- [Character Device Basics](../fundamentals/character-device-basics.md)
- [Regmap](../driver-interfaces/regmap.md)

## Official References

- [Memory Allocation Guide](https://docs.kernel.org/core-api/memory-allocation.html)
- [Linux device drivers infrastructure: I/O access](https://docs.kernel.org/driver-api/device-io.html)
- [DMA API HOWTO](https://docs.kernel.org/core-api/dma-api-howto.html)
- [DMA API](https://docs.kernel.org/core-api/dma-api.html)
- [Ioctl based interfaces](https://docs.kernel.org/driver-api/ioctl.html)

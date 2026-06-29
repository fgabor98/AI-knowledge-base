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

This page connects three ideas that are easy to confuse:

- CPU virtual memory used by the kernel
- `vmalloc()` memory, which is virtually contiguous but not physically contiguous
- userspace VMAs, which describe regions of a process address space

Most simple drivers do not need custom `mmap()`. When they do, correctness depends on page alignment, permissions, cache attributes, and lifetime.

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

```text
kernel CPU pointer:
  normal pointer for CPU access

vmalloc area:
  contiguous in kernel virtual address space
  backed by pages that may be physically scattered

userspace VMA:
  range in a process address space
  maps pages or device memory according to driver rules
```

Do not infer physical layout from the fact that a CPU pointer increments normally.

## `kmalloc()` Versus `vmalloc()`

`kmalloc()`:

- returns virtually contiguous kernel memory
- is physically contiguous for the allocation
- is suitable for small objects and buffers
- becomes less reliable for large allocations

`vmalloc()`:

- returns virtually contiguous kernel memory
- may be backed by physically non-contiguous pages
- is useful for large CPU-only buffers
- has different mapping and TLB behavior
- is not suitable as one physically contiguous DMA buffer

Example:

```c
buf = vzalloc(size);
if (!buf)
    return -ENOMEM;
```

Free:

```c
vfree(buf);
```

If the code may use either kmalloc or vmalloc backing, prefer `kvzalloc()` and `kvfree()`.

## Why `vmalloc()` Is Not A DMA Shortcut

Wrong model:

```text
vmalloc gives one big pointer, so the device can DMA to it as one buffer
```

Correct model:

```text
vmalloc gives one big CPU virtual range, but physical pages may be scattered
```

A device using direct DMA needs DMA addresses, not CPU virtual addresses. For non-contiguous memory, use scatter-gather DMA or a DMA API pattern designed for the subsystem.

Do not call `virt_to_phys()` on arbitrary `vmalloc()` memory. It is not a valid way to build DMA addresses.

## Kernel Virtual Address Types

| Memory | CPU Access | Physical Contiguity | Common Use |
| --- | --- | --- | --- |
| stack | normal C | not for DMA assumptions | local variables only |
| `kmalloc` | normal C | physically contiguous allocation | small driver data |
| `vmalloc` | normal C | not necessarily contiguous | large CPU-only arrays |
| coherent DMA | normal C plus DMA address | DMA API defined | descriptor rings/shared buffers |
| MMIO | I/O accessors | device registers | `readl()`/`writel()` |

Never DMA-map stack memory. Stack lifetime and physical layout are wrong for device ownership.

## Userspace VMAs

A VMA describes a contiguous range of a process virtual address space.

Drivers see VMAs in `mmap()`:

```c
static int demo_mmap(struct file *filp, struct vm_area_struct *vma)
{
    unsigned long size = vma->vm_end - vma->vm_start;

    if (size > DEMO_MAX_MMAP_SIZE)
        return -EINVAL;

    return 0;
}
```

Important VMA fields:

| Field | Meaning |
| --- | --- |
| `vm_start` | userspace start address |
| `vm_end` | userspace end address |
| `vm_pgoff` | page offset passed by userspace |
| `vm_flags` | mapping flags and permissions |
| `vm_page_prot` | page protection attributes |
| `vm_ops` | optional VMA operations |
| `vm_private_data` | driver-private mapping state |

Treat userspace input to `mmap()` as untrusted. Validate size, offset, permissions, and object lifetime.

## Page Alignment

Mappings are page based.

```c
size = vma->vm_end - vma->vm_start;
if (size == 0 || size > priv->buffer_size)
    return -EINVAL;

if (vma->vm_pgoff != 0)
    return -EINVAL;
```

For page-aligned calculations:

```c
size = PAGE_ALIGN(size);
```

Do not let `vm_pgoff` select arbitrary kernel memory. It is userspace-controlled input.

## Mapping Coherent DMA Memory To Userspace

Some drivers expose coherent DMA buffers to userspace through subsystem-supported mechanisms. The generic shape uses DMA mmap helpers.

```c
static int demo_mmap(struct file *filp, struct vm_area_struct *vma)
{
    struct demo_priv *priv = filp->private_data;
    unsigned long size = vma->vm_end - vma->vm_start;

    if (size > priv->dma_size)
        return -EINVAL;

    return dma_mmap_coherent(priv->dev, vma,
                             priv->dma_cpu,
                             priv->dma_handle,
                             priv->dma_size);
}
```

This maps memory allocated with `dma_alloc_coherent()` or the managed equivalent.

Do not map coherent DMA memory with a generic helper that loses the required cache attributes.

## Mapping `vmalloc()` Memory

If a driver needs to map `vmalloc()` memory to userspace, use VM helpers designed for it.

Conceptual shape:

```c
static int demo_mmap(struct file *filp, struct vm_area_struct *vma)
{
    struct demo_priv *priv = filp->private_data;
    unsigned long size = vma->vm_end - vma->vm_start;

    if (size > priv->vmalloc_size)
        return -EINVAL;

    return remap_vmalloc_range(vma, priv->vmalloc_area, 0);
}
```

This is for CPU memory. It does not make the memory physically contiguous or automatically suitable for a device DMA engine.

## Mapping Device Registers

Userspace register mappings are specialized and risky. Most drivers should not expose raw device registers to userspace.

If a subsystem intentionally supports it, mapping must use the correct page protection and `remap_pfn_range()` style flow.

Conceptual shape:

```c
vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
ret = remap_pfn_range(vma, vma->vm_start,
                      phys_addr >> PAGE_SHIFT,
                      size, vma->vm_page_prot);
```

Do not provide raw MMIO `mmap()` as a shortcut around a real kernel driver ABI. It bypasses validation, locking, power management, and concurrency control.

## VMA Operations

VMA operations let a driver track mapping lifetime.

```c
static void demo_vma_open(struct vm_area_struct *vma)
{
    struct demo_priv *priv = vma->vm_private_data;

    demo_get(priv);
}

static void demo_vma_close(struct vm_area_struct *vma)
{
    struct demo_priv *priv = vma->vm_private_data;

    demo_put(priv);
}

static const struct vm_operations_struct demo_vm_ops = {
    .open = demo_vma_open,
    .close = demo_vma_close,
};
```

Install:

```c
vma->vm_ops = &demo_vm_ops;
vma->vm_private_data = priv;
demo_vma_open(vma);
```

Use this when mappings need to hold references or track active users.

## Page Fault Based Mapping

Some drivers implement `.fault` to insert pages lazily.

Conceptual shape:

```c
static vm_fault_t demo_fault(struct vm_fault *vmf)
{
    struct demo_mapping *map = vmf->vma->vm_private_data;
    unsigned long offset = vmf->pgoff << PAGE_SHIFT;
    struct page *page;

    if (offset >= map->size)
        return VM_FAULT_SIGBUS;

    page = map->pages[vmf->pgoff];
    get_page(page);
    vmf->page = page;

    return 0;
}
```

This is advanced driver work. The page lifetime, reference counts, and invalidation behavior must be correct.

## Cache Attributes

The same physical memory cannot be safely mapped with incompatible cache attributes.

Drivers must use the mapping helper appropriate to the memory type:

- coherent DMA memory: DMA mmap helper
- MMIO: non-cached or device mapping through I/O mapping helpers
- normal CPU memory: normal page mapping helpers
- write-combining memory: only when explicitly required and supported

Cache-attribute mistakes can produce intermittent data corruption that is hard to debug.

## `mmap()` ABI Design

If a driver exposes `mmap()`, document:

```text
what region offset 0 means
allowed mapping sizes
allowed protections
whether MAP_SHARED is required
whether writes are allowed
what happens on device removal
whether cache coherency with the device is guaranteed
```

Reject unsupported mappings:

```c
if (!(vma->vm_flags & VM_SHARED))
    return -EINVAL;

if (vma->vm_flags & VM_WRITE && !priv->writable)
    return -EPERM;
```

Use subsystem conventions when available. For example, graphics, V4L2, and IIO buffers have subsystem-specific mmap models.

## Common Failure Modes

| Symptom | Likely Cause | First Checks |
| --- | --- | --- |
| DMA corrupts data | vmalloc memory treated as contiguous DMA buffer | DMA mapping path |
| userspace SIGBUS | fault handler rejected offset or page missing | VMA offset/size |
| data incoherent in userspace | wrong cache attributes | mmap helper |
| use-after-free after `mmap()` | VMA did not hold object reference | VMA open/close |
| security bug | mmap offset exposes unrelated memory | `vm_pgoff` validation |
| crash on unload | mapping outlives driver data | references and invalidation |

## Practice Exercises

### Exercise 1: Contiguity Table

For stack, `kmalloc`, `vmalloc`, coherent DMA, and MMIO memory, write:

```text
CPU pointer type
physically contiguous?
DMA-safe directly?
free/unmap helper
```

### Exercise 2: VMA Validation

Sketch an `mmap()` method that validates:

```text
size
offset
permissions
shared/private mapping
object lifetime
```

### Exercise 3: Map Type Choice

For each mapping requirement, choose the helper:

```text
coherent DMA buffer to userspace
vmalloc CPU buffer to userspace
device register range to userspace
normal copy-based read/write instead of mmap
```

## Debugging Checklist

- Do not DMA from arbitrary `vmalloc` memory.
- Check mapping lifetime.
- Check page alignment.
- Check cacheability and access permissions.
- Validate `vm_pgoff` and mapping size.
- Use VMA operations when mappings must hold references.
- Use memory-type-specific mmap helpers.
- Reject unsupported protections instead of silently accepting them.

## Related Topics

- [Kernel Memory Allocation](kernel-memory-allocation.md)
- [DMA Mapping Basics](dma-mapping-basics.md)
- [Userspace Copy And ioctl ABI](userspace-copy-and-ioctl-abi.md)
- [Reference Counting And Lifetime](../execution-and-concurrency/reference-counting-and-lifetime.md)

## Official References

- [Memory Management APIs](https://docs.kernel.org/core-api/mm-api.html)
- [DMA API](https://docs.kernel.org/core-api/dma-api.html)

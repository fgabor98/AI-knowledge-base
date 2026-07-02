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

Allocation choices affect correctness:

- an IRQ handler cannot sleep waiting for memory
- a DMA buffer may need special mapping or coherent allocation
- a large physically contiguous allocation may fail under fragmentation
- a userspace ABI must not expose uninitialized padding
- device-managed memory does not automatically stop asynchronous callbacks
- arrays need overflow-safe allocation helpers

This page is about ordinary CPU-owned kernel memory. DMA-specific memory ownership is covered later in this chapter.

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

```text
small fixed-size object?
  kzalloc() or devm_kzalloc()

array?
  kcalloc() or kmalloc_array()

large virtually contiguous CPU buffer?
  vmalloc(), vzalloc(), kvzalloc()

device lifetime?
  devm_* helper if teardown ordering still remains correct

atomic context?
  preallocate if possible; otherwise use non-sleeping GFP flags carefully

DMA-visible buffer?
  use DMA API rules, not ordinary allocation alone
```

## Allocation Questions

Before choosing a helper, answer:

```text
How large is the allocation?
Does it need to be zeroed?
Does it need physical contiguity?
Does a device need to DMA to it?
Who frees it?
Can the current context sleep?
Could multiplication overflow?
Is failure recoverable?
```

If the answer is "device will DMA to it," jump to [DMA Mapping Basics](dma-mapping-basics.md). Ordinary `kmalloc()` only gives CPU memory; it does not create a DMA ownership contract.

## Common Helpers

| Helper | Use |
| --- | --- |
| `kmalloc(size, flags)` | allocate uninitialized physically contiguous kernel memory |
| `kzalloc(size, flags)` | allocate zeroed memory |
| `kcalloc(n, size, flags)` | allocate zeroed array with overflow checking |
| `kmalloc_array(n, size, flags)` | allocate uninitialized array with overflow checking |
| `krealloc(ptr, size, flags)` | resize allocation |
| `kfree(ptr)` | free `kmalloc` family allocation |
| `devm_kzalloc(dev, size, flags)` | zeroed allocation tied to device lifetime |
| `vmalloc(size)` | virtually contiguous memory |
| `vzalloc(size)` | zeroed vmalloc memory |
| `kvzalloc(size, flags)` | try kmalloc-style allocation, fall back to vmalloc |
| `kvfree(ptr)` | free memory from `kmalloc`, `vmalloc`, or `kv*` helpers |

Use the helper that expresses intent. `kcalloc()` is clearer and safer than `kmalloc(n * size, flags)` when allocating an array.

## `GFP_KERNEL`

Use `GFP_KERNEL` in normal sleepable kernel context.

Examples:

- probe
- remove
- file operations
- sysfs show/store
- workqueue callbacks
- threaded IRQ handlers, if no atomic lock is held

```c
priv = devm_kzalloc(&pdev->dev, sizeof(*priv), GFP_KERNEL);
if (!priv)
    return -ENOMEM;
```

`GFP_KERNEL` may sleep and enter reclaim. Do not use it from hard IRQ handlers, timer callbacks, hrtimer callbacks, or while holding spinlocks.

## Non-Sleeping Allocation Flags

If code cannot sleep, prefer preallocation. Allocating from atomic context is a fallback, not a design goal.

Common choices:

| Flag | Meaning |
| --- | --- |
| `GFP_NOWAIT` | do not sleep or perform direct reclaim |
| `GFP_ATOMIC` | non-sleeping allocation with access to emergency reserves |

Example in a constrained path:

```c
buf = kmalloc(len, GFP_ATOMIC);
if (!buf)
    return IRQ_NONE;
```

This may be acceptable for small emergency allocations, but repeated IRQ-path allocation is fragile. Better:

```c
/* probe */
priv->irq_buf = devm_kmalloc(dev, DEMO_IRQ_BUF_SIZE, GFP_KERNEL);
if (!priv->irq_buf)
    return -ENOMEM;

/* IRQ */
demo_fill_irq_buf(priv->irq_buf);
```

Design high-frequency or hard-realtime paths to use memory allocated earlier.

## Zeroing And Initialization

Use zeroing helpers when zero is the correct initial state:

```c
priv = kzalloc(sizeof(*priv), GFP_KERNEL);
if (!priv)
    return -ENOMEM;
```

This avoids bugs from uninitialized flags, list heads, counters, and padding. But zeroing does not initialize everything.

Still initialize objects that require API initialization:

```c
mutex_init(&priv->lock);
spin_lock_init(&priv->irq_lock);
init_waitqueue_head(&priv->wait);
INIT_WORK(&priv->work, demo_work_fn);
INIT_LIST_HEAD(&priv->buffers);
```

Zero is not a substitute for `INIT_*()` helpers.

## Overflow-Safe Array Allocation

Wrong:

```c
entries = kmalloc(count * sizeof(*entries), GFP_KERNEL);
```

If `count * sizeof(*entries)` overflows, the allocation is too small and later writes corrupt memory.

Better:

```c
entries = kcalloc(count, sizeof(*entries), GFP_KERNEL);
if (!entries)
    return -ENOMEM;
```

For uninitialized arrays:

```c
entries = kmalloc_array(count, sizeof(*entries), GFP_KERNEL);
```

When resizing:

```c
tmp = krealloc_array(entries, new_count, sizeof(*entries), GFP_KERNEL);
if (!tmp)
    return -ENOMEM;
entries = tmp;
```

Check whether the kernel version you target provides the exact helper you want; use the closest overflow-safe local pattern when maintaining older kernels.

## Device-Managed Allocation

Device-managed allocation is useful for per-device state that should be freed automatically when the device is detached.

```c
static int demo_probe(struct platform_device *pdev)
{
    struct demo_priv *priv;

    priv = devm_kzalloc(&pdev->dev, sizeof(*priv), GFP_KERNEL);
    if (!priv)
        return -ENOMEM;

    platform_set_drvdata(pdev, priv);
    return 0;
}
```

Good uses:

- per-device private structure
- small configuration structures
- resource tables tied to the device
- memory that does not need a custom free order

Bad assumptions:

- devm allocation cancels work for you
- devm allocation stops timers for you
- devm allocation closes open file descriptors
- devm allocation fixes references held by async callbacks

If work, timers, IRQ handlers, or file operations can still use the object, stop them before devm cleanup frees the memory.

## Manual Lifetime Allocation

Use manual allocation when lifetime is not exactly device lifetime.

Example: per-open file state.

```c
static int demo_open(struct inode *inode, struct file *filp)
{
    struct demo_file *ctx;

    ctx = kzalloc(sizeof(*ctx), GFP_KERNEL);
    if (!ctx)
        return -ENOMEM;

    filp->private_data = ctx;
    return 0;
}

static int demo_release(struct inode *inode, struct file *filp)
{
    struct demo_file *ctx = filp->private_data;

    kfree(ctx);
    return 0;
}
```

Do not use devm for per-open state. The file descriptor can outlive probe and remove sequencing in ways that need explicit release handling.

## `kmalloc()` Versus `vmalloc()`

`kmalloc()` returns memory that is virtually contiguous and physically contiguous. This matters for low-level hardware and page-level assumptions, but large physically contiguous allocations are harder to satisfy.

`vmalloc()` returns memory that is virtually contiguous but not necessarily physically contiguous.

| Need | Prefer |
| --- | --- |
| small object or buffer | `kmalloc()`/`kzalloc()` |
| array with overflow checking | `kcalloc()`/`kmalloc_array()` |
| large CPU-only buffer | `vmalloc()`/`vzalloc()` or `kvzalloc()` |
| buffer that may be either kmalloc or vmalloc | `kvzalloc()` plus `kvfree()` |
| DMA coherent buffer | `dma_alloc_coherent()` or managed variant |
| streaming DMA buffer | ordinary memory plus DMA mapping, subject to DMA API constraints |

Do not use `vmalloc()` when a device needs a physically contiguous buffer. Use DMA mapping, scatter-gather, or coherent DMA allocation as appropriate.

## `kvzalloc()` And `kvfree()`

`kvzalloc()` is useful when a buffer is logically one CPU-only array but might be too large for reliable `kmalloc()`.

```c
priv->table = kvzalloc(array_size(count, sizeof(*priv->table)),
                       GFP_KERNEL);
if (!priv->table)
    return -ENOMEM;
```

Free with:

```c
kvfree(priv->table);
```

Use `kvfree()` for memory that may have come from `kmalloc()` or `vmalloc()` through `kv*` helpers.

Do not pass arbitrary `kvzalloc()` memory to hardware as a single DMA segment.

## Slab Caches

For many allocations of the same object type, a slab cache can improve clarity and performance.

```c
static struct kmem_cache *demo_cmd_cache;

demo_cmd_cache = KMEM_CACHE(demo_cmd, 0);
if (!demo_cmd_cache)
    return -ENOMEM;
```

Allocate:

```c
cmd = kmem_cache_zalloc(demo_cmd_cache, GFP_KERNEL);
if (!cmd)
    return -ENOMEM;
```

Free:

```c
kmem_cache_free(demo_cmd_cache, cmd);
```

Use slab caches when:

- the object is allocated frequently
- the object has a fixed size
- constructor behavior or debugging helps
- allocation profiling shows value

For ordinary driver code, `kzalloc()` is simpler until repetition justifies a cache.

## Freeing Rules

Match allocation and free:

| Allocated With | Free With |
| --- | --- |
| `kmalloc()`, `kzalloc()`, `kcalloc()` | `kfree()` |
| `vmalloc()`, `vzalloc()` | `vfree()` |
| `kvzalloc()`/`kvmalloc()` | `kvfree()` |
| `kmem_cache_alloc()` | `kmem_cache_free()` |
| `devm_kzalloc()` | no manual `kfree()`; devm cleanup handles it |
| `dma_alloc_coherent()` | `dma_free_coherent()` |
| `dmam_alloc_coherent()` | managed DMA cleanup |

Do not mix these casually. A wrong free helper is a memory-corruption bug, not a style issue.

## Error Handling

Always check allocation failure:

```c
buf = kmalloc(size, GFP_KERNEL);
if (!buf)
    return -ENOMEM;
```

In probe paths, return the real error:

```c
priv->buf = devm_kmalloc(dev, size, GFP_KERNEL);
if (!priv->buf)
    return -ENOMEM;
```

When a function has multiple manual allocations, use a clear unwind path:

```c
a = kzalloc(sizeof(*a), GFP_KERNEL);
if (!a)
    return -ENOMEM;

b = kzalloc(sizeof(*b), GFP_KERNEL);
if (!b) {
    ret = -ENOMEM;
    goto err_free_a;
}

return 0;

err_free_a:
kfree(a);
return ret;
```

Device-managed helpers can reduce this boilerplate, but only when the resource really has device lifetime.

## Allocating For Userspace Copies

Do not copy uninitialized kernel memory to userspace.

Wrong:

```c
struct demo_status status;

status.value = priv->value;
copy_to_user(argp, &status, sizeof(status));
```

Padding bytes may contain stack data.

Better:

```c
struct demo_status status = { };

status.value = priv->value;

if (copy_to_user(argp, &status, sizeof(status)))
    return -EFAULT;
```

ABI structures should be zeroed and explicitly filled.

## Common Failure Modes

| Symptom | Likely Cause | First Checks |
| --- | --- | --- |
| sleep warning | `GFP_KERNEL` in atomic context | stack trace and allocation flags |
| rare allocation failure | large physically contiguous `kmalloc()` | allocation size and fragmentation |
| memory corruption | overflowed `count * size` | use `kcalloc()` or `array_size()` |
| use-after-free after remove | async callback used devm memory after cleanup | work/timer/IRQ teardown |
| leak on error path | missing free during partial init | unwind labels |
| userspace sees garbage bytes | uninitialized padding copied out | zero ABI structs |
| DMA corruption | ordinary allocation used without DMA mapping | DMA API usage |

## Practice Exercises

### Exercise 1: Allocation Audit

For every allocation in a driver, record:

```text
helper
size
lifetime owner
free helper
context
GFP flags
can device DMA to it?
```

Fix any allocation whose flags or lifetime do not match its context.

### Exercise 2: Array Safety

Replace open-coded `count * sizeof(*ptr)` allocations with `kcalloc()`, `kmalloc_array()`, or `array_size()`.

### Exercise 3: Remove-Path Lifetime

For every devm allocation referenced by work, timers, IRQ handlers, or file operations, verify remove stops those users before cleanup.

## Debugging Checklist

- Check NULL returns.
- Check overflow-safe allocation helpers.
- Check context before using `GFP_KERNEL`.
- Use KASAN and kmemleak where available.
- Match allocation and free helpers.
- Zero structures copied to userspace.
- Preallocate for hard IRQ paths where possible.
- Avoid large physically contiguous allocations unless required.
- Do not assume devm cleanup handles asynchronous users.

## Related Topics

- [Resource Lookup And Managed Allocation](../fundamentals/resource-lookup-and-devm.md)
- [Sleepable Vs Atomic Code](../execution-and-concurrency/sleepable-vs-atomic-code.md)
- [DMA Mapping Basics](dma-mapping-basics.md)
- [Reference Counting And Lifetime](../execution-and-concurrency/reference-counting-and-lifetime.md)

## Official References

- [Memory Allocation Guide](https://docs.kernel.org/core-api/memory-allocation.html)
- [Memory Management APIs](https://docs.kernel.org/core-api/mm-api.html)

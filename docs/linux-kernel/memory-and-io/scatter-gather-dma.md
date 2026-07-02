---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# Scatter-Gather DMA

## What Problem Does This Solve?

Scatter-gather DMA lets a device transfer data across multiple memory segments without copying them into one physically contiguous buffer first.

It is the answer when a logical buffer is made of several pieces:

- multiple `kmalloc()` buffers
- pages from a page list
- block or network buffers
- subsystem-managed buffers
- large buffers that should not require one physically contiguous allocation

The device sees a list of DMA address/length segments. The CPU sees a list of memory fragments described by `struct scatterlist`.

## Core Concepts

- scatterlist
- segment
- `dma_map_sg`
- `dma_unmap_sg`
- DMA direction
- DMAengine descriptor
- cookie
- completion callback

## Mental Model

Scatter-gather describes a logical buffer as a list of physical segments the DMA API maps for the device.

```text
original scatterlist:
  describes CPU memory fragments

dma_map_sg():
  maps fragments for device
  may merge adjacent entries
  returns mapped entry count

driver or DMAengine:
  uses mapped DMA address/length entries

dma_unmap_sg():
  releases mapping using original entry count
```

The original entry count and mapped entry count are not always the same.

## Scatterlist Basics

Declare and initialize:

```c
struct scatterlist sgl[2];

sg_init_table(sgl, ARRAY_SIZE(sgl));
sg_set_buf(&sgl[0], buf0, len0);
sg_set_buf(&sgl[1], buf1, len1);
```

Map:

```c
int mapped;

mapped = dma_map_sg(dev, sgl, ARRAY_SIZE(sgl), DMA_TO_DEVICE);
if (mapped == 0)
    return -EIO;
```

Use mapped entries:

```c
struct scatterlist *sg;
int i;

for_each_sg(sgl, sg, mapped, i) {
    dma_addr_t addr = sg_dma_address(sg);
    unsigned int len = sg_dma_len(sg);

    demo_program_segment(priv, i, addr, len);
}
```

Unmap:

```c
dma_unmap_sg(dev, sgl, ARRAY_SIZE(sgl), DMA_TO_DEVICE);
```

Unmap uses the original number of entries, not the mapped return count.

## Original Entries Versus Mapped Entries

This is one of the most important SG rules:

```text
nents:
  number of entries originally passed to dma_map_sg()

mapped:
  return value from dma_map_sg()
  number of DMA segments the device should use

unmap:
  call dma_unmap_sg(dev, sgl, nents, dir)
```

Wrong:

```c
mapped = dma_map_sg(dev, sgl, nents, DMA_TO_DEVICE);
...
dma_unmap_sg(dev, sgl, mapped, DMA_TO_DEVICE); /* wrong */
```

Better:

```c
mapped = dma_map_sg(dev, sgl, nents, DMA_TO_DEVICE);
if (mapped == 0)
    return -EIO;

...

dma_unmap_sg(dev, sgl, nents, DMA_TO_DEVICE);
```

The DMA API may merge adjacent entries for the device, but it still needs the original list shape to undo the mapping.

## Building SG Lists From Buffers

For a small fixed set of kernel buffers:

```c
struct demo_sg_xfer {
    struct scatterlist sgl[DEMO_MAX_SEGS];
    int nents;
};

sg_init_table(xfer->sgl, 2);
sg_set_buf(&xfer->sgl[0], header, header_len);
sg_set_buf(&xfer->sgl[1], payload, payload_len);
xfer->nents = 2;
```

Do not use stack buffers as segment memory:

```c
u8 header[16];
sg_set_buf(&sgl[0], header, sizeof(header)); /* wrong for DMA */
```

The scatterlist object can be temporary if the mapping and transfer lifetime are controlled, but the memory it describes must remain valid until DMA is complete and unmapped.

## Page-Based SG Lists

For page-backed buffers:

```c
sg_set_page(&sgl[i], page, len, offset);
```

This is common in subsystems that already work with pages. Page references and ownership must remain valid until the DMA transfer completes.

Do not pin userspace pages casually. Long-term user page pins affect memory migration, reclaim, and filesystem behavior. Use subsystem APIs when possible.

## Direct Hardware SG Programming

Some devices have a descriptor format:

```c
struct demo_hw_desc {
    __le64 addr;
    __le32 len;
    __le32 flags;
};
```

After `dma_map_sg()`:

```c
for_each_sg(sgl, sg, mapped, i) {
    desc[i].addr = cpu_to_le64(sg_dma_address(sg));
    desc[i].len = cpu_to_le32(sg_dma_len(sg));
    desc[i].flags = cpu_to_le32(DEMO_DESC_VALID);
}

desc[mapped - 1].flags |= cpu_to_le32(DEMO_DESC_LAST);
dma_wmb();
writel(DEMO_START, priv->regs + DEMO_REG_DOORBELL);
```

The descriptor ring itself is usually coherent DMA memory. The data buffers are streaming mappings.

## DMAengine Scatter-Gather Transfer

For DMAengine:

```c
struct device *dma_dev = dmaengine_get_dma_device(chan);
struct dma_async_tx_descriptor *desc;
dma_cookie_t cookie;
int mapped;
int ret;

mapped = dma_map_sg(dma_dev, sgl, nents, DMA_TO_DEVICE);
if (mapped == 0)
    return -EIO;

desc = dmaengine_prep_slave_sg(chan, sgl, mapped,
                               DMA_MEM_TO_DEV,
                               DMA_PREP_INTERRUPT);
if (!desc) {
    dma_unmap_sg(dma_dev, sgl, nents, DMA_TO_DEVICE);
    return -EIO;
}

reinit_completion(&priv->dma_done);
desc->callback = demo_dma_done;
desc->callback_param = priv;

cookie = dmaengine_submit(desc);
ret = dma_submit_error(cookie);
if (ret) {
    dma_unmap_sg(dma_dev, sgl, nents, DMA_TO_DEVICE);
    return ret;
}

dma_async_issue_pending(chan);
```

After completion:

```c
ret = wait_for_completion_timeout(&priv->dma_done,
                                  msecs_to_jiffies(1000));
if (!ret) {
    dmaengine_terminate_sync(chan);
    dma_unmap_sg(dma_dev, sgl, nents, DMA_TO_DEVICE);
    return -ETIMEDOUT;
}

dma_unmap_sg(dma_dev, sgl, nents, DMA_TO_DEVICE);
```

Pass the mapped segment count to the DMAengine descriptor preparation. Pass the original entry count to `dma_unmap_sg()`.

## RX Scatter-Gather

For device-to-memory transfers:

```c
mapped = dma_map_sg(dev, sgl, nents, DMA_FROM_DEVICE);
if (mapped == 0)
    return -EIO;

demo_start_sg_rx(priv, sgl, mapped);
```

After device completion:

```c
dma_unmap_sg(dev, sgl, nents, DMA_FROM_DEVICE);
demo_consume_received_fragments(sgl, nents);
```

The CPU reads the data only after unmap or sync-for-CPU.

If the mapping remains active for repeated receives, use `dma_sync_sg_for_device()` and `dma_sync_sg_for_cpu()` with explicit ownership transitions.

## Segment Limits

Hardware often has limits:

- maximum number of descriptors
- maximum segment length
- alignment requirements
- address boundary restrictions
- 32-bit or smaller DMA address width
- descriptor ring alignment

The DMA API handles some platform restrictions, but the driver must still respect device limits.

Example checks:

```c
if (mapped > DEMO_MAX_DESCS)
    return -E2BIG;

for_each_sg(sgl, sg, mapped, i) {
    if (sg_dma_len(sg) > DEMO_MAX_SEG_SIZE)
        return -E2BIG;
}
```

If a transfer exceeds hardware limits, split it into several transfers or use a subsystem helper that already handles segmentation.

## Lifetime Rules

The scatterlist and the memory it describes must remain valid until the DMA operation is complete and unmapped.

Rules:

- do not free buffers while DMA is active
- do not reuse stack-backed segment memory
- do not modify transmit buffers while device owns them
- do not read receive buffers before unmap/sync
- do not free the scatterlist if hardware or DMAengine still needs it
- terminate DMA before freeing mappings on timeout/remove

For asynchronous SG transfers, store the SG state in a transfer object:

```c
struct demo_sg_transfer {
    struct completion done;
    struct scatterlist *sgl;
    int nents;
    int mapped;
    enum dma_data_direction dir;
};
```

Then free it after completion and unmap.

## Error Unwind Pattern

```c
mapped = dma_map_sg(dev, sgl, nents, dir);
if (mapped == 0)
    return -EIO;

ret = demo_prepare_hw_descriptors(priv, sgl, mapped);
if (ret)
    goto err_unmap;

ret = demo_start_dma(priv);
if (ret)
    goto err_unmap;

return 0;

err_unmap:
dma_unmap_sg(dev, sgl, nents, dir);
return ret;
```

If hardware started, stop it safely before unmapping.

## Debugging SG DMA

Add debug logs for:

```text
original nents
mapped nents
direction
each sg_dma_address()
each sg_dma_len()
hardware descriptor count
completion status
unmap path
```

Use rate-limited logs for high-frequency paths.

Enable DMA API debugging where possible:

```text
CONFIG_DMA_API_DEBUG
CONFIG_DMA_API_DEBUG_SG
```

IOMMU faults are especially useful for SG bugs because they often include the address the device tried to access.

## Common Failure Modes

| Symptom | Likely Cause | First Checks |
| --- | --- | --- |
| only first fragment transfers | hardware programmed with original list incorrectly | mapped iteration |
| memory leak warning | missing `dma_unmap_sg()` | error/timeout paths |
| DMA API warning | unmapped with mapped count | unmap call |
| corruption after timeout | SG buffers freed while device still active | termination path |
| works for small buffers only | hardware segment limit exceeded | max segment/count |
| IOMMU fault | stale descriptor or wrong DMA address | descriptor contents |
| data stale after RX | CPU read before unmap/sync | ownership transition |

## Practice Exercises

### Exercise 1: Two-Buffer TX

Create an SG transmit transfer with a header buffer and payload buffer.

Show:

```text
sg_init_table()
sg_set_buf()
dma_map_sg()
for_each_sg() over mapped entries
dma_unmap_sg() with original nents
```

### Exercise 2: DMAengine SG

Convert the same SG list into a DMAengine `dmaengine_prep_slave_sg()` transfer. Explain which count goes to prep and which count goes to unmap.

### Exercise 3: Segment Limit Audit

Given a hardware maximum of 16 descriptors and 4096 bytes per descriptor, add validation or splitting for a larger SG transfer.

## Debugging Checklist

- Check the mapped segment count.
- Check max segment size and alignment constraints.
- Check unmap pairing.
- Check cache coherency on non-coherent systems.
- Use `sg_dma_address()` and `sg_dma_len()` after mapping.
- Unmap with the original entry count.
- Keep buffers and SG state alive until completion.
- Stop DMA before unmapping on timeout.
- Do not build DMA segments from stack memory.

## Related Topics

- [DMA Mapping Basics](dma-mapping-basics.md)
- [Single-Buffer DMA](single-buffer-dma.md)
- [Kernel Memory Allocation](kernel-memory-allocation.md)
- [Wait Queues And Completions](../execution-and-concurrency/wait-queues-and-completions.md)

## Official References

- [DMA API HOWTO](https://docs.kernel.org/core-api/dma-api-howto.html)
- [DMA API](https://docs.kernel.org/core-api/dma-api.html)
- [DMAengine client documentation](https://docs.kernel.org/driver-api/dmaengine/client.html)

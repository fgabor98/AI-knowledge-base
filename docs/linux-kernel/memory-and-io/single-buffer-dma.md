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

This is the simplest useful DMA shape:

```text
one buffer
one DMA address
one transfer
one completion
one unmap or sync
```

It appears in UART/SPI/I2C controller DMA paths, simple memory-to-device transfers, DMAengine examples, and direct device drivers that program a source or destination address register.

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

```text
CPU prepares buffer
-> dma_map_single()
-> program device or DMAengine descriptor with dma_addr_t
-> start transfer
-> wait for IRQ/callback/completion
-> stop/confirm device is done
-> dma_unmap_single()
-> CPU may inspect or free buffer
```

## Buffer Requirements

A single-buffer streaming mapping uses a CPU buffer that is suitable for DMA mapping.

Good examples:

- `kmalloc()` buffer
- page-backed buffer managed by a subsystem
- coherent buffer when persistent sharing is required

Bad examples:

- stack buffer
- raw userspace pointer
- arbitrary `vmalloc()` buffer as one contiguous DMA segment
- freed or short-lived temporary object

Allocate a simple test buffer:

```c
priv->rx_buf = devm_kmalloc(dev, DEMO_DMA_SIZE, GFP_KERNEL);
if (!priv->rx_buf)
    return -ENOMEM;
```

For a real data path, buffer ownership and lifetime usually deserve their own structure.

## Direct DMA Transmit Example

Assume the device has registers for source address, length, and control.

```c
static int demo_tx_one(struct demo_priv *priv, const void *data,
                       size_t len)
{
    dma_addr_t dma;
    int ret;
    int stop_ret;

    if (len > priv->tx_size)
        return -EINVAL;

    memcpy(priv->tx_buf, data, len);

    dma = dma_map_single(priv->dev, priv->tx_buf, len, DMA_TO_DEVICE);
    if (dma_mapping_error(priv->dev, dma))
        return -EIO;

    reinit_completion(&priv->dma_done);

    writel(lower_32_bits(dma), priv->regs + DEMO_REG_DMA_ADDR_LO);
    writel(upper_32_bits(dma), priv->regs + DEMO_REG_DMA_ADDR_HI);
    writel(len, priv->regs + DEMO_REG_DMA_LEN);
    writel(DEMO_DMA_START | DEMO_DMA_TO_DEVICE,
           priv->regs + DEMO_REG_DMA_CTRL);

    ret = wait_for_completion_timeout(&priv->dma_done,
                                      msecs_to_jiffies(1000));
    if (!ret) {
        stop_ret = demo_dma_stop_and_wait(priv);
        dma_unmap_single(priv->dev, dma, len, DMA_TO_DEVICE);
        return stop_ret ? stop_ret : -ETIMEDOUT;
    }

    dma_unmap_single(priv->dev, dma, len, DMA_TO_DEVICE);
    return priv->dma_status;
}
```

IRQ handler:

```c
static irqreturn_t demo_irq(int irq, void *data)
{
    struct demo_priv *priv = data;
    u32 status;

    status = readl(priv->regs + DEMO_REG_IRQ_STATUS);
    if (!(status & DEMO_IRQ_DMA_DONE))
        return IRQ_NONE;

    writel(DEMO_IRQ_DMA_DONE, priv->regs + DEMO_REG_IRQ_STATUS);
    priv->dma_status = (status & DEMO_IRQ_DMA_ERR) ? -EIO : 0;
    complete(&priv->dma_done);

    return IRQ_HANDLED;
}
```

The IRQ does not unmap here. It only records completion. The process-context waiter unmaps after the device is known to be done.

## Direct DMA Receive Example

For receive, the device writes the buffer.

```c
static int demo_rx_one(struct demo_priv *priv, size_t len)
{
    dma_addr_t dma;
    int ret;
    int stop_ret;

    if (len > priv->rx_size)
        return -EINVAL;

    dma = dma_map_single(priv->dev, priv->rx_buf, len,
                         DMA_FROM_DEVICE);
    if (dma_mapping_error(priv->dev, dma))
        return -EIO;

    reinit_completion(&priv->dma_done);

    writel(lower_32_bits(dma), priv->regs + DEMO_REG_DMA_ADDR_LO);
    writel(upper_32_bits(dma), priv->regs + DEMO_REG_DMA_ADDR_HI);
    writel(len, priv->regs + DEMO_REG_DMA_LEN);
    writel(DEMO_DMA_START | DEMO_DMA_FROM_DEVICE,
           priv->regs + DEMO_REG_DMA_CTRL);

    ret = wait_for_completion_timeout(&priv->dma_done,
                                      msecs_to_jiffies(1000));
    if (!ret) {
        stop_ret = demo_dma_stop_and_wait(priv);
        dma_unmap_single(priv->dev, dma, len, DMA_FROM_DEVICE);
        return stop_ret ? stop_ret : -ETIMEDOUT;
    }

    dma_unmap_single(priv->dev, dma, len, DMA_FROM_DEVICE);

    if (priv->dma_status)
        return priv->dma_status;

    return demo_parse_rx(priv->rx_buf, len);
}
```

The CPU parses `rx_buf` only after unmapping.

## Timeout Safety

A timeout does not prove the device stopped.

Before unmapping a buffer after timeout, make sure the device can no longer DMA into it:

```c
demo_dma_stop(priv);
ret = readl_poll_timeout(priv->regs + DEMO_REG_DMA_STATUS, val,
                         !(val & DEMO_DMA_BUSY),
                         1000, 100000);
if (ret)
    return -ETIMEDOUT;

dma_unmap_single(priv->dev, dma, len, DMA_FROM_DEVICE);
```

If hardware cannot be stopped reliably, the error recovery path may need a full device reset before the buffer can be reused or freed.

## Active Mapping With Sync

For a reusable receive buffer, keep the mapping active and sync ownership.

Setup:

```c
priv->rx_dma = dma_map_single(priv->dev, priv->rx_buf,
                              priv->rx_size, DMA_FROM_DEVICE);
if (dma_mapping_error(priv->dev, priv->rx_dma))
    return -EIO;
```

Before device receives:

```c
dma_sync_single_for_device(priv->dev, priv->rx_dma,
                           priv->rx_size, DMA_FROM_DEVICE);
demo_start_rx(priv, priv->rx_dma, priv->rx_size);
```

After completion:

```c
dma_sync_single_for_cpu(priv->dev, priv->rx_dma,
                        priv->rx_size, DMA_FROM_DEVICE);
demo_parse_rx(priv->rx_buf, priv->rx_len);
```

Final teardown:

```c
dma_unmap_single(priv->dev, priv->rx_dma,
                 priv->rx_size, DMA_FROM_DEVICE);
```

Use this pattern only when the long-lived mapping is intentional and all ownership transitions are explicit.

## DMAengine Single Transfer

DMAengine is used when a separate DMA controller performs the transfer.

Probe:

```c
priv->rx_chan = dma_request_chan(dev, "rx");
if (IS_ERR(priv->rx_chan))
    return dev_err_probe(dev, PTR_ERR(priv->rx_chan),
                         "failed to request rx DMA\n");
```

Configure channel for a peripheral:

```c
struct dma_slave_config cfg = { };

cfg.direction = DMA_DEV_TO_MEM;
cfg.src_addr = priv->fifo_phys;
cfg.src_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
cfg.src_maxburst = 4;

ret = dmaengine_slave_config(priv->rx_chan, &cfg);
if (ret)
    return ret;
```

Prepare transfer:

```c
struct device *dma_dev = dmaengine_get_dma_device(priv->rx_chan);
struct dma_async_tx_descriptor *desc;
dma_cookie_t cookie;
dma_addr_t dma;
int ret;

dma = dma_map_single(dma_dev, priv->rx_buf, len, DMA_FROM_DEVICE);
if (dma_mapping_error(dma_dev, dma))
    return -EIO;

desc = dmaengine_prep_slave_single(priv->rx_chan, dma, len,
                                   DMA_DEV_TO_MEM,
                                   DMA_PREP_INTERRUPT);
if (!desc) {
    dma_unmap_single(dma_dev, dma, len, DMA_FROM_DEVICE);
    return -EIO;
}

reinit_completion(&priv->dma_done);
desc->callback = demo_dmaengine_done;
desc->callback_param = priv;

cookie = dmaengine_submit(desc);
ret = dma_submit_error(cookie);
if (ret) {
    dma_unmap_single(dma_dev, dma, len, DMA_FROM_DEVICE);
    return ret;
}

dma_async_issue_pending(priv->rx_chan);
```

Callback:

```c
static void demo_dmaengine_done(void *data)
{
    struct demo_priv *priv = data;

    complete(&priv->dma_done);
}
```

DMAengine callbacks are not a place for sleepable cleanup. Use them to complete, wake, or schedule work.

Wait and unmap:

```c
ret = wait_for_completion_timeout(&priv->dma_done,
                                  msecs_to_jiffies(1000));
if (!ret) {
    dmaengine_terminate_sync(priv->rx_chan);
    dma_unmap_single(dma_dev, dma, len, DMA_FROM_DEVICE);
    return -ETIMEDOUT;
}

dma_unmap_single(dma_dev, dma, len, DMA_FROM_DEVICE);
```

Use the DMA device associated with the channel for mapping when the DMAengine provider requires it.

## Completion And Callback Context

DMA completion may be reported by:

- device IRQ
- DMAengine callback
- subsystem callback
- polling a status register

The completion path should usually do minimal work:

```c
priv->dma_status = status;
complete(&priv->dma_done);
```

Then process results in a sleepable context:

```c
ret = wait_for_completion_interruptible_timeout(...);
if (ret <= 0)
    handle_signal_or_timeout();

dma_unmap_single(...);
demo_process_data(...);
```

This mirrors the IRQ/threaded/workqueue split from the concurrency chapter.

## Error Path Checklist

For every single-buffer DMA path:

```text
allocation succeeded?
mapping succeeded?
hardware/descriptor submission succeeded?
completion happened?
timeout path stops device?
unmap happens exactly once?
buffer lifetime outlives transfer?
remove path terminates active transfers?
```

Example remove cleanup:

```c
WRITE_ONCE(priv->stopping, true);
dmaengine_terminate_sync(priv->rx_chan);
complete_all(&priv->dma_done);
```

For direct DMA hardware, disable DMA and IRQ sources before freeing buffers.

## Common Failure Modes

| Symptom | Likely Cause | First Checks |
| --- | --- | --- |
| transmit sends old bytes | mapped before filling buffer or missing sync | CPU-before-device order |
| receive reads old bytes | CPU read before unmap/sync | unmap path |
| crash after timeout | unmapped/freed while device still active | stop sequence |
| DMAengine transfer never completes | channel config or missing issue pending | config and `dma_async_issue_pending()` |
| callback deadlocks | sleepable work in callback | callback body |
| intermittent corruption | wrong DMA device used for mapping | `dmaengine_get_dma_device()` |

## Practice Exercises

### Exercise 1: Direct RX Transfer

Write a direct-DMA receive path with:

```text
kmalloc buffer
dma_map_single(..., DMA_FROM_DEVICE)
hardware register programming
completion wait
timeout stop
dma_unmap_single()
CPU parse after unmap
```

### Exercise 2: DMAengine RX Transfer

Request a DMA channel, configure it, prepare a slave single transfer, submit it, issue pending, wait for completion, and unmap.

### Exercise 3: Timeout Audit

For an existing DMA path, prove that the device can no longer access the buffer before the timeout path unmaps or frees it.

## Debugging Checklist

- Check mapping errors.
- Check DMA direction.
- Check callback completion.
- Check CPU/device ownership transitions.
- Confirm the DMA address, not CPU pointer, is programmed into hardware.
- Confirm timeout paths terminate DMA before unmap.
- Confirm DMAengine callbacks do not sleep.
- Confirm the mapping device is correct for DMAengine channels.
- Confirm remove terminates active transfers before freeing buffers.

## Related Topics

- [DMA Mapping Basics](dma-mapping-basics.md)
- [DMA Basics](../driver-interfaces/dma-basics.md)
- [Wait Queues And Completions](../execution-and-concurrency/wait-queues-and-completions.md)
- [MMIO And Register Access](mmio-and-register-access.md)
- [Scatter-Gather DMA](scatter-gather-dma.md)

## Official References

- [DMA API HOWTO](https://docs.kernel.org/core-api/dma-api-howto.html)
- [DMA API](https://docs.kernel.org/core-api/dma-api.html)
- [DMAengine client documentation](https://docs.kernel.org/driver-api/dmaengine/client.html)

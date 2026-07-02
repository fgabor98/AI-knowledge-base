---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# MMIO And Register Access

## What Problem Does This Solve?

MMIO lets drivers access device registers mapped into the CPU address space using the kernel's I/O access rules.

Device registers are not normal RAM. Reads and writes can have side effects, ordering requirements, width restrictions, posted-write behavior, and endian rules. The kernel exposes those constraints through I/O memory annotations and accessor functions.

## Core Concepts

- memory resources
- `ioremap`
- `devm_platform_ioremap_resource`
- `readl`
- `writel`
- relaxed accessors
- endianness
- barriers
- register fields

## Mental Model

Device registers are not normal memory. Use I/O accessors so ordering, width, and architecture rules are visible to the kernel.

```text
Device Tree or firmware describes register range
-> platform resource exposes address and size
-> driver maps resource to void __iomem *
-> driver uses readl()/writel() style accessors
-> remove/unbind unmaps through devm or explicit cleanup
```

The CPU pointer and the bus address are different things.

## Mapping A Platform Register Resource

For platform drivers, prefer the managed helper:

```c
struct demo_priv {
    void __iomem *regs;
};

static int demo_probe(struct platform_device *pdev)
{
    struct demo_priv *priv;

    priv = devm_kzalloc(&pdev->dev, sizeof(*priv), GFP_KERNEL);
    if (!priv)
        return -ENOMEM;

    priv->regs = devm_platform_ioremap_resource(pdev, 0);
    if (IS_ERR(priv->regs))
        return PTR_ERR(priv->regs);

    platform_set_drvdata(pdev, priv);
    return 0;
}
```

The helper:

- obtains the memory resource
- validates it
- requests the region
- maps it
- arranges cleanup at device detach

For manual control, the lower-level shape is:

```c
res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
regs = devm_ioremap_resource(&pdev->dev, res);
if (IS_ERR(regs))
    return PTR_ERR(regs);
```

## `__iomem` Pointers

MMIO mappings use `void __iomem *`.

```c
void __iomem *regs;
```

Do not directly dereference:

```c
value = *(u32 *)(regs + DEMO_STATUS); /* wrong */
```

Use accessors:

```c
value = readl(regs + DEMO_STATUS);
```

The `__iomem` annotation lets sparse and reviewers catch address-space mistakes.

## Register Offsets And Fields

Use named constants:

```c
#define DEMO_REG_CTRL          0x00
#define DEMO_REG_STATUS        0x04
#define DEMO_CTRL_ENABLE       BIT(0)
#define DEMO_CTRL_RESET        BIT(1)
#define DEMO_STATUS_READY      BIT(0)
#define DEMO_STATUS_ERR        BIT(1)
```

Read:

```c
u32 status;

status = readl(priv->regs + DEMO_REG_STATUS);
if (status & DEMO_STATUS_READY)
    demo_handle_ready(priv);
```

Write:

```c
writel(DEMO_CTRL_ENABLE, priv->regs + DEMO_REG_CTRL);
```

Avoid magic offsets and bit values in driver logic.

## Read-Modify-Write

Register bit updates often require read-modify-write.

```c
static void demo_enable_irq(struct demo_priv *priv)
{
    u32 val;

    val = readl(priv->regs + DEMO_REG_CTRL);
    val |= DEMO_CTRL_IRQ_EN;
    writel(val, priv->regs + DEMO_REG_CTRL);
}
```

If the register can be touched from multiple contexts, protect the sequence:

```c
spin_lock_irqsave(&priv->reg_lock, flags);
val = readl(priv->regs + DEMO_REG_CTRL);
val |= DEMO_CTRL_IRQ_EN;
writel(val, priv->regs + DEMO_REG_CTRL);
spin_unlock_irqrestore(&priv->reg_lock, flags);
```

The lock protects the software read-modify-write sequence. It does not change hardware ordering by itself.

## Register Width

Use the accessor matching the register width:

| Register Width | Accessors |
| --- | --- |
| 8 bit | `readb()`, `writeb()` |
| 16 bit | `readw()`, `writew()` |
| 32 bit | `readl()`, `writel()` |
| 64 bit | architecture/device-specific helpers, check documentation |

Do not use `readl()` on a register documented as 16 bit just because the address is aligned. Some devices have side effects per access width.

## Endianness

The normal `readl()`/`writel()` family is for the platform's standard little-endian I/O access model on most systems. Some devices expose big-endian registers or special bus semantics.

Use endian-specific helpers when required by the device and architecture:

```c
value = ioread32be(priv->regs + DEMO_BE_STATUS);
iowrite32be(value, priv->regs + DEMO_BE_CTRL);
```

Do not fix endian problems with ad hoc byte swaps around the wrong accessor unless the subsystem documentation requires it.

## Ordering And Relaxed Accessors

I/O accessors include ordering semantics appropriate for many device-driver cases. Relaxed accessors reduce some ordering constraints.

Examples:

```c
status = readl(priv->regs + DEMO_REG_STATUS);
writel(cmd, priv->regs + DEMO_REG_CMD);
```

Relaxed variants:

```c
status = readl_relaxed(priv->regs + DEMO_REG_STATUS);
writel_relaxed(cmd, priv->regs + DEMO_REG_CMD);
```

Use relaxed accessors only when you know the ordering requirement is provided elsewhere or unnecessary.

Common safe pattern:

```c
/* Fill coherent descriptor before ringing doorbell. */
priv->desc->len = len;
priv->desc->flags = DEMO_DESC_OWNED_BY_DEVICE;
dma_wmb();
writel(DEMO_DOORBELL_START, priv->regs + DEMO_REG_DOORBELL);
```

The DMA barrier orders descriptor writes before the device is told to fetch them.

## Posted Writes

MMIO writes can be posted: the CPU may continue before the write reaches the device.

If the driver must ensure a write has reached the device before continuing, a read from the same device is commonly used as a flush:

```c
writel(DEMO_CTRL_RESET, priv->regs + DEMO_REG_CTRL);
readl(priv->regs + DEMO_REG_CTRL);
```

Use this when the hardware sequence requires completion, not after every write.

## Polling Registers

Use polling helpers instead of open-coded loops where possible.

Sleepable context:

```c
#include <linux/iopoll.h>

ret = readl_poll_timeout(priv->regs + DEMO_REG_STATUS, val,
                         val & DEMO_STATUS_READY,
                         1000, 100000);
if (ret)
    return -ETIMEDOUT;
```

Atomic context:

```c
ret = readl_poll_timeout_atomic(priv->regs + DEMO_REG_STATUS, val,
                                val & DEMO_STATUS_READY,
                                1, 100);
```

Do not use sleepable polling from hard IRQ, timer, or spinlock-held paths.

## Interrupt Acknowledge Pattern

Many devices require reading status and writing back bits to acknowledge.

```c
static irqreturn_t demo_irq(int irq, void *data)
{
    struct demo_priv *priv = data;
    u32 status;

    status = readl(priv->regs + DEMO_REG_IRQ_STATUS);
    if (!(status & DEMO_IRQ_MASK))
        return IRQ_NONE;

    writel(status & DEMO_IRQ_MASK, priv->regs + DEMO_REG_IRQ_STATUS);

    if (status & DEMO_IRQ_RX_READY)
        schedule_work(&priv->rx_work);

    return IRQ_HANDLED;
}
```

Always follow the hardware manual. Some devices clear on write-one, some clear on write-zero, and some clear by reading another register.

## Resource Size And Bounds

Check that the register offsets you use fit inside the resource.

```c
resource_size_t size = resource_size(res);

if (size < DEMO_REG_REQUIRED_SIZE)
    return dev_err_probe(dev, -EINVAL, "register window too small\n");
```

An invalid offset can access another device's register window or trigger a bus fault.

## Regmap Versus Raw MMIO

Use raw MMIO when:

- the device has simple registers
- the driver needs direct control
- there is no register cache requirement

Use regmap when:

- the device is accessed over I2C/SPI
- register caching helps
- field updates and debugfs dumps are useful
- endian/stride abstractions reduce mistakes
- the subsystem convention uses regmap

For MMIO devices, regmap-mmio can provide regmap features on top of mapped registers.

## Debugging MMIO

Useful tools:

- boot logs showing resource addresses
- `/proc/iomem`
- devres debug output where available
- dynamic debug in probe and register access paths
- ftrace for driver functions
- hardware manual register reset values
- `devmem` only in controlled lab situations, not as a driver substitute

Print resource ranges during early debugging:

```c
dev_dbg(dev, "regs %pa-%pa\n", &res->start, &res->end);
```

Do not spam logs on high-frequency register reads.

## Common Failure Modes

| Symptom | Likely Cause | First Checks |
| --- | --- | --- |
| bus fault | wrong resource, offset, width, or clock/power state | DT resource and PM |
| reads all `0xffffffff` | device not powered, bus error, wrong address | clocks, resets, resource |
| write appears ignored | posted write, missing clock/reset, wrong bit | readback and hardware manual |
| intermittent DMA failure | missing barrier before doorbell | descriptor write ordering |
| interrupt storm | wrong ACK sequence | IRQ status/clear semantics |
| endian-swapped values | wrong accessor | device endian requirements |
| lock race | read-modify-write from several contexts | register lock |

## Practice Exercises

### Exercise 1: Map And Read ID

Write a platform-driver probe path that maps resource 0, reads an ID register, and checks it against an expected value.

### Exercise 2: Bit Update

Implement helpers:

```text
enable device
disable device
acknowledge IRQ
read status
```

Use named offsets and masks only.

### Exercise 3: Polling Audit

Find all busy polling loops in a driver and replace them with `readl_poll_timeout()` or `readl_poll_timeout_atomic()` as context requires.

## Debugging Checklist

- Check resource address and size.
- Check register width and alignment.
- Check endianness.
- Avoid direct pointer dereferences into MMIO.
- Confirm clocks, resets, regulators, and runtime PM state before access.
- Use accessors matching register width.
- Use barriers around descriptor/doorbell sequences when required.
- Check posted-write requirements in the hardware manual.
- Protect read-modify-write sequences shared by multiple contexts.

## Related Topics

- [Platform Devices And Platform Drivers](../fundamentals/platform-devices-and-drivers.md)
- [Regmap](../driver-interfaces/regmap.md)
- [Device Tree](../../device-tree/index.md)
- [Clocks](../driver-interfaces/clocks.md)
- [Resets](../driver-interfaces/resets.md)
- [Runtime PM](../power-management/runtime-pm.md)

## Official References

- [Linux device drivers infrastructure: I/O access](https://docs.kernel.org/driver-api/device-io.html)
- [Memory Barriers](https://docs.kernel.org/core-api/wrappers/memory-barriers.html)

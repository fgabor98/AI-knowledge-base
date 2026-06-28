---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# Platform Devices And Platform Drivers

## What Problem Does This Solve?

Platform drivers handle devices that are present because board firmware, Device Tree, ACPI, or board code described them, not because a discoverable bus enumerated them dynamically.

They are common for:

- SoC MMIO blocks
- FPGA-attached IP blocks
- board-specific glue devices
- simple memory-mapped controllers
- reset or power-control blocks
- devices created from Device Tree nodes under simple buses

Platform drivers are one of the first driver types to learn because they show the kernel device model clearly without bus transaction details like I2C or SPI transfers.

## Core Concepts

- platform bus
- `struct platform_device`
- `struct platform_driver`
- firmware-created devices
- Device Tree platform devices
- `platform_driver_register()`
- `module_platform_driver()`
- memory resources
- IRQ resources
- named resources
- `platform_get_resource()`
- `devm_platform_ioremap_resource()`
- `platform_get_irq()`
- `platform_get_irq_byname()`
- `platform_set_drvdata()`
- `platform_get_drvdata()`

## Mental Model

A platform device represents a hardware block that the system description says exists.

```text
Device Tree node
-> platform_device
-> platform_driver match
-> probe(platform_device)
-> resource lookup
```

The platform driver does not scan the hardware. It trusts the firmware/device description, validates the resources it needs, and initializes the device instance.

## When To Use A Platform Driver

Use a platform driver for:

- MMIO blocks described by `reg`
- devices directly instantiated from Device Tree
- SoC internal devices
- board-specific control devices
- simple devices not on I2C, SPI, PCI, USB, or another real bus

Do not use a platform driver for:

- I2C client devices
- SPI devices
- PCI devices
- USB devices
- standard subsystem devices that already have a better driver model

Example:

```dts
adc@0 {
    compatible = "example,spi-adc";
    reg = <0>;
};
```

This is an SPI device, not a platform device, because it is a child of an SPI controller and uses SPI transfers.

## Minimal Device Tree Node

```dts
demo@10000000 {
    compatible = "example,demo-mmio";
    reg = <0x0 0x10000000 0x0 0x1000>;
    interrupts = <GIC_SPI 42 IRQ_TYPE_LEVEL_HIGH>;
    status = "okay";
};
```

This can become a `struct platform_device`.

## Minimal Platform Driver

```c
#include <linux/module.h>
#include <linux/platform_device.h>

struct demo_priv {
    struct device *dev;
};

static int demo_probe(struct platform_device *pdev)
{
    struct demo_priv *priv;

    priv = devm_kzalloc(&pdev->dev, sizeof(*priv), GFP_KERNEL);
    if (!priv)
        return -ENOMEM;

    priv->dev = &pdev->dev;
    platform_set_drvdata(pdev, priv);

    dev_info(&pdev->dev, "platform device probed\n");
    return 0;
}

static void demo_remove(struct platform_device *pdev)
{
    struct demo_priv *priv = platform_get_drvdata(pdev);

    dev_info(priv->dev, "platform device removed\n");
}

static const struct of_device_id demo_of_match[] = {
    { .compatible = "example,demo-mmio" },
    { }
};
MODULE_DEVICE_TABLE(of, demo_of_match);

static struct platform_driver demo_driver = {
    .probe = demo_probe,
    .remove = demo_remove,
    .driver = {
        .name = "demo-mmio",
        .of_match_table = demo_of_match,
    },
};
module_platform_driver(demo_driver);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Minimal platform driver");
```

## Resource Lookup

### MMIO Resource

Device Tree:

```dts
reg = <0x0 0x10000000 0x0 0x1000>;
```

Driver:

```c
priv->base = devm_platform_ioremap_resource(pdev, 0);
if (IS_ERR(priv->base))
    return PTR_ERR(priv->base);
```

This helper:

- gets the memory resource
- requests the region
- maps it into kernel virtual address space
- ties cleanup to the device lifecycle

Use MMIO accessors later:

```c
val = readl(priv->base + DEMO_STATUS);
writel(DEMO_ENABLE, priv->base + DEMO_CTRL);
```

### Named MMIO Resource

Device Tree:

```dts
reg-names = "control", "data";
reg = <0x0 0x10000000 0x0 0x1000>,
      <0x0 0x10001000 0x0 0x1000>;
```

Driver:

```c
priv->ctrl = devm_platform_ioremap_resource_byname(pdev, "control");
if (IS_ERR(priv->ctrl))
    return PTR_ERR(priv->ctrl);
```

Names are clearer when a device has multiple address ranges.

### IRQ Resource

Device Tree:

```dts
interrupts = <GIC_SPI 42 IRQ_TYPE_LEVEL_HIGH>;
```

Driver:

```c
irq = platform_get_irq(pdev, 0);
if (irq < 0)
    return dev_err_probe(&pdev->dev, irq, "failed to get irq\n");
```

Named IRQ:

```dts
interrupt-names = "rx", "tx";
interrupts = <GIC_SPI 42 IRQ_TYPE_LEVEL_HIGH>,
             <GIC_SPI 43 IRQ_TYPE_LEVEL_HIGH>;
```

Driver:

```c
irq = platform_get_irq_byname(pdev, "rx");
```

## IRQ Registration Example

```c
static irqreturn_t demo_irq_thread(int irq, void *data)
{
    struct demo_priv *priv = data;

    dev_dbg(priv->dev, "interrupt handled\n");
    return IRQ_HANDLED;
}

static int demo_probe(struct platform_device *pdev)
{
    struct demo_priv *priv;
    int irq;
    int ret;

    priv = devm_kzalloc(&pdev->dev, sizeof(*priv), GFP_KERNEL);
    if (!priv)
        return -ENOMEM;

    priv->dev = &pdev->dev;
    platform_set_drvdata(pdev, priv);

    irq = platform_get_irq(pdev, 0);
    if (irq < 0)
        return dev_err_probe(&pdev->dev, irq, "failed to get irq\n");

    ret = devm_request_threaded_irq(&pdev->dev, irq,
                                    NULL, demo_irq_thread,
                                    IRQF_ONESHOT,
                                    dev_name(&pdev->dev), priv);
    if (ret)
        return dev_err_probe(&pdev->dev, ret, "failed to request irq\n");

    return 0;
}
```

This uses a threaded interrupt handler so potentially sleepable work can happen in thread context. Real drivers must still understand interrupt context and hardware acknowledgement rules.

## Per-Device State

Store all instance-specific state in a private structure:

```c
struct demo_priv {
    struct device *dev;
    void __iomem *base;
    int irq;
    struct mutex lock;
};
```

Initialize:

```c
mutex_init(&priv->lock);
platform_set_drvdata(pdev, priv);
```

Retrieve:

```c
struct demo_priv *priv = platform_get_drvdata(pdev);
```

This lets one driver support multiple hardware instances.

## Platform Driver Registration Helpers

Manual form:

```c
static int __init demo_init(void)
{
    return platform_driver_register(&demo_driver);
}

static void __exit demo_exit(void)
{
    platform_driver_unregister(&demo_driver);
}

module_init(demo_init);
module_exit(demo_exit);
```

Common helper:

```c
module_platform_driver(demo_driver);
```

Use the helper when init and exit only register and unregister one platform driver.

## Match Data For Variants

Device Tree:

```dts
compatible = "example,demo-v2";
```

Driver:

```c
struct demo_variant {
    u32 fifo_depth;
    bool has_dma;
};

static const struct demo_variant demo_v1 = {
    .fifo_depth = 16,
    .has_dma = false,
};

static const struct demo_variant demo_v2 = {
    .fifo_depth = 64,
    .has_dma = true,
};

static const struct of_device_id demo_of_match[] = {
    { .compatible = "example,demo-v1", .data = &demo_v1 },
    { .compatible = "example,demo-v2", .data = &demo_v2 },
    { }
};
MODULE_DEVICE_TABLE(of, demo_of_match);
```

Probe:

```c
priv->variant = of_device_get_match_data(&pdev->dev);
if (!priv->variant)
    return -EINVAL;
```

Use match data for hardware differences that are tied to compatible strings. Use Device Tree properties for board wiring and configuration values defined by the binding.

## Platform Device Without Device Tree

Some systems create platform devices from board files, ACPI, MFD children, or other kernel code. The same driver structure still applies, but matching may use:

```c
static const struct platform_device_id demo_id_table[] = {
    { "demo-mmio", 0 },
    { }
};
MODULE_DEVICE_TABLE(platform, demo_id_table);
```

Driver:

```c
static struct platform_driver demo_driver = {
    .probe = demo_probe,
    .remove = demo_remove,
    .id_table = demo_id_table,
    .driver = {
        .name = "demo-mmio",
    },
};
```

For new embedded boards, Device Tree is often the relevant path, but knowing ID tables helps when reading older or non-DT code.

## Debugging Platform Devices

List platform devices:

```sh
ls /sys/bus/platform/devices
```

List platform drivers:

```sh
ls /sys/bus/platform/drivers
```

Inspect a device:

```sh
readlink /sys/bus/platform/devices/<device>/driver
cat /sys/bus/platform/devices/<device>/modalias
```

Check module aliases:

```sh
modinfo demo.ko | grep alias
```

Check logs:

```sh
dmesg | grep -i demo
```

## Common Failure Modes

| Symptom | Likely Cause | First Checks |
| --- | --- | --- |
| No platform device exists | DT node missing/disabled or wrong bus placement | `/proc/device-tree`, `/sys/bus/platform/devices` |
| Device exists but no driver binds | compatible mismatch, driver not loaded, missing alias | `modalias`, `modinfo`, `.config` |
| MMIO map fails | bad `reg`, address conflict, wrong cells | binding, parent bus cells |
| IRQ lookup fails | missing/invalid interrupt specifier | `interrupts`, interrupt parent |
| Probe defers | provider dependency missing | clocks, regulators, resets, pinctrl |
| Multiple devices corrupt state | global state used for per-device data | private struct and drvdata |

## Common Mistakes

- Using a platform driver for an I2C or SPI child device.
- Hard-coding register addresses instead of using resources.
- Forgetting `MODULE_DEVICE_TABLE()` for module autoloading.
- Omitting `platform_set_drvdata()` and then relying on globals.
- Requesting IRQs before private state is fully initialized.
- Mapping registers with ad hoc `ioremap()` instead of resource helpers.
- Ignoring named resources when multiple resources exist.

## Practice Exercises

### Exercise 1: Create A Dummy Platform Match

Add a DT node:

```dts
demo {
    compatible = "example,demo-device";
    status = "okay";
};
```

Write a platform driver that logs `probe()`.

### Exercise 2: Add A Memory Resource

Use a harmless reserved test region only in a lab environment, then read the resource with:

```c
platform_get_resource(pdev, IORESOURCE_MEM, 0);
```

Do not access real hardware registers without documentation.

### Exercise 3: Add Match Data

Create two compatible strings with different `.data` values and print the selected variant in probe.

## Debugging Checklist

- Is the node in the runtime Device Tree?
- Is it enabled?
- Does it become a platform device?
- Does the driver have the exact compatible string?
- Does `modinfo` show the alias?
- Do MMIO and IRQ resources match the binding?
- Are per-device resources stored in private state?
- Does unbind/rebind work?

## Related Topics

- [Device Tree Matching From Drivers](device-tree-matching.md)
- [Driver Binding, Probe, And Remove](driver-binding-probe-remove.md)
- [Resource Lookup And Managed Allocation](resource-lookup-and-devm.md)
- [MMIO And Register Access](../memory-and-io/mmio-and-register-access.md)
- [IRQ Handling](../driver-interfaces/irq-handling.md)

## Official References

- [Platform Devices and Drivers](https://docs.kernel.org/driver-api/driver-model/platform.html)
- [Driver Model](https://docs.kernel.org/driver-api/driver-model/index.html)
- [Linux and the Devicetree](https://docs.kernel.org/devicetree/usage-model.html)

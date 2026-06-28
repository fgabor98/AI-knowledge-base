---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# Driver Binding, Probe, And Remove

## What Problem Does This Solve?

The Linux device model decides which driver owns a device and calls the driver when hardware becomes available or goes away.

This is the core difference between a simple module and a real driver:

```text
module init registers a driver
device model matches device to driver
probe initializes one device instance
remove tears down that device instance
```

`probe()` is not generic module initialization. It is per-device initialization after a concrete `struct device` has been matched to your driver.

## Core Concepts

- `struct device`
- `struct device_driver`
- bus type
- device object
- driver object
- bus matching
- `probe()`
- `remove()`
- `shutdown()`
- deferred probe
- `-EPROBE_DEFER`
- driver data
- `dev_set_drvdata()`
- `dev_get_drvdata()`
- `platform_set_drvdata()`
- `platform_get_drvdata()`
- device-managed cleanup
- bind and unbind
- modalias

## Mental Model

The kernel does not call `probe()` because your module loaded. It calls `probe()` because a bus found a device-driver match.

```text
device exists
+ driver registered
+ bus match function says yes
-> probe(device)
```

The order can vary:

```text
driver registers first, device appears later
device exists first, driver registers later
both already exist during boot
```

A correct driver works in all three cases.

## Device, Driver, And Bus

Every driver belongs to a bus type, broadly speaking:

| Bus | Device Type | Driver Type |
| --- | --- | --- |
| platform | `struct platform_device` | `struct platform_driver` |
| I2C | `struct i2c_client` | `struct i2c_driver` |
| SPI | `struct spi_device` | `struct spi_driver` |
| PCI | `struct pci_dev` | `struct pci_driver` |
| USB | `struct usb_interface` | `struct usb_driver` |

The bus owns matching rules. For Device Tree platform devices, matching often uses `compatible`. For I2C and SPI, matching may use Device Tree, ACPI, board info, or ID tables depending on platform.

## Probe Responsibilities

`probe()` should normally:

1. Allocate per-device state.
2. Store the state with driver data.
3. Read hardware resources from the device.
4. Acquire resources through kernel APIs.
5. Initialize hardware enough to make it safe.
6. Register the subsystem/user interface.
7. Enable runtime operation.

Example:

```c
static int demo_probe(struct platform_device *pdev)
{
    struct demo_priv *priv;

    priv = devm_kzalloc(&pdev->dev, sizeof(*priv), GFP_KERNEL);
    if (!priv)
        return -ENOMEM;

    priv->dev = &pdev->dev;
    platform_set_drvdata(pdev, priv);

    dev_info(&pdev->dev, "probed\n");
    return 0;
}
```

Do not:

- assume only one device instance exists
- hard-code global state for per-device resources
- create userspace ABI before resources are valid
- start interrupts before state is initialized
- ignore errors and continue with partial hardware setup

## Remove Responsibilities

`remove()` should stop the device instance from being used and release what is not managed automatically.

Typical responsibilities:

- unregister user-facing interfaces
- stop new I/O
- disable interrupts or make handlers harmless
- cancel timers and work
- stop hardware
- put device into safe state
- release unmanaged resources

Example:

```c
static void demo_remove(struct platform_device *pdev)
{
    struct demo_priv *priv = platform_get_drvdata(pdev);

    dev_info(priv->dev, "removed\n");
}
```

If all resources are managed and no asynchronous work remains, `remove()` may be short. That does not mean cleanup is optional. You must still stop active paths in the correct order.

## Driver Data

Per-device state should be stored in a private structure:

```c
struct demo_priv {
    struct device *dev;
    void __iomem *base;
    int irq;
    struct mutex lock;
};
```

Store it:

```c
platform_set_drvdata(pdev, priv);
```

Retrieve it later:

```c
priv = platform_get_drvdata(pdev);
```

For generic `struct device *`:

```c
dev_set_drvdata(dev, priv);
priv = dev_get_drvdata(dev);
```

Avoid file-scope globals unless the state is genuinely global to the driver and not per-device.

## Deferred Probe

Probe can run before provider drivers are ready. For example, your driver may need:

- regulator provider
- clock controller
- reset controller
- GPIO controller
- pinctrl provider
- PHY provider
- power domain provider

If a provider is not ready, helpers may return `-EPROBE_DEFER`.

Correct pattern:

```c
priv->vdd = devm_regulator_get(&pdev->dev, "vdd");
if (IS_ERR(priv->vdd))
    return dev_err_probe(&pdev->dev, PTR_ERR(priv->vdd),
                         "failed to get vdd regulator\n");
```

`dev_err_probe()` avoids noisy logs for expected deferrals and reports useful device-scoped errors.

Do not convert `-EPROBE_DEFER` to `-EINVAL` or `-ENODEV`. Return it upward.

## Matching And Modalias

When a device is created, it often exposes a modalias:

```sh
find /sys -name modalias -exec grep -H . {} \; 2>/dev/null | head
```

For modules, aliases let userspace load the right module:

```sh
modinfo demo.ko | grep alias
```

For Device Tree drivers, ensure:

```c
MODULE_DEVICE_TABLE(of, demo_of_match);
```

Without it, manual loading may still work, but autoloading may fail.

## Bind And Unbind From Sysfs

Many buses expose bind/unbind controls:

```text
/sys/bus/platform/drivers/<driver>/bind
/sys/bus/platform/drivers/<driver>/unbind
```

List devices:

```sh
ls /sys/bus/platform/devices
ls /sys/bus/platform/drivers/demo
```

Unbind:

```sh
echo 10000000.demo | sudo tee /sys/bus/platform/drivers/demo/unbind
```

Bind:

```sh
echo 10000000.demo | sudo tee /sys/bus/platform/drivers/demo/bind
```

The device name is bus-specific. Use sysfs to find the exact name.

Bind/unbind is useful for development, but not every driver/hardware combination can survive repeated cycles unless remove/probe paths are correct.

## Probe Ordering Versus Resource Ordering

Do not solve resource readiness by arbitrary sleeps:

```c
msleep(1000); /* bad dependency handling */
```

Use provider/consumer APIs and return `-EPROBE_DEFER` when dependencies are not ready. The device model will retry.

Example:

```c
priv->clk = devm_clk_get(&pdev->dev, "core");
if (IS_ERR(priv->clk))
    return dev_err_probe(&pdev->dev, PTR_ERR(priv->clk),
                         "failed to get core clock\n");
```

## Probe Failure Cleanup

With managed resources:

```c
priv = devm_kzalloc(&pdev->dev, sizeof(*priv), GFP_KERNEL);
base = devm_platform_ioremap_resource(pdev, 0);
ret = devm_request_threaded_irq(...);
```

the device core releases those resources automatically when probe fails or the device is removed.

But not everything is solved by `devm_*`. You still must handle:

- hardware state
- active IRQ generation
- queued work
- registered subsystem objects
- userspace-visible interfaces
- locks and active file operations

If you enable hardware halfway through probe and then fail, turn it off before returning or register a managed cleanup action.

## Full Minimal Platform Driver Flow

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

    dev_info(&pdev->dev, "probe complete\n");
    return 0;
}

static void demo_remove(struct platform_device *pdev)
{
    struct demo_priv *priv = platform_get_drvdata(pdev);

    dev_info(priv->dev, "remove complete\n");
}

static const struct of_device_id demo_of_match[] = {
    { .compatible = "example,demo-device" },
    { }
};
MODULE_DEVICE_TABLE(of, demo_of_match);

static struct platform_driver demo_driver = {
    .probe = demo_probe,
    .remove = demo_remove,
    .driver = {
        .name = "demo",
        .of_match_table = demo_of_match,
    },
};
module_platform_driver(demo_driver);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Probe/remove example");
```

## Common Failure Modes

| Symptom | Likely Cause | First Checks |
| --- | --- | --- |
| Module loads but `probe()` never runs | no device matched | runtime DT, sysfs devices, aliases |
| Probe logs repeat with defer | provider not ready or missing | `dmesg`, provider config/nodes |
| Remove hangs | active users, work, IRQ, or lock problem | `lsmod`, traces, driver state |
| Rebind fails after unbind | remove did not restore hardware state | reset/clock/IRQ cleanup |
| Autoload fails | missing `MODULE_DEVICE_TABLE()` or depmod | `modinfo alias`, modalias |
| Probe sees wrong device data | global state or wrong match data | per-device `priv`, match table |

## Common Mistakes

- Doing per-device resource acquisition in module init.
- Assuming only one instance of a device can exist.
- Using globals for MMIO base or IRQ state.
- Returning a generic error instead of preserving `-EPROBE_DEFER`.
- Starting hardware before all state and handlers are ready.
- Registering userspace interfaces before hardware initialization succeeds.
- Trusting `devm_*` to stop asynchronous callbacks automatically.
- Forgetting to test unbind/rebind.

## Practice Exercises

### Exercise 1: Prove Probe Is Per Device

Create two matching dummy platform nodes with different unit addresses. Add logs:

```c
dev_info(&pdev->dev, "probe %s\n", dev_name(&pdev->dev));
```

Confirm `probe()` runs once per device.

### Exercise 2: Test Bind And Unbind

Find the platform device and driver:

```sh
ls /sys/bus/platform/devices
ls /sys/bus/platform/drivers
```

Use `unbind` and `bind`, then inspect logs.

### Exercise 3: Observe Deferred Probe

Temporarily reference a missing regulator in Device Tree and watch probe defer or fail. Restore the provider afterward.

## Debugging Checklist

- Does the device exist under the expected bus in sysfs?
- Did the driver register?
- Does the bus have matching data?
- Does `modinfo` show the expected alias for modules?
- Does `dmesg` show `-EPROBE_DEFER`?
- Are provider drivers enabled and their nodes present?
- Is per-device state stored with driver data?
- Does remove stop hardware and asynchronous paths?
- Can the driver survive unbind/rebind?

## Related Topics

- [Platform Devices And Platform Drivers](platform-devices-and-drivers.md)
- [Device Tree Matching From Drivers](device-tree-matching.md)
- [Resource Lookup And Managed Allocation](resource-lookup-and-devm.md)
- [Probe Failure Debugging](../debugging/probe-failure-debugging.md)
- [Reference Counting And Lifetime](../execution-and-concurrency/reference-counting-and-lifetime.md)

## Official References

- [Driver Model](https://docs.kernel.org/driver-api/driver-model/index.html)
- [Driver Basics](https://docs.kernel.org/driver-api/basics.html)
- [Platform Devices and Drivers](https://docs.kernel.org/driver-api/driver-model/platform.html)

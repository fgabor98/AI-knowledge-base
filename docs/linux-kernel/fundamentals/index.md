---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# Linux Device Driver Fundamentals

This track covers the first complete mental model for Linux driver work: how driver code enters the kernel, how it binds to hardware, how it acquires resources, how it exposes a controlled userspace interface when needed, and how it cleans up.

It assumes you already know the material in:

- [Kernel Foundations For Driver Developers](../foundations/index.md)
- [Kernel Source, Build, And Tailoring](../source-build-and-tailoring/index.md)

## What Problem Does This Solve?

Beginners often start by writing a module that logs `hello world`, then jump directly to a real device. The missing bridge is the Linux device model:

```text
firmware or bus describes a device
-> kernel creates struct device
-> bus matches device to driver
-> driver probe runs for one concrete device instance
-> driver acquires resources
-> driver registers a subsystem or userspace interface
-> remove/shutdown/suspend paths unwind safely
```

This track teaches that bridge. It focuses on concepts that appear in almost every real embedded driver:

- module and built-in driver lifecycle
- Device Tree as hardware description
- platform devices and platform drivers
- `probe()` and `remove()`
- `compatible` matching
- resource lookup
- `devm_*` managed cleanup
- character devices and sysfs
- uevents, classes, and `/dev` nodes
- module parameters and driver logging
- deciding when userspace bus access is only a prototype

## Learning Materials

1. [Kernel Module Lifecycle](kernel-module-lifecycle.md)
2. [Built-In Drivers Vs Loadable Modules](built-in-vs-loadable-modules.md)
3. [Device Tree Hardware Description](device-tree-hardware-description.md)
4. [Device Tree Overlays](device-tree-overlays.md)
5. [Driver Binding, Probe, And Remove](driver-binding-probe-remove.md)
6. [Platform Devices And Platform Drivers](platform-devices-and-drivers.md)
7. [Device Tree Matching From Drivers](device-tree-matching.md)
8. [Resource Lookup And Managed Allocation](resource-lookup-and-devm.md)
9. [Character Device Basics](character-device-basics.md)
10. [Device Classes, Uevents, And udev](device-classes-uevents-and-udev.md)
11. [Sysfs Attributes](sysfs-attributes.md)
12. [Kobjects And Sysfs Groups](kobjects-and-sysfs-groups.md)
13. [Pollable Sysfs Attributes](pollable-sysfs-attributes.md)
14. [Module Parameters And Driver Logging](module-parameters-and-logging.md)
15. [User-Space Hardware Access Vs Kernel Drivers](userspace-hardware-access-vs-kernel-drivers.md)

## Mental Model

A practical embedded Linux driver usually has four connected surfaces:

```text
hardware description
-> kernel device object
-> driver binding and probe
-> kernel subsystem or userspace ABI
```

The driver should not guess board details. Board wiring belongs in firmware data such as Device Tree. The driver should bind through the kernel device model, request resources through kernel APIs, register with the right subsystem, and release or quiesce everything deterministically.

## The Minimal Shape Of A Real Driver

A simple platform driver usually has this shape:

```c
#include <linux/module.h>
#include <linux/of.h>
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

    dev_info(&pdev->dev, "demo device probed\n");
    return 0;
}

static void demo_remove(struct platform_device *pdev)
{
    dev_info(&pdev->dev, "demo device removed\n");
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
MODULE_DESCRIPTION("Minimal platform driver skeleton");
```

A matching Device Tree node might look like:

```dts
demo@10000000 {
    compatible = "example,demo-device";
    reg = <0x10000000 0x1000>;
    interrupts = <42>;
    status = "okay";
};
```

This skeleton is not enough for hardware control, but it demonstrates the core relationship:

```text
compatible string in DT
-> of_device_id table in driver
-> platform bus match
-> probe gets one platform_device
```

## Driver Responsibilities

A driver normally owns these responsibilities for each device instance:

| Responsibility | Example |
| --- | --- |
| Identify supported hardware | `of_match_table`, bus IDs, ACPI IDs |
| Acquire resources | MMIO, IRQ, GPIOs, clocks, regulators, resets |
| Initialize hardware | reset sequence, register programming, firmware load |
| Register an interface | subsystem device, character device, sysfs attributes |
| Handle runtime events | interrupts, workqueues, polling, errors |
| Manage power | runtime PM, suspend/resume, wakeup policy |
| Serialize access | mutexes, spinlocks, refcounts where needed |
| Clean up safely | disable IRQs, stop work, unregister interfaces |

This fundamentals track covers the first half. Later tracks cover bus-specific APIs, interrupts, concurrency, memory, MMIO, DMA, debugging, and power management.

## What Belongs Where?

Use the right interface for the job:

| Need | Usual Kernel Mechanism |
| --- | --- |
| Board wiring and fixed hardware facts | Device Tree |
| Device instance lifetime | device model, bus, `probe()`/`remove()` |
| Memory-mapped registers | platform resources, `ioremap`, regmap |
| Interrupts | IRQ APIs, threaded IRQs |
| Simple per-device state | sysfs attributes |
| Stream or command-style userspace access | character device or subsystem ABI |
| Debug-only inspection | debugfs, tracepoints, dynamic debug |
| Standard sensor/ADC interface | IIO |
| Buttons/keys | input subsystem |
| LEDs | LED subsystem |
| Networking | netdev |
| Sound | ALSA/ASoC |
| Display | DRM |

Do not invent a private character device or sysfs ABI when a kernel subsystem already provides the expected userspace contract.

## Suggested Study Path

Follow this order for a first real driver:

```text
external module lifecycle
-> built-in vs module choice
-> Device Tree node
-> platform driver match
-> probe/remove
-> resource lookup with devm helpers
-> logging and parameters
-> minimal sysfs or character device
-> subsystem-specific APIs
```

For an embedded board, practice with a harmless dummy node first. Then move to a real simple device such as:

- GPIO-controlled LED or reset line
- simple I2C sensor
- SPI EEPROM-like device
- platform MMIO block in an FPGA or SoC
- IRQ-backed push button

## Completion Criteria

You are ready to move on when you can:

- build and load a minimal external module
- explain why some drivers must be built in
- identify the runtime kernel and module compatibility using `uname -r` and `modinfo`
- write a simple Device Tree node with `compatible`, `reg`, `interrupts`, and named resources
- explain how a `compatible` string reaches a driver's `probe()`
- register a minimal platform driver
- store per-device state with `platform_set_drvdata()`
- request resources with managed helpers
- explain why `-EPROBE_DEFER` is not a normal fatal error
- create a simple character device or sysfs attribute
- explain the difference between sysfs, debugfs, character devices, and subsystem ABIs
- use `dev_*()` logging and rate-limited logging appropriately
- decide when userspace GPIO/I2C/SPI access should become a kernel driver

## Common Mistakes

- Treating module init as if it were the same as per-device initialization.
- Hard-coding addresses, IRQs, or GPIO numbers in the driver.
- Adding a Device Tree node but forgetting the driver Kconfig symbol.
- Matching the wrong `compatible` string or omitting `MODULE_DEVICE_TABLE()`.
- Doing too much global work before a concrete device exists.
- Leaking resources across `probe()` failure paths.
- Depending only on `devm_*` while leaving interrupts, workqueues, or hardware running.
- Exposing unstable internal state as a permanent userspace ABI.
- Using sysfs for high-rate data streams.
- Shipping `spidev`, `i2c-dev`, or raw GPIO access as the product driver interface without a clear policy.

## Related Topics

- [Common Driver Interfaces](../driver-interfaces/index.md)
- [Kernel Execution And Concurrency](../execution-and-concurrency/index.md)
- [Kernel Memory And I/O](../memory-and-io/index.md)
- [Kernel Debugging Basics](../debugging/index.md)
- [Linux Kernel Build System](../../build-systems/advanced/linux-kernel/index.md)
- [Device Tree](../../device-tree/index.md)

## Official References

- [Driver Basics](https://docs.kernel.org/driver-api/basics.html)
- [Driver Model](https://docs.kernel.org/driver-api/driver-model/index.html)
- [Linux and the Devicetree](https://docs.kernel.org/devicetree/usage-model.html)
- [Building External Modules](https://docs.kernel.org/kbuild/modules.html)

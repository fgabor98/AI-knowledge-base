---
status: draft
reviewed: false
domain: linux-kernel
difficulty: beginner
last_reviewed: null
---

# Device Model Primer

## What Problem Does This Solve?

The Linux device model explains how devices, drivers, buses, classes, sysfs, uevents, and userspace device nodes fit together. Without it, driver failures are confusing: a module may load but never probe, a device may exist but have no driver, or a driver may probe successfully but no `/dev` node appears.

This page gives the beginner map.

## Core Concepts

- device
- driver
- bus
- class
- subsystem
- probe
- remove
- modalias
- sysfs
- uevent
- udev
- bind
- unbind
- Device Tree matching
- module autoloading

## The Basic Model

```text
device object
-> appears on a bus
-> bus compares device identity with registered drivers
-> matching driver probes device
-> driver registers with subsystem
-> sysfs and userspace ABI appear
```

Important distinction:

- A **device** represents hardware or virtual hardware.
- A **driver** is code that can manage compatible devices.
- A **bus** defines how devices and drivers match.
- A **class** groups devices by userspace function.

## Device

A device object is represented by `struct device`.

Examples:

- platform device from Device Tree
- I2C client at address `0x48`
- SPI device on chip select 0
- PCI device discovered by hardware enumeration
- USB device discovered during USB enumeration

Inspect devices:

```bash
find /sys/devices -maxdepth 3 -type d | head
find /sys/bus/platform/devices -maxdepth 1 -print
find /sys/bus/i2c/devices -maxdepth 1 -print
find /sys/bus/spi/devices -maxdepth 1 -print
```

## Driver

A driver object says: "I know how to manage devices matching these IDs."

Platform driver example:

```c
static struct platform_driver demo_driver = {
        .probe = demo_probe,
        .remove = demo_remove,
        .driver = {
                .name = "demo",
                .of_match_table = demo_of_match,
        },
};
```

Inspect platform drivers:

```bash
find /sys/bus/platform/drivers -maxdepth 1 -type d | sort
```

## Bus

A bus owns matching rules.

Examples:

| Bus | Device identity |
|---|---|
| platform | firmware data, name, Device Tree compatible |
| I2C | bus number and 7-bit address, OF/ACPI/ID tables |
| SPI | controller, chip select, OF/ACPI/ID tables |
| PCI | vendor/device IDs |
| USB | vendor/product/interface IDs |

Inspect bus layout:

```bash
ls /sys/bus
ls /sys/bus/platform/devices
ls /sys/bus/platform/drivers
```

## Class

A class groups devices by function for userspace.

Examples:

```bash
ls /sys/class
ls /sys/class/gpio
ls /sys/class/input
ls /sys/class/net
ls /sys/class/tty
```

A character driver may create a class device so userspace gets a `/dev` node:

```c
demo->class = class_create("demo");
device_create(demo->class, NULL, devt, NULL, "demo0");
```

Then userspace may see:

```text
/sys/class/demo/demo0
/dev/demo0
```

## Probe And Remove

`probe` runs when the bus has matched a device to a driver.

`remove` runs when the device is unbound or removed, or when the driver is unloaded for a bound module.

Conceptual lifecycle:

```text
driver registers
-> bus finds matching devices
-> probe(device)
-> runtime callbacks
-> remove(device)
-> driver unregisters
```

`probe` should:

- allocate per-device state
- acquire resources
- initialize hardware
- register with subsystem
- store driver data

`remove` should:

- stop new operations
- stop hardware
- cancel timers/work
- unregister subsystem objects
- let managed resources unwind safely

## Matching

### Device Tree Match

Device Tree:

```dts
demo@48000000 {
        compatible = "example,demo";
        reg = <0x48000000 0x1000>;
};
```

Driver:

```c
static const struct of_device_id demo_of_match[] = {
        { .compatible = "example,demo" },
        { }
};
MODULE_DEVICE_TABLE(of, demo_of_match);
```

Inspection:

```bash
tr '\0' '\n' < /proc/device-tree/demo@48000000/compatible
modinfo demo.ko | grep of:
```

### I2C Match

Device Tree:

```dts
sensor@48 {
        compatible = "example,demo-sensor";
        reg = <0x48>;
};
```

Runtime device:

```bash
ls /sys/bus/i2c/devices/
cat /sys/bus/i2c/devices/1-0048/modalias
```

### Platform Name Match

Some old or non-DT platform devices match by name:

```c
static struct platform_driver demo_driver = {
        .driver = {
                .name = "demo",
        },
};
```

This is less descriptive than firmware-based matching but still appears in examples and legacy code.

## Modalias And Module Autoloading

`modalias` describes the device identity in a form module tools can match to module aliases.

Device:

```bash
cat /sys/bus/platform/devices/48000000.demo/modalias
```

Module:

```bash
modinfo demo.ko | grep alias
```

If aliases line up and the module is installed under `/lib/modules/$(uname -r)`, userspace module loading may automatically load the module.

Common autoload failure:

- driver has `of_match_table`
- but missing `MODULE_DEVICE_TABLE(of, demo_of_match)`
- module loads manually but not automatically

## Sysfs View Of Binding

A bound device usually has a `driver` symlink:

```bash
readlink /sys/bus/platform/devices/48000000.demo/driver
```

Driver directory may list bound devices:

```bash
ls -l /sys/bus/platform/drivers/demo/
```

Common files:

```text
bind
unbind
uevent
driver_override
```

Manual bind:

```bash
echo 48000000.demo | sudo tee /sys/bus/platform/drivers/demo/bind
```

Manual unbind:

```bash
echo 48000000.demo | sudo tee /sys/bus/platform/drivers/demo/unbind
```

Use only in a controlled lab. Unbinding storage, display, power, or active bus-controller drivers can break the system.

## Uevents And udev

The kernel emits uevents when devices are added, removed, or changed. Userspace `udev` can create device nodes, set permissions, and create symlinks.

Inspect:

```bash
udevadm monitor --kernel --udev
udevadm info /dev/demo0
```

Important split:

- kernel creates device/class/subsystem state
- userspace policy creates names, permissions, and symlinks

If `/dev/demo0` is missing, first confirm the kernel class device exists:

```bash
find /sys/class -maxdepth 3 -name '*demo*'
```

## End-To-End Example: Platform Character Device

```text
Device Tree node
-> platform device
-> platform bus match
-> demo_probe()
-> alloc_chrdev_region()
-> cdev_add()
-> class_create()
-> device_create()
-> uevent
-> udev creates /dev/demo0
```

Inspection path:

```bash
find /sys/bus/platform/devices -name '*demo*'
readlink /sys/bus/platform/devices/48000000.demo/driver
find /sys/class -name '*demo*'
ls -l /dev/demo0
udevadm info /dev/demo0
```

If `/dev/demo0` is missing:

1. Did the platform device exist?
2. Did the driver bind?
3. Did `probe` succeed?
4. Did class/device creation succeed?
5. Did udev process the event?

## Common Mistakes

- Confusing loaded module with bound driver.
- Confusing device existence with probe success.
- Debugging udev before checking sysfs.
- Forgetting `MODULE_DEVICE_TABLE`.
- Using the wrong bus type.
- Expecting a `/dev` node for a subsystem that exposes sysfs or netdev instead.
- Manually binding dangerous devices on a live system.

## Debugging Checklist

- Does the device object exist?
- Which bus owns it?
- What is its modalias?
- Is the driver registered?
- Does the driver's alias match?
- Is there a `driver` symlink?
- Did `probe` log success?
- What subsystem object should appear after probe?
- Is userspace policy needed to create a `/dev` node?

## Related Topics

- [Driver Binding, Probe, And Remove](../fundamentals/driver-binding-probe-remove.md)
- [Platform Devices And Platform Drivers](../fundamentals/platform-devices-and-drivers.md)
- [Device Classes, Uevents, And udev](../fundamentals/device-classes-uevents-and-udev.md)
- [Device Tree Matching From Drivers](../fundamentals/device-tree-matching.md)

## References

- Linux Driver Model: <https://docs.kernel.org/driver-api/driver-model/index.html>
- Driver binding: <https://docs.kernel.org/driver-api/driver-model/binding.html>
- Infrastructure for device drivers: <https://docs.kernel.org/driver-api/infrastructure.html>

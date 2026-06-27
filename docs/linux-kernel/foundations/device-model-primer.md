---
status: draft
reviewed: false
domain: linux-kernel
difficulty: beginner
last_reviewed: null
---

# Device Model Primer

## What Problem Does This Solve?

The Linux device model explains how devices, drivers, buses, classes, sysfs, and udev fit together.

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

## Mental Model

The kernel does not just call a driver directly. A bus matches a device to a driver, the driver probes the device, and the resulting objects appear through sysfs and possibly userspace device nodes.

```text
firmware or bus discovery
-> device object
-> bus match
-> driver probe
-> subsystem registration
-> sysfs, uevents, and userspace ABI
```

## Practice Skeleton

- Find a device under `/sys/devices`.
- Identify its bus and bound driver.
- Inspect its modalias.
- Map a `/dev` node back to the kernel device.

## Debugging Checklist

- Check whether the device object exists.
- Check whether the driver registered.
- Check whether bus matching succeeded.
- Check whether userspace created the expected node or permissions.

## Related Topics

- [Driver Binding, Probe, And Remove](../fundamentals/driver-binding-probe-remove.md)
- [Platform Devices And Platform Drivers](../fundamentals/platform-devices-and-drivers.md)
- [Device Classes, Uevents, And udev](../fundamentals/device-classes-uevents-and-udev.md)

---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# Driver Binding, Probe, And Remove

## What Problem Does This Solve?

The kernel device model decides which driver owns a device and calls the driver when hardware becomes available or goes away.

## Core Concepts

- device object
- driver object
- bus matching
- `probe`
- `remove`
- deferred probe
- device-managed cleanup
- driver data
- bind and unbind

## Mental Model

`probe` is not generic initialization. It is the point where a specific driver is matched to a specific device instance and can safely request that device's resources.

## Practice Skeleton

- Register a minimal platform driver.
- Add a probe log with device identity.
- Store private driver data.
- Trigger bind and unbind from sysfs where supported.

## Debugging Checklist

- Check whether the device exists.
- Check whether the driver registered.
- Check matching data and modalias.
- Look for `-EPROBE_DEFER`.
- Confirm `remove` does not race active callbacks.

## Related Topics

- [Platform Devices And Platform Drivers](platform-devices-and-drivers.md)
- [Device Tree Matching From Drivers](device-tree-matching.md)
- [Resource Lookup And Managed Allocation](resource-lookup-and-devm.md)

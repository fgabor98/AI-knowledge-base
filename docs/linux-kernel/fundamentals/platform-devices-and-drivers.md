---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# Platform Devices And Platform Drivers

## What Problem Does This Solve?

Platform drivers handle memory-mapped and board-described devices that are not discoverable through buses such as PCI or USB.

## Core Concepts

- platform bus
- `struct platform_device`
- `struct platform_driver`
- firmware-created devices
- memory resources
- IRQ resources
- `platform_get_resource`
- `platform_get_irq`
- `platform_set_drvdata`

## Mental Model

A platform device represents a hardware block that firmware or board code described. The platform driver binds to that description and requests the resources needed to control the block.

## Practice Skeleton

- Add a minimal Device Tree node for a simple platform device.
- Match it with a platform driver.
- Retrieve MMIO and IRQ resources.
- Store per-device state with driver data.

## Debugging Checklist

- Confirm the node exists in the runtime Device Tree.
- Confirm the `compatible` string matches the driver table.
- Check resource names, address ranges, and IRQ cells.
- Check for probe deferral due to missing providers.

## Related Topics

- [Device Tree Matching From Drivers](device-tree-matching.md)
- [MMIO And Register Access](../memory-and-io/mmio-and-register-access.md)
- [IRQ Handling](../driver-interfaces/irq-handling.md)

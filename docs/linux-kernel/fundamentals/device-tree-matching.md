---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# Device Tree Matching From Drivers

## What Problem Does This Solve?

Device Tree matching connects board-level hardware descriptions to reusable driver code without hard-coding board details in the driver.

## Core Concepts

- `compatible`
- fallback compatible strings
- `of_match_table`
- `MODULE_DEVICE_TABLE`
- match data
- firmware node
- required properties
- optional properties
- binding documentation

## Mental Model

The Device Tree describes what hardware exists and how it is wired. The driver advertises which compatible devices it supports and reads only the properties defined by the binding.

## Practice Skeleton

- Add an `of_device_id` table.
- Attach driver-specific match data.
- Read required and optional properties.
- Fail clearly when required resources are missing.

## Debugging Checklist

- Compare source DTS with `/proc/device-tree`.
- Check the exact compatible string order.
- Confirm `MODULE_DEVICE_TABLE(of, ...)` exists for modules.
- Check binding documentation before inventing properties.

## Related Topics

- [Device Tree](../../device-tree/index.md)
- [Platform Devices And Platform Drivers](platform-devices-and-drivers.md)
- [Device Tree Binding Validation](../../build-systems/advanced/linux-kernel/device-tree-binding-validation.md)

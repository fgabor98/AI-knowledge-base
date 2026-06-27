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

## Practice Skeleton

- Map a register region from a platform resource.
- Read a version register.
- Set and clear a bit field.
- Add named constants for register offsets and masks.

## Debugging Checklist

- Check resource address and size.
- Check register width and alignment.
- Check endianness.
- Avoid direct pointer dereferences into MMIO.

## Related Topics

- [Platform Devices And Platform Drivers](../fundamentals/platform-devices-and-drivers.md)
- [Regmap](../driver-interfaces/regmap.md)
- [Device Tree](../../device-tree/index.md)

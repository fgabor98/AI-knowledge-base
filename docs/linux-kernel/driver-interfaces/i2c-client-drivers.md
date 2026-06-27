---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# I2C Client Drivers

## What Problem Does This Solve?

I2C client drivers bind to devices on an I2C bus and communicate through the kernel I2C subsystem.

## Core Concepts

- I2C adapter
- I2C client
- `i2c_driver`
- `probe`
- Device Tree matching
- SMBus helpers
- raw I2C transfers
- regmap over I2C

## Mental Model

The bus controller driver moves bytes. The client driver owns the device protocol and should not know controller-specific details.

## Practice Skeleton

- Create a minimal I2C client driver.
- Match through Device Tree.
- Read an identification register.
- Convert register access to regmap.

## Debugging Checklist

- Confirm the device exists under the expected I2C bus.
- Check the 7-bit address.
- Use `i2cdetect` cautiously on real hardware.
- Check pull-ups, pinmux, and bus speed.

## Related Topics

- [Regmap](regmap.md)
- [Device Tree Matching From Drivers](../fundamentals/device-tree-matching.md)
- [Pinctrl](pinctrl.md)

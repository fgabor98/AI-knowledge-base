---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# Regmap

## What Problem Does This Solve?

Regmap centralizes register access for devices connected through buses such as I2C, SPI, MMIO, or custom transports.

## Core Concepts

- register map configuration
- register width
- value width
- caching
- volatile registers
- precious registers
- bulk reads and writes
- regmap fields

## Mental Model

Regmap lets the driver describe register layout once and then use a consistent access API independent of the transport details.

## Practice Skeleton

- Create an I2C or SPI regmap.
- Read and update one register field.
- Mark volatile registers explicitly.
- Inspect debugfs regmap output where available.

## Debugging Checklist

- Check register and value widths.
- Check endianness.
- Avoid caching status registers incorrectly.
- Validate register ranges and access tables.

## Related Topics

- [I2C Client Drivers](i2c-client-drivers.md)
- [SPI Device Drivers](spi-device-drivers.md)
- [Debugfs And Sysfs Inspection](../debugging/debugfs-and-sysfs-inspection.md)

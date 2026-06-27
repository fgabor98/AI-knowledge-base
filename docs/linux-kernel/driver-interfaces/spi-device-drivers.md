---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# SPI Device Drivers

## What Problem Does This Solve?

SPI device drivers bind to chip-select-addressed devices on an SPI controller and communicate through SPI messages and transfers.

## Core Concepts

- SPI controller
- SPI device
- chip select
- mode
- bits per word
- transfer speed
- `spi_message`
- `spi_transfer`
- regmap over SPI

## Mental Model

The SPI controller handles electrical transfer mechanics. The device driver controls the peripheral protocol, transaction layout, and timing assumptions.

## Practice Skeleton

- Create a minimal SPI device driver.
- Read one device register.
- Validate SPI mode and maximum frequency.
- Convert register access to regmap where appropriate.

## Debugging Checklist

- Check chip select polarity and numbering.
- Check SPI mode.
- Check pinmux and controller enablement.
- Use a logic analyzer for transaction shape.

## Related Topics

- [Regmap](regmap.md)
- [Pinctrl](pinctrl.md)
- [MMIO And Register Access](../memory-and-io/mmio-and-register-access.md)

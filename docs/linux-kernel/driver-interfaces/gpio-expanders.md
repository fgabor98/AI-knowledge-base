---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# GPIO Expanders

## What Problem Does This Solve?

GPIO expanders provide additional GPIO lines through buses such as I2C or SPI.

## Core Concepts

- I2C GPIO expander
- SPI GPIO expander
- `gpio_chip`
- `gpio-controller`
- `#gpio-cells`
- can-sleep GPIOs
- interrupt-capable expanders
- register caching

## Mental Model

A GPIO expander is both a bus client and a GPIO provider. Its driver must respect bus transaction context and expose lines through the GPIO subsystem.

## Practice Skeleton

- Add an I2C expander node in Device Tree.
- Bind an expander driver.
- Verify the new gpiochip appears.
- Consume one expander GPIO from another driver.

## Debugging Checklist

- Check bus address and Device Tree binding.
- Check whether GPIO operations may sleep.
- Check interrupt-controller support separately from GPIO support.
- Check pull-ups and reset lines on the expander.

## Related Topics

- [I2C Client Drivers](i2c-client-drivers.md)
- [GPIO Controller Drivers](gpio-controller-drivers.md)
- [Regmap](regmap.md)

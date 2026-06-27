---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# GPIO Controller Drivers

## What Problem Does This Solve?

GPIO controller drivers expose hardware GPIO lines to the kernel's GPIO subsystem so other drivers can consume them by descriptor.

## Core Concepts

- `gpio_chip`
- GPIO line offset
- direction callbacks
- get and set callbacks
- `ngpio`
- base numbering
- GPIO descriptor consumers
- can-sleep controllers
- irqchip integration overview

## Mental Model

A GPIO controller driver provides lines. Other drivers consume named GPIOs. Do not mix provider and consumer responsibilities without a clear reason.

## Practice Skeleton

- Register a simple `gpio_chip`.
- Implement direction, get, and set callbacks.
- Expose the controller through sysfs or debugfs inspection.
- Consume one line from another driver.

## Debugging Checklist

- Check `ngpio`, label, and line offsets.
- Check whether callbacks can sleep.
- Check pinctrl ownership.
- Avoid relying on global GPIO numbers in new code.

## Related Topics

- [GPIO Consumer API](gpio-consumer-api.md)
- [GPIO Expanders](gpio-expanders.md)
- [Legacy GPIO Interfaces](legacy-gpio-interfaces.md)

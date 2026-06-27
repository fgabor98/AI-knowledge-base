---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# GPIO Consumer API

## What Problem Does This Solve?

The GPIO consumer API lets drivers request and control GPIO lines without depending on board-specific GPIO numbers.

## Core Concepts

- descriptors
- `gpiod_get`
- optional GPIOs
- active-low handling
- direction
- output values
- sleepable GPIO access
- GPIOs from Device Tree

## Mental Model

A driver asks for a named GPIO role. The board description maps that role to a physical line and encodes polarity.

## Practice Skeleton

- Request a required GPIO.
- Request an optional GPIO.
- Toggle an output line.
- Read an input line with active-low semantics handled by the API.

## Debugging Checklist

- Confirm the GPIO property name.
- Check whether the provider GPIO controller probed.
- Use the sleepable accessor when the GPIO provider may sleep.
- Do not manually invert active-low lines.

## Related Topics

- [Device Tree](../../device-tree/index.md)
- [Pinctrl](pinctrl.md)
- [Resource Lookup And Managed Allocation](../fundamentals/resource-lookup-and-devm.md)

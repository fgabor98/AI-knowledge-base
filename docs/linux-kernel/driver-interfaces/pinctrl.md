---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# Pinctrl

## What Problem Does This Solve?

Pinctrl configures pin multiplexing and electrical settings so SoC pins are connected to the intended hardware functions.

## Core Concepts

- pinmux
- pin configuration
- pinctrl states
- default state
- sleep state
- bias
- drive strength
- slew rate

## Mental Model

Drivers select named pinctrl states. Board descriptions define what each state means for the package pins.

## Practice Skeleton

- Define default and sleep pinctrl states.
- Confirm a driver selects the default state.
- Switch states during suspend and resume where relevant.
- Validate pins with a scope or logic analyzer.

## Debugging Checklist

- Check the applied pinctrl state.
- Check whether another driver owns the pins.
- Validate electrical settings, not only mux function.
- Compare bootloader and Linux pin configuration.

## Related Topics

- [GPIO Consumer API](gpio-consumer-api.md)
- [Suspend And Resume](../power-management/suspend-resume.md)
- [Device Tree](../../device-tree/index.md)

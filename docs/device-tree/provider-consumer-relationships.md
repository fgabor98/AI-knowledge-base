---
status: draft
reviewed: false
domain: device-tree
difficulty: intermediate
last_reviewed: null
---

# Provider-Consumer Relationships

This page explains how Device Tree connects resource providers to the devices that consume them.

## Topics Covered

- phandles with argument cells
- zero-cell vs multi-cell providers
- `#clock-cells`
- `#reset-cells`
- `#gpio-cells`
- `#interrupt-cells`
- other provider-specific `#*-cells` properties
- consumer properties and `*-names` properties
- mapping a consumer property to its provider binding
- decoding specifier cells using the provider binding
- provider-consumer relationships for clocks, resets, GPIOs, interrupts, DMA, IOMMUs, PHYs, power domains, and regulators

## Related Topics

- [Syntax, Values, And Source Composition](syntax-values-and-source-composition.md)
- [Clocks, Resets, Regulators, And Power](clocks-resets-regulators-and-power.md)

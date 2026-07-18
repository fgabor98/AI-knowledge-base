---
status: draft
reviewed: false
domain: device-tree
difficulty: intermediate
last_reviewed: null
---

# Provider-Consumer Relationships

Device Tree is structurally a tree, but real hardware dependencies form a graph. A device can sit below one bus while consuming clocks, resets, GPIOs, interrupts, DMA channels, IOMMU contexts, PHYs, power domains, and regulators from nodes elsewhere. This module teaches one repeatable method for decoding those links.

## Learning Outcomes

After completing this module, you should be able to:

- split a phandle array into provider-specific entries
- use each provider's `#*-cells` value and binding to decode its arguments
- distinguish zero-cell, one-cell, and multi-cell providers
- map `*-names` entries to resource entries without losing positional order
- explain why similar-looking specifiers can mean different things under different providers
- decode clock, reset, GPIO, interrupt, DMA, IOMMU, PHY, and power-domain references
- recognize regulator supplies as named phandle relationships without a generic argument-cell convention
- connect consumer property names to Linux resource lookup APIs
- trace a missing supplier from DTS through binding, kernel configuration, and probe state

## Prerequisites

Complete [Syntax, Values, And Source Composition](syntax-values-and-source-composition.md). You should already understand labels, phandles, cells, string lists, and the difference between source and runtime trees.

## Learning Path

1. [Specifier Decoding And Resource Names](provider-consumer-relationships/specifier-decoding-and-resource-names.md)
2. [Clocks, Resets, And Power Domains](provider-consumer-relationships/clocks-resets-and-power-domains.md)
3. [GPIO And Interrupt Relationships](provider-consumer-relationships/gpio-and-interrupt-relationships.md)
4. [DMA, IOMMU, PHY, And Regulator Dependencies](provider-consumer-relationships/dma-iommu-phy-and-regulator-dependencies.md)
5. [Provider-Consumer Tracing Lab](provider-consumer-relationships/provider-consumer-tracing-lab.md)

## The Core Decoding Algorithm

Given a property such as:

```dts
clocks = <&osc>, <&clock_controller 4 1>;
clock-names = "reference", "bus";
```

decode it in this order:

1. Read the consumer binding to learn the property, entry count, order, and names.
2. Resolve the first phandle to its provider node.
3. Read that provider's `#clock-cells`.
4. Consume exactly that many cells after the phandle.
5. Decode those cells using that provider's binding.
6. Repeat from the next phandle.
7. Pair the completed entries positionally with `clock-names`.

For this example, `&osc` may declare `#clock-cells = <0>`, so its entry ends immediately after the phandle. If `&clock_controller` declares `#clock-cells = <2>`, the cells `4 1` belong to its provider-defined specifier.

## Dependency Graph Mental Model

```text
consumer node
├── clocks ----------> clock provider
├── resets ----------> reset controller
├── reset-gpios -----> GPIO controller
├── interrupts ------> interrupt parent/domain
├── dmas ------------> DMA controller
├── iommus ----------> IOMMU
├── phys ------------> PHY provider
├── power-domains ---> power-domain provider
└── vdd-supply ------> regulator provider
```

These links describe hardware. They may also allow Linux to establish supplier/consumer ordering, but a phandle alone does not guarantee that the provider driver exists, has probed, or has registered the required resource.

## Scope Boundary

This module focuses on relationship encoding and diagnosis. Detailed electrical GPIO behavior, interrupt maps, clock topology, regulator constraints, IOMMU address spaces, and runtime PM policy continue in later dedicated modules.

## Completion Check

You are ready to continue when you can:

- split a mixed-provider phandle array without assuming a fixed tuple width
- explain what zero-cell provider means
- identify which binding defines each specifier cell
- pair resources with names and detect mismatched cardinality
- distinguish `interrupts` from `interrupts-extended`
- explain the naming transformation from `reset-gpios` to `gpiod_get(dev, "reset", ...)`
- explain why `vdd-supply = <&regulator>;` does not use `#regulator-cells`
- produce a supplier checklist for a deferred probe

## Authoritative References

- [Devicetree Specification: phandles and specifier mappings](https://devicetree-specification.readthedocs.io/en/stable/devicetree-basics.html)
- [Linux Devicetree bindings guidance](https://docs.kernel.org/devicetree/bindings/writing-bindings.html)
- [Linux DeviceTree kernel API](https://docs.kernel.org/devicetree/kernel-api.html)
- [Linux driver model device links](https://docs.kernel.org/driver-api/device_link.html)

## Related Topics

- [Syntax, Values, And Source Composition](syntax-values-and-source-composition.md)
- [Addressing And Bus Modeling](addressing-and-bus-modeling.md)
- [Pinctrl, GPIOs, And Interrupts](pinctrl-gpios-and-interrupts.md)
- [Clocks, Resets, Regulators, And Power](clocks-resets-regulators-and-power.md)

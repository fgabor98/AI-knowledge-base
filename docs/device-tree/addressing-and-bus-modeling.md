---
status: draft
reviewed: false
domain: device-tree
difficulty: intermediate
last_reviewed: null
---

# Addressing And Bus Modeling

Device Tree addresses are interpreted in a bus-specific namespace. A `reg` value may name a CPU-visible register window, an I2C target address, an SPI chip select, or a PCI bus/device/function tuple. Translation properties connect those namespaces; interrupt properties describe a separate routing hierarchy.

This module builds one disciplined method for decoding each value without assuming that a unit address is already a CPU physical address.

## Learning Outcomes

After completing this module, you should be able to:

- split `reg` into entries using the immediate parent's cell-count contract
- reconstruct multi-cell addresses and sizes without truncation
- distinguish an absent `ranges` property from an empty identity mapping
- translate an address across several nested buses
- explain why CPU and DMA views of memory can differ
- decode `interrupts` through inherited `interrupt-parent` relationships
- interpret I2C, SPI, MDIO, and other binding-specific unit addresses
- read the address-space tag in a PCI `ranges` entry
- separate PCI configuration, I/O, memory, DMA, INTx, and MSI routing
- verify the final translated resources in Linux rather than trusting DTS arithmetic

## Prerequisites

Complete [Standard Nodes And Properties](standard-nodes-and-properties.md). You should already understand cells, phandles, provider specifiers, reservations, and the difference between source and runtime trees.

## Learning Path

1. [Cell Counts, `reg`, And Unit Addresses](addressing-and-bus-modeling/cell-counts-reg-and-unit-addresses.md)
2. [`simple-bus`, `ranges`, And Nested Translation](addressing-and-bus-modeling/simple-bus-ranges-and-nested-translation.md)
3. [DMA Address Spaces And `dma-ranges`](addressing-and-bus-modeling/dma-address-spaces-and-dma-ranges.md)
4. [Interrupt Parents, Specifiers, And Routing](addressing-and-bus-modeling/interrupt-parents-specifiers-and-routing.md)
5. [Bus-Specific Child Addressing](addressing-and-bus-modeling/bus-specific-child-addressing.md)
6. [PCI Host Bridges, Address Windows, And Interrupts](addressing-and-bus-modeling/pci-host-bridges-address-windows-and-interrupts.md)
7. [Address Translation And Bus Modeling Lab](addressing-and-bus-modeling/address-translation-and-bus-modeling-lab.md)

## The Four Questions To Ask

For every address-like property, answer these before calculating:

1. **Which binding defines the value?** Generic `reg` rules are only the envelope; the bus binding defines the address meaning.
2. **Which node defines the widths?** Child entries use the immediate parent's `#address-cells` and `#size-cells` unless that bus binding says otherwise.
3. **Which namespace is this?** Child-bus, parent-bus, CPU physical, DMA, I/O-port, or configuration space are not interchangeable.
4. **Which translations apply?** Walk one boundary at a time and reject any address not covered by a required window.

## Independent Topologies

One device can participate in several mappings:

```text
CPU load/store:  child reg --ranges--> parent address --ranges--> CPU physical
device DMA:      device DMA address --dma-ranges/IOMMU--> memory address
interrupt:       child specifier --interrupt-map--> parent domain --> CPU IRQ
```

These paths are related by hardware but encoded independently. A correct MMIO `reg` does not prove DMA reachability, and a working interrupt does not prove either address path.

## Review Principle

Never approve address arithmetic that exists only in comments or reviewer intuition. Show the cell grouping, combine the cells, apply every mapping as a half-open interval, and compare the result with the final DTB and runtime resources.

## Completion Check

You are ready for [Driver Matching](driver-matching.md) when you can:

- identify the exact node that supplies every field width
- calculate a nested translated CPU address
- explain why missing `ranges` does not generally mean identity
- give separate CPU-visible and device-visible addresses for one DMA buffer
- identify an interrupt controller and decode its provider-specific specifier
- explain why `sensor@48` and `flash@0` are not MMIO locations
- split a PCI range into type, child address, parent address, and length

## Authoritative References

- [Devicetree Specification: addressing, `reg`, `ranges`, DMA, and interrupts](https://devicetree-specification.readthedocs.io/en/stable/devicetree-basics.html)
- [Linux DeviceTree kernel API](https://docs.kernel.org/devicetree/kernel-api.html)
- [Linux Device Tree bindings](https://docs.kernel.org/devicetree/bindings/index.html)
- [Linux DTS coding style](https://docs.kernel.org/devicetree/bindings/dts-coding-style.html)

## Related Topics

- [Provider-Consumer Relationships](provider-consumer-relationships.md)
- [Standard Nodes And Properties](standard-nodes-and-properties.md)
- [Pinctrl, GPIOs, And Interrupts](pinctrl-gpios-and-interrupts.md)

---
status: draft
reviewed: false
domain: device-tree
difficulty: intermediate
last_reviewed: null
---

# Bus-Specific Child Addressing

The parent bus binding determines what a child's `reg` and unit address mean. Memory-mapped arithmetic must not be applied to protocol addresses, chip selects, ports, or discoverable-bus identifiers.

## Compare Common Buses

| Parent bus | Typical cell counts | Child `reg` commonly means | Size encoded? |
|---|---|---|---|
| simple MMIO bus | binding/platform-defined | bus address or offset | yes |
| I2C | `1`, `0` | 7-bit or 10-bit target address | no |
| SPI controller | `1`, `0` | chip-select index | no |
| MDIO | `1`, `0` | PHY address | no |
| CPU container | architecture-defined, `0` | hardware CPU/thread identifier | no |
| PCI | `3`, `2` | address-space flags and PCI address fields | yes |

This table is a starting point, not a replacement for the specific controller and child schemas.

## I2C Children

```dts
i2c@4000 {
        #address-cells = <1>;
        #size-cells = <0>;

        temperature-sensor@48 {
                compatible = "ti,tmp102";
                reg = <0x48>;
        };
};
```

`0x48` is the I2C target address as encoded by the binding, not an offset inside the controller's register window. The controller node's own `reg` is interpreted by its parent MMIO bus; the sensor's `reg` is interpreted by the I2C controller. The same property changes namespace at the boundary.

Ten-bit targets and address-related flags require the common I2C binding's encoding. Do not invent extra address cells or store an 8-bit wire value that includes the read/write bit.

I2C multiplexers add further bus levels. Each channel node can become a logical I2C bus with its own children. Channel selection is routing state, not normally an address offset to add to the target address.

## SPI Children

```dts
spi@5000 {
        #address-cells = <1>;
        #size-cells = <0>;

        flash@0 {
                compatible = "jedec,spi-nor";
                reg = <0>;
                spi-max-frequency = <50000000>;
        };
};
```

`reg = <0>` selects chip select 0. The SPI peripheral's internal register addresses are part of its transaction protocol and are not child `reg` resources. Some bindings allow several chip selects or special slave-mode layouts; follow the schema rather than assuming one cell always means one device.

Chip-select GPIOs are controller resources and have their own indexing/polarity rules. They do not change the child's logical chip-select number.

## MDIO And Other Numbered Buses

```dts
mdio {
        #address-cells = <1>;
        #size-cells = <0>;

        ethernet-phy@1 {
                reg = <1>;
        };
};
```

The PHY address is local to that MDIO bus. An MDIO multiplexer or switch can create nested management buses where the same PHY number appears under different parents.

Other bindings use `reg` for slots, ports, endpoints, functions, or controller-local indices. If the hardware has an identifier but the binding does not use child nodes, do not force it into a fabricated bus model.

## Discoverable Versus Non-Discoverable Buses

SoC MMIO devices usually need explicit nodes because no standard enumeration mechanism discovers them. PCI and USB can enumerate devices, so firmware generally describes the host controller and non-discoverable board integration rather than duplicating every discoverable function.

Device Tree nodes may still be needed for discoverable devices when the binding must provide information that enumeration cannot, such as:

- fixed board wiring and GPIOs
- regulators, clocks, or reset controls outside the protocol
- MAC addresses or calibration data
- interrupt routing not discoverable through the bus
- platform-specific mode or lane configuration

The node address then follows that discoverable bus's binding and topology.

## Bus Nodes Are Devices Too

An I2C or SPI controller participates in two contracts:

1. As a child of its parent, its own `reg`, interrupts, clocks, and power describe the controller hardware.
2. As a parent, its `#address-cells`, `#size-cells`, and child binding define attached peripherals.

Debug each side independently. A correct sensor address cannot compensate for an incorrectly translated controller MMIO resource.

## Binding-First Workflow

For any unfamiliar child node:

1. Read the parent's bus/controller binding.
2. Read the child's compatible-specific binding.
3. Identify the prescribed cell counts and `reg` meaning.
4. Check unit-address formatting.
5. Validate controller-local limits, reserved values, and flags.
6. Confirm board wiring and runtime enumeration.

`dtc` can check structural conventions but cannot know that the schematic places a device at `0x48` rather than `0x49`. Schema validation catches more constraints, but hardware evidence remains necessary.

## Review Traps

- Adding a size cell below a zero-size-cell bus.
- Treating I2C `0x48` as a CPU address.
- Encoding the I2C read/write bit in `reg`.
- Confusing SPI chip-select index with a peripheral-internal register.
- Flattening mux channels and creating duplicate addresses on one logical bus.
- Copying cell counts from the controller's parent side to its child side.
- Describing discoverable devices redundantly and then letting firmware and enumeration disagree.

## Authoritative References

- [Devicetree Specification: parent cell counts and `reg`](https://devicetree-specification.readthedocs.io/en/stable/devicetree-basics.html)
- [Linux Device Tree bindings](https://docs.kernel.org/devicetree/bindings/index.html)
- [Linux SPI controller common binding](https://github.com/torvalds/linux/blob/master/Documentation/devicetree/bindings/spi/spi-controller.yaml)
- [Linux I2C subsystem documentation](https://docs.kernel.org/i2c/index.html)
- [Linux MDIO bus and PHY library](https://docs.kernel.org/networking/phy.html)

## Next Step

Continue with [PCI Host Bridges, Address Windows, And Interrupts](pci-host-bridges-address-windows-and-interrupts.md).

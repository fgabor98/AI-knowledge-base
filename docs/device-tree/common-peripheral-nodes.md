---
status: draft
reviewed: false
domain: device-tree
difficulty: intermediate
last_reviewed: null
---

# Common Peripheral Nodes

Common peripherals are where every earlier Device Tree concept meets real hardware: bus addressing, driver matching, clocks, supplies, pin control, interrupts, DMA, PHYs, and subsystem policy. Familiarity with a bus is not enough; each child node must satisfy both its bus contract and its device binding.

## Learning Outcomes

After completing this module, you should be able to:

- distinguish controller nodes, bus children, connectors, PHYs, and protocol endpoints
- review UART console handoff without confusing aliases, stdout selection, and Linux tty numbering
- model non-discoverable I²C and SPI devices on the correct physical segment
- reason about chip selects, bus modes, addressing, muxes, and shared-bus electrical limits
- connect CAN controllers to transceivers and validate bit timing against the physical network
- describe USB roles, PHYs, connectors, VBUS, and Type-C control without conflicting ownership
- review PCIe host windows, requester-ID maps, resets, and enumeration boundaries
- distinguish removable SD, soldered eMMC, and SDIO function requirements
- trace Ethernet from MAC through PCS/interface timing to an MDIO PHY or fixed link
- use standard subsystem bindings for LEDs, keys, watchdogs, RTCs, hwmon, and NVMEM
- partition raw flash without confusing physical layout, update policy, and bootloader contracts
- debug from the final DTB through the relevant Linux subsystem and physical bus

## Prerequisites

Complete [Clocks, Resets, Regulators, And Power](clocks-resets-regulators-and-power.md). This module assumes you can decode provider tuples, prove resource sequencing, and inspect live framework state.

## Learning Path

1. [UARTs, Consoles, And Serial Children](common-peripheral-nodes/uarts-consoles-and-serial-children.md)
2. [I2C Controllers, Devices, And Muxes](common-peripheral-nodes/i2c-controllers-devices-and-muxes.md)
3. [SPI Controllers, Chip Selects, And Peripherals](common-peripheral-nodes/spi-controllers-chip-selects-and-peripherals.md)
4. [CAN Controllers, Transceivers, And Bit Timing](common-peripheral-nodes/can-controllers-transceivers-and-bit-timing.md)
5. [USB Controllers, PHYs, Roles, And Connectors](common-peripheral-nodes/usb-controllers-phys-roles-and-connectors.md)
6. [PCIe Host Bridges, Windows, And Enumeration](common-peripheral-nodes/pcie-host-bridges-windows-and-enumeration.md)
7. [MMC, SD, SDIO, And eMMC](common-peripheral-nodes/mmc-sd-sdio-and-emmc.md)
8. [Ethernet MACs, MDIO, PHYs, And Fixed Links](common-peripheral-nodes/ethernet-macs-mdio-phys-and-fixed-links.md)
9. [Board Services: LEDs, Keys, Watchdogs, RTC, Hwmon, And NVMEM](common-peripheral-nodes/board-services-leds-keys-watchdogs-rtc-hwmon-and-nvmem.md)
10. [MTD, SPI NOR, And Fixed Partitions](common-peripheral-nodes/mtd-spi-nor-and-fixed-partitions.md)
11. [Peripheral Integration And Diagnosis Lab](common-peripheral-nodes/peripheral-integration-and-diagnosis-lab.md)

## A Repeatable Review Model

For every peripheral, answer six questions:

| Layer | Question |
|---|---|
| placement | Which physical controller or bus segment contains it? |
| identity | Which compatible contract creates the device and matches its driver? |
| address | What does `reg` mean on this bus? |
| resources | Which supplies, clocks, resets, pins, interrupts, DMA channels, and PHYs are required? |
| policy boundary | Which properties are hardware facts, and which behavior belongs in a driver or user space? |
| runtime proof | Which sysfs, debugfs, subsystem tool, trace, or measurement proves operation? |

The same property name can mean different things under different buses. An I²C child `reg` is normally a bus address; an SPI child `reg` selects a chip select; a PCI child address encodes PCI address-space information. Decode it in parent-bus context.

## Discovery Boundaries

I²C and SPI devices generally do not self-enumerate, so firmware must describe populated devices. USB and PCIe normally enumerate downstream devices, so DT primarily describes the host/controller, fixed wiring, non-discoverable platform constraints, or devices that must exist before enumeration. MDIO sits between these models: PHY IDs can be read, but the bus topology, address, resets, delays, and board wiring often still need DT.

Do not describe a hot-pluggable USB device as a permanent child merely because one unit was attached during development. Conversely, do not expect scanning to discover an I²C sensor safely.

## Completion Check

You are ready for [Graph Bindings And Complex Data Paths](graph-bindings-and-complex-data-paths.md) when you can:

- place each peripheral on the correct bus segment and decode its address
- explain where enumeration starts and firmware description ends
- prove bus electrical and timing configuration from schematic and binding
- separate a controller, PHY/transceiver, connector, and protocol device
- trace probe and runtime state through the appropriate subsystem
- identify when a plausible node duplicates ownership or models policy instead of hardware
- design cold-boot, suspend/resume, error, and hotplug tests appropriate to each bus

## Authoritative References

- [Linux Devicetree binding index](https://docs.kernel.org/devicetree/bindings/index.html)
- [Linux I2C device instantiation](https://docs.kernel.org/i2c/instantiating-devices.html)
- [Linux SPI overview](https://docs.kernel.org/spi/spi-summary.html)
- [Linux SocketCAN documentation](https://docs.kernel.org/networking/can.html)
- [Linux PHY and phylink documentation](https://docs.kernel.org/networking/sfp-phylink.html)

## Related Topics

- [Addressing And Bus Modeling](addressing-and-bus-modeling.md)
- [Pinctrl, GPIOs, And Interrupts](pinctrl-gpios-and-interrupts.md)
- [Clocks, Resets, Regulators, And Power](clocks-resets-regulators-and-power.md)

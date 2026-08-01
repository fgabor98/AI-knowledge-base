---
status: active
reviewed: false
domain: device-tree
difficulty: intermediate
last_reviewed: null
---

# 4. Standard Nodes, Buses, Providers, Peripherals, And Graphs

Official source: [`Documentation/devicetree/bindings`](https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/tree/Documentation/devicetree/bindings) in the exact Linux tree

Knowledge-guide companion: [Stage 4](knowledge-guide-companion.md#stage-4-standard-nodes-buses-providers-peripherals-and-graphs)

## Reading A Binding Directory

- [ ] **P0** Read the common schema(s) before the vendor device schema.
- [ ] **P0** Record the schema `$id`, compatible/node selection, referenced schemas, properties, required list, conditionals, and closure rule.
- [ ] **P0** Read examples as schema test cases, not as board templates.
- [ ] **P0** Find two in-tree DTS users and compare their hardware topology.
- [ ] **P0** Find the driver match table and every property/resource read during probe.
- [ ] **P0** Decode cells using the named provider binding; do not infer them from numbers.
- [ ] **P1** Read subsystem documentation for runtime semantics not defined by DT.

## Root And Standard Nodes

- [ ] **P0** [`root-node.yaml`](https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/tree/Documentation/devicetree/bindings/root-node.yaml)
- [ ] **P0** [`chosen.yaml`](https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/tree/Documentation/devicetree/bindings/chosen.yaml)
- [ ] **P0** CPU, topology, idle-state, cache, and operating-point schemas relevant to the architecture.
- [ ] **P0** Memory and reserved-memory common schemas.
- [ ] **P0** `simple-bus`, fixed-clock, fixed-regulator, and GPIO/interrupt common schemas.
- [ ] **P1** `/aliases` consumers and project numbering policy.
- [ ] **P1** firmware, nvmem, thermal, watchdog, RTC, and chosen extensions used by the platform.

## Addressed Buses

- [ ] **P0** `simple-bus`, parent cell counts, `reg`, and `ranges`.
- [ ] **P0** I2C controller/device/mux bindings and child address rules.
- [ ] **P0** SPI controller/peripheral/chip-select bindings.
- [ ] **P0** MDIO and PHY child addressing.
- [ ] **P0** MMC/SDIO child/device conventions when used.
- [ ] **P1** PCI host bridge `ranges`, `dma-ranges`, address flags, and interrupt mapping.
- [ ] **P1** MTD/NAND/SPI-NOR partitions and ECC ownership for relevant products.
- [ ] **P2** Other bus classes only when present in the project.

## Pins, GPIOs, And Interrupts

- [ ] **P0** [Linux pin control documentation](https://docs.kernel.org/driver-api/pin-control.html) plus controller-specific binding.
- [ ] **P0** GPIO controller/consumer schemas, polarity, hogs, ranges, and line names.
- [ ] **P0** Interrupt-controller bindings, parent inheritance, cascades, maps, trigger type, and shared lines.
- [ ] **P0** `pinctrl-names` and default/init/idle/sleep states.
- [ ] **P1** Wakeup-source and suspend-state interactions.

## Clocks, Resets, Regulators, And Power

- [ ] **P0** Common clock binding and controller-specific specifier IDs.
- [ ] **P0** Reset binding and shared/exclusive sequencing implications.
- [ ] **P0** Regulator schemas, input supplies, constraints, enable GPIOs, and consumer supply names.
- [ ] **P0** Generic power-domain bindings and device dependencies.
- [ ] **P0** OPP v2 and thermal-zone/cooling-map bindings when DVFS or thermal policy is used.
- [ ] **P1** Runtime-PM/device-link documentation needed to understand lifecycle beyond DT.

## Common Peripherals

For each project device, read common schema, vendor schema, driver, and subsystem docs:

- [ ] **P0** UART and console path
- [ ] **P0** I2C clients and muxes
- [ ] **P0** SPI clients and chip selects
- [ ] **P0** MMC/SD/SDIO/eMMC
- [ ] **P0** Ethernet MAC, MDIO, PHY/fixed-link/SFP and [phylink](https://docs.kernel.org/networking/sfp-phylink.html)
- [ ] **P1** USB controller/PHY/connector/role switch
- [ ] **P1** CAN controller/transceiver
- [ ] **P1** PCIe host/controller/PHY
- [ ] **P1** MTD, SPI NOR, NAND, and partitions
- [ ] **P1** LEDs, keys, watchdogs, RTC, hwmon, and NVMEM
- [ ] **P2** project-specific audio, media, display, input, IIO, PWM, and other devices

## Graph Bindings And Pipelines

- [ ] **P0** Generic graph schema and `ports`/`port`/`endpoint` numbering.
- [ ] **P0** `remote-endpoint` reciprocity and interface properties.
- [ ] **P1** DRM/display connector, panel, bridge, and controller bindings for display projects.
- [ ] **P1** V4L2 fwnode endpoint/bus documentation and sensor/receiver/ISP bindings for camera projects.
- [ ] **P1** ASoC DAI/audio graph/card bindings for audio projects.
- [ ] **P1** Power/lifecycle ownership across every pipeline component.

## Binding Reading Exercise

For one complex device, produce:

```text
node path and parent bus
compatible selection
reg/address translation
interrupt parent/specifier
clocks/resets/power domains
regulator supplies
DMA/IOMMU/PHY resources
pinctrl states
child nodes or graph endpoints
driver match/property reads
probe order and subsystem result
```

- [ ] Validate the node against its schema.
- [ ] Compare with two in-tree users without copying board-specific values.
- [ ] Prove identity/basic transfer, interrupt, DMA, stress, and power behavior as applicable.

## Stage Completion

- [ ] I can find the applicable common and device-specific schemas for any project node.
- [ ] I can distinguish parent-bus encoding from provider-cell encoding.
- [ ] I can map electrical topology into pins, power, clocks, resets, interrupts, buses, and graph links.
- [ ] I use examples as tests and patterns, never as unexplained board data.

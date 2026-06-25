---
status: draft
reviewed: false
domain: device-tree
difficulty: intermediate
last_reviewed: null
---

# Device Tree

Device Tree topics focused on describing embedded hardware for Linux, U-Boot, firmware handoff, and board bring-up.

Device Tree appears in kernel, U-Boot, TI SDK, board porting, pinmux, clocks, regulators, networking, storage, and remoteproc work. This section is the standalone roadmap for learning it systematically.

## Roadmap

### Foundations

- what Device Tree solves
- DTS
- DTSI
- DTB
- overlays
- source include structure
- labels
- phandles
- node names
- unit addresses
- properties
- comments and style

### Addressing And Bus Modeling

- `reg`
- `ranges`
- `#address-cells`
- `#size-cells`
- `interrupt-parent`
- `interrupts`
- `dma-ranges`
- simple-bus
- bus-specific child addressing

### Driver Matching

- `compatible`
- fallback compatible strings
- `of_match_table`
- platform devices
- modalias
- binding-driven driver expectations
- optional vs required properties

### Pinctrl, GPIOs, And Interrupts

- pinmux
- pin configuration
- GPIO controllers
- GPIO consumers
- active-high vs active-low
- interrupt controllers
- interrupt trigger types
- reset GPIOs

### Clocks, Resets, Regulators, And Power

- clock providers
- clock consumers
- reset controllers
- fixed regulators
- PMIC regulators
- regulator constraints
- power domains
- wake sources
- runtime PM dependencies

### Common Peripheral Nodes

- UART
- I2C
- SPI
- CAN
- USB
- PCIe
- MMC
- SD
- eMMC
- Ethernet MAC
- Ethernet PHY
- MDIO
- fixed-link

### Memory, Firmware, And Heterogeneous SoCs

- `/memory`
- `/chosen`
- reserved memory
- CMA
- firmware nodes
- remoteproc
- RPMsg
- PRU
- R5/M4 cores
- shared memory

### U-Boot And Bootloader Device Tree

- U-Boot control DTB
- SPL DTB
- Linux DTB
- U-Boot-specific properties
- pre-relocation properties
- overlays applied by U-Boot
- FIT image DTB selection
- environment-driven DTB loading

### Binding Schema Validation

- YAML bindings
- `dt-bindings`
- `dtc` warnings
- `dtbs_check`
- schema errors
- undocumented properties
- binding examples
- vendor bindings

### Runtime Inspection

- `/proc/device-tree`
- `/sys/firmware/devicetree/base`
- decoded DTBs with `dtc`
- checking deployed DTB identity
- `dmesg` probe logs
- driver bind/unbind checks
- comparing source DTS to runtime tree

### Board Porting Workflow

- start from closest EVM
- board delta list
- minimal boot DTS
- console first
- boot media next
- regulators and clocks
- Ethernet
- storage
- remoteproc and reserved memory
- overlays
- validation checklist

## Related Topics

- [Linux Kernel Programming](../linux-kernel/index.md)
- [Embedded Linux](../embedded-linux/index.md)
- [Build Systems Device Tree Build and Validation](../build-systems/advanced/device-tree-build-and-validation.md)
- [TI Processor SDK Custom Sitara Board Bring-Up](../build-systems/advanced/ti-processor-sdk/custom-sitara-board-bring-up.md)
- [Networking](../networking/index.md)
- [Topic Map](../topic-map.md)

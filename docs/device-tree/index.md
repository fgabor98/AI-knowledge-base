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

## Learning Materials

1. [Official Device Tree Documentation Reading Checklist](official-docs-reading/index.md)
2. [Foundations](foundations.md)
3. [Syntax, Values, And Source Composition](syntax-values-and-source-composition.md)
4. [Provider-Consumer Relationships](provider-consumer-relationships.md)
5. [Standard Nodes And Properties](standard-nodes-and-properties.md)
6. [Addressing And Bus Modeling](addressing-and-bus-modeling.md)
7. [Driver Matching](driver-matching.md)
8. [Pinctrl, GPIOs, And Interrupts](pinctrl-gpios-and-interrupts.md)
9. [Clocks, Resets, Regulators, And Power](clocks-resets-regulators-and-power.md)
10. [Common Peripheral Nodes](common-peripheral-nodes.md)
11. [Graph Bindings And Complex Data Paths](graph-bindings-and-complex-data-paths.md)
12. [Memory, Firmware, And Heterogeneous SoCs](memory-firmware-and-heterogeneous-socs.md)
13. [U-Boot And Bootloader Device Tree](u-boot-and-bootloader-device-tree.md)
14. [Boot-Time Mutation And Ownership](boot-time-mutation-and-ownership.md)
15. [Binding Design And Stable ABI](binding-design-and-stable-abi.md)
16. [Writing And Validating Binding Schemas](writing-and-validating-binding-schemas.md)
17. [Overlays In Depth](overlays-in-depth.md)
18. [Build And Diagnostic Tools](build-and-diagnostic-tools.md)
19. [Runtime Inspection](runtime-inspection.md)
20. [Security And Production Lifecycle](security-and-production-lifecycle.md)
21. [Product-Scale Maintenance And Engineering](product-scale-maintenance-and-engineering.md)
22. [Board Porting Workflow](board-porting-workflow.md)

## Roadmap

### Foundations

- what Device Tree solves
- Device Tree as a hardware description rather than driver configuration
- Device Tree bindings as a stable ABI across bootloaders, kernels, and operating systems
- Device Tree vs ACPI and when each hardware-description model applies
- DTS
- DTSI
- DTB
- DTBO
- overlays
- flattened Device Tree structure
- DTB header, memory reservation block, structure block, and strings block
- source include structure and inheritance
- labels
- phandles
- node names
- unit addresses
- properties
- paths and aliases
- comments and style

### Syntax, Values, And Source Composition

- 32-bit cells and cell arrays
- strings and string lists
- byte arrays
- empty and boolean properties
- 64-bit values represented by multiple cells
- property and node references
- label references and path references
- `/bits/`
- `/delete-node/`
- `/delete-property/`
- `/include/` directives
- C preprocessor includes and macros
- DTS includes vs C preprocessor includes
- overriding and extending nodes from included DTSI files
- board, SoC, and shared-family source layering
- source formatting and Linux DTS coding style

### Provider–Consumer Relationships

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
- provider–consumer relationships for clocks, resets, GPIOs, interrupts, DMA, IOMMUs, PHYs, power domains, and regulators

### Standard Nodes And Properties

- root-node `compatible`
- root-node `model`
- `status`
- `/aliases`
- `/cpus`
- CPU topology
- `/memory`
- `/chosen`
- `stdout-path`
- boot arguments
- `/reserved-memory`
- `/memreserve/`
- `interrupts-extended`
- `interrupt-map`
- `interrupt-map-mask`
- `dma-coherent`
- `iommus`
- `phys`
- `phy-names`

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
- address translation across nested buses
- PCI host bridge address mapping
- PCI interrupt mapping

### Driver Matching

- `compatible`
- fallback compatible strings
- board-compatible vs SoC-compatible fallback chains
- backward compatibility and when to introduce a new `compatible`
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
- operating-points-v2 tables
- CPU frequency relationships
- thermal zones
- cooling devices

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
- LEDs
- keys and buttons
- watchdogs
- RTCs
- hardware monitors
- NVMEM providers and consumers
- MTD devices and fixed partitions

### Graph Bindings And Complex Data Paths

- `ports`
- `port`
- `endpoint`
- local and remote endpoints
- display pipelines
- camera pipelines
- audio routing
- graph validation and endpoint consistency

### Memory, Firmware, And Heterogeneous SoCs

- CMA
- firmware nodes
- IOMMU topology
- DMA coherency
- DMA address translation
- remoteproc
- RPMsg
- PRU
- R5/M4 cores
- shared memory
- trusted firmware
- OP-TEE
- secure-world reserved memory

### U-Boot And Bootloader Device Tree

- U-Boot control DTB
- SPL DTB
- Linux DTB
- U-Boot-specific properties
- pre-relocation properties
- overlays applied by U-Boot
- FIT image DTB selection
- environment-driven DTB loading

### Boot-Time Mutation And Ownership

- bootloader fixups
- firmware fixups
- memory-size updates
- MAC-address injection
- serial-number injection
- `/chosen` modifications
- overlay application order
- DTB relocation and available padding
- ownership of each boot-time mutation
- built DTB vs bootloader-visible tree vs Linux runtime tree
- tracing the exact DTB and overlays selected during boot

### Binding Design And Stable ABI

- describing hardware rather than Linux implementation details
- avoiding nodes created only to instantiate drivers
- complete hardware descriptions despite incomplete driver support
- binding backward compatibility
- compatible-string versioning
- property naming and standard unit suffixes
- standard property reuse
- avoiding policy in Device Tree
- board and product revision strategies
- Devicetree ABI versioning across product revisions
- binding review expectations
- upstream binding submission workflow
- submitting bindings before DTS users

### Writing And Validating Binding Schemas

- YAML bindings
- `dt-bindings`
- `$id`
- `$schema`
- `maintainers`
- `description`
- `select`
- `properties`
- `patternProperties`
- `required`
- `$ref`
- `allOf`
- `oneOf`
- conditional schemas
- `additionalProperties` vs `unevaluatedProperties`
- child-node schemas
- property types
- array cardinality
- binding examples
- vendor bindings
- `dt_binding_check`
- targeted validation with `DT_SCHEMA_FILES`
- `dtc` warnings
- `dtbs_check`
- why invalid schemas can cause `dtbs_check` to skip checks
- schema errors
- undocumented properties

### Overlays In Depth

- `/plugin/`
- fragments and targets
- label targets vs path targets
- `__symbols__`
- `__fixups__`
- local fixups
- base DTB symbol requirements
- compiling overlays with symbols
- bootloader-applied vs kernel-applied overlays
- overlay stacking and removal dependencies
- overlay compatibility across base DTB versions
- lifetime hazards when dynamically removing overlay nodes
- limitations of overlays as a board-variant mechanism

### Build And Diagnostic Tools

- `dtc`
- `fdtdump`
- `fdtget`
- `fdtput`
- `fdtoverlay`
- U-Boot `fdt` commands
- compiler symbols with `-@`
- compiler warning levels
- kernel `W=1` and `W=2` Device Tree builds
- preprocessing a DTS
- tracing a generated DTB to its source and build rule
- `libfdt`
- firmware and bootloader use of `libfdt`

### Runtime Inspection

- `/proc/device-tree`
- `/sys/firmware/devicetree/base`
- decoded DTBs with `dtc`
- checking deployed DTB identity
- `dmesg` probe logs
- driver bind/unbind checks
- comparing source DTS to runtime tree
- inspecting NUL-terminated property values safely
- inspecting binary cells and byte arrays
- comparing DTB hashes across build, boot media, and target
- inspecting the tree from U-Boot before kernel handoff

### Security And Production Lifecycle

- DTB and DTBO integrity
- FIT signing and authenticated Device Tree selection
- measured boot and Device Tree
- malicious or untrusted DTB risks
- security impact of bootloader fixups and overlays
- coordinating kernel, DTB, modules, and firmware versions
- reproducible DTB builds
- DTB provenance and release manifests
- field update compatibility

### Product-Scale Maintenance And Engineering

- Devicetree specification version differences and compatibility implications
- schema and DTS review methodology
- separating correctness, ABI, style, and maintainability concerns during review
- maintaining downstream vendor trees vs upstream DTS
- managing patch stacks and minimizing long-lived DTS divergence
- cross-version kernel, bootloader, firmware, DTB, and overlay compatibility testing
- large-product DT organization and ownership conventions
- defining ownership boundaries across silicon, module, carrier-board, and product teams
- CI design for multiple boards, product variants, and overlays
- validation matrices and representative hardware coverage
- deprecation and migration strategies for bindings, properties, and compatible strings
- realistic board bring-up failure postmortems
- converting failures and escapes into reusable review checks and CI coverage

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
- board revision and product variant modeling
- minimizing board-specific deltas
- upstreaming bindings and DTS changes
- validation checklist

## Related Topics

- [Linux Kernel Programming](../linux-kernel/index.md)
- [Embedded Linux](../embedded-linux/index.md)
- [Build Systems Device Tree Build and Validation](../build-systems/advanced/device-tree-build-and-validation.md)
- [TI Processor SDK Custom Sitara Board Bring-Up](../build-systems/advanced/ti-processor-sdk/custom-sitara-board-bring-up.md)
- [Networking](../networking/index.md)
- [Topic Map](../topic-map.md)

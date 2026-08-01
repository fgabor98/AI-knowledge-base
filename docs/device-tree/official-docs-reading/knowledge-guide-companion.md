---
status: active
reviewed: false
domain: device-tree
difficulty: intermediate
last_reviewed: null
---

# Knowledge Guide Companion Checklist

This checklist tracks the local knowledge-guide pages that accompany the [Official Device Tree Documentation Reading Checklist](index.md). It uses the same eight stages so normative sources, local explanations, source/schema study, and labs can be completed together.

All 182 Device Tree knowledge-guide pages that existed when this checklist was created are assigned exactly once. A page may relate to several stages, but it has one checkbox here so completion remains unambiguous. The official-reading pages themselves are tracked by the overview and are not included in the 182-page count.

## Synchronization Rule

For each stage:

```text
read one coherent group of official P0 sources
-> read the matching knowledge-guide pages below
-> inspect the named specifications, schemas, source, and real DTS users
-> complete the associated exercise or lab
-> check off both trackers
```

Mark a local page complete when you can explain its model, decode its examples from the governing binding, and apply its validation/debugging workflow. Reading the prose once is not enough.

## Stage 1: Specification Foundations And The Core Tree Model

Official tracker: [Specification Foundations And The Core Tree Model](01-specification-foundations-and-core-tree-model.md)

- [ ] [Device Tree](../index.md)

### Foundations

- [ ] [Foundations](../foundations.md)
- [ ] [The Hardware Description Model](../foundations/hardware-description-model.md)
- [ ] [Source And Binary Artifacts](../foundations/source-and-binary-artifacts.md)
- [ ] [Tree Anatomy And Vocabulary](../foundations/tree-anatomy-and-vocabulary.md)
- [ ] [First Device Tree Lab](../foundations/first-device-tree-lab.md)

Stage completion:

- [ ] I can distinguish the specification, binding, source language, binary format, and implementation.
- [ ] I can explain the hardware-description model and navigate a tree using nodes, properties, paths, labels, and phandles.

## Stage 2: DTS Source, FDT Binary Format, dtc, And libfdt

Official tracker: [DTS Source, FDT Binary Format, dtc, And libfdt](02-dts-fdt-dtc-and-libfdt.md)

### Syntax, Values, And Source Composition

- [ ] [Syntax, Values, And Source Composition](../syntax-values-and-source-composition.md)
- [ ] [DTS Grammar And Value Encodings](../syntax-values-and-source-composition/dts-grammar-and-value-encodings.md)
- [ ] [References, Amendments, And Deletions](../syntax-values-and-source-composition/references-amendments-and-deletions.md)
- [ ] [Includes, Preprocessing, And Macros](../syntax-values-and-source-composition/includes-preprocessing-and-macros.md)
- [ ] [Hardware-Based Layering And Source Style](../syntax-values-and-source-composition/hardware-based-layering-and-source-style.md)
- [ ] [Source Composition Lab](../syntax-values-and-source-composition/source-composition-lab.md)

### Provider-Consumer Relationships

- [ ] [Provider-Consumer Relationships](../provider-consumer-relationships.md)
- [ ] [Specifier Decoding And Resource Names](../provider-consumer-relationships/specifier-decoding-and-resource-names.md)
- [ ] [Clocks, Resets, And Power Domains](../provider-consumer-relationships/clocks-resets-and-power-domains.md)
- [ ] [GPIO And Interrupt Relationships](../provider-consumer-relationships/gpio-and-interrupt-relationships.md)
- [ ] [DMA, IOMMU, PHY, And Regulator Dependencies](../provider-consumer-relationships/dma-iommu-phy-and-regulator-dependencies.md)
- [ ] [Provider-Consumer Tracing Lab](../provider-consumer-relationships/provider-consumer-tracing-lab.md)

### Standard Nodes And Properties

- [ ] [Standard Nodes And Properties](../standard-nodes-and-properties.md)
- [ ] [Root Identity, Availability, And Aliases](../standard-nodes-and-properties/root-identity-availability-and-aliases.md)
- [ ] [CPUs, Topology, And Memory](../standard-nodes-and-properties/cpus-topology-and-memory.md)
- [ ] [Chosen And Boot Handoff](../standard-nodes-and-properties/chosen-and-boot-handoff.md)
- [ ] [Reserved Memory](../standard-nodes-and-properties/reserved-memory.md)
- [ ] [Cross-Cutting Standard Relationships](../standard-nodes-and-properties/cross-cutting-standard-relationships.md)
- [ ] [Standard Platform Tree Lab](../standard-nodes-and-properties/standard-platform-tree-lab.md)

### Addressing And Bus Modeling

- [ ] [Addressing And Bus Modeling](../addressing-and-bus-modeling.md)
- [ ] [Cell Counts, reg, And Unit Addresses](../addressing-and-bus-modeling/cell-counts-reg-and-unit-addresses.md)
- [ ] [simple-bus, ranges, And Nested Translation](../addressing-and-bus-modeling/simple-bus-ranges-and-nested-translation.md)
- [ ] [DMA Address Spaces And dma-ranges](../addressing-and-bus-modeling/dma-address-spaces-and-dma-ranges.md)
- [ ] [Interrupt Parents, Specifiers, And Routing](../addressing-and-bus-modeling/interrupt-parents-specifiers-and-routing.md)
- [ ] [Bus-Specific Child Addressing](../addressing-and-bus-modeling/bus-specific-child-addressing.md)
- [ ] [PCI Host Bridges, Address Windows, And Interrupts](../addressing-and-bus-modeling/pci-host-bridges-address-windows-and-interrupts.md)
- [ ] [Address Translation And Bus Modeling Lab](../addressing-and-bus-modeling/address-translation-and-bus-modeling-lab.md)

Stage completion:

- [ ] I can encode/decode every common DTS value and explain the exact FDT bytes produced.
- [ ] I can resolve provider specifiers, addresses, DMA translations, and interrupts through their parent/provider contracts.

## Stage 3: Linux OF Model, Devices, Matching, And Resource Relationships

Official tracker: [Linux OF Model, Devices, Matching, And Resource Relationships](03-linux-of-devices-matching-and-resources.md)

### Driver Matching

- [ ] [Driver Matching](../driver-matching.md)
- [ ] [Compatible Contracts And Fallback Chains](../driver-matching/compatible-contracts-and-fallback-chains.md)
- [ ] [Compatible Evolution And Stable ABI Decisions](../driver-matching/compatible-evolution-and-stable-abi-decisions.md)
- [ ] [From Device Tree Nodes To Linux Devices](../driver-matching/from-device-tree-nodes-to-linux-devices.md)
- [ ] [of_match_table, Variant Data, And Probe Selection](../driver-matching/of-match-table-variant-data-and-probe-selection.md)
- [ ] [Modaliases, Module Metadata, And Autoloading](../driver-matching/modaliases-module-metadata-and-autoloading.md)
- [ ] [Binding-Driven Probe Contracts](../driver-matching/binding-driven-probe-contracts.md)
- [ ] [Driver Matching And Probe Diagnosis Lab](../driver-matching/driver-matching-and-probe-diagnosis-lab.md)

### Pinctrl, GPIOs, And Interrupts

- [ ] [Pinctrl, GPIOs, And Interrupts](../pinctrl-gpios-and-interrupts.md)
- [ ] [Pinmux, Pin Configuration, And States](../pinctrl-gpios-and-interrupts/pinmux-pin-configuration-and-states.md)
- [ ] [GPIO Controllers, Ranges, Line Names, And Hogs](../pinctrl-gpios-and-interrupts/gpio-controllers-ranges-line-names-and-hogs.md)
- [ ] [GPIO Consumers, Polarity, And Reset Sequencing](../pinctrl-gpios-and-interrupts/gpio-consumers-polarity-and-reset-sequencing.md)
- [ ] [Interrupt Controllers, Specifiers, And Trigger Types](../pinctrl-gpios-and-interrupts/interrupt-controllers-specifiers-and-trigger-types.md)
- [ ] [GPIO Interrupts, Cascades, And Shared Lines](../pinctrl-gpios-and-interrupts/gpio-interrupts-cascades-and-shared-lines.md)
- [ ] [Sleep States, Wakeup, And Ownership](../pinctrl-gpios-and-interrupts/sleep-states-wakeup-and-ownership.md)
- [ ] [Pin, GPIO, And Interrupt Bring-Up Lab](../pinctrl-gpios-and-interrupts/pin-gpio-and-interrupt-bring-up-lab.md)

### Clocks, Resets, Regulators, And Power

- [ ] [Clocks, Resets, Regulators, And Power](../clocks-resets-regulators-and-power.md)
- [ ] [Clock Trees, Consumers, And Assignments](../clocks-resets-regulators-and-power/clock-trees-consumers-and-assignments.md)
- [ ] [Reset Controllers And Safe Sequencing](../clocks-resets-regulators-and-power/reset-controllers-and-safe-sequencing.md)
- [ ] [Regulators, Supplies, And Board Constraints](../clocks-resets-regulators-and-power/regulators-supplies-and-board-constraints.md)
- [ ] [Power Domains, Runtime PM, And Device Links](../clocks-resets-regulators-and-power/power-domains-runtime-pm-and-device-links.md)
- [ ] [Operating Points, DVFS, And Performance States](../clocks-resets-regulators-and-power/operating-points-dvfs-and-performance-states.md)
- [ ] [Thermal Zones, Trips, And Cooling Maps](../clocks-resets-regulators-and-power/thermal-zones-trips-and-cooling-maps.md)
- [ ] [Power Lifecycle, Ordering, And Diagnosis](../clocks-resets-regulators-and-power/power-lifecycle-ordering-and-diagnosis.md)
- [ ] [Integrated Power Bring-Up Lab](../clocks-resets-regulators-and-power/integrated-power-bring-up-lab.md)

Stage completion:

- [ ] I can trace a node through Linux device creation, matching, probe, deferral, binding, and subsystem function.
- [ ] I can diagnose pin, GPIO, interrupt, clock, reset, regulator, power-domain, DVFS, and thermal dependency chains.

## Stage 4: Standard Nodes, Buses, Providers, Peripherals, And Graphs

Official tracker: [Standard Nodes, Buses, Providers, Peripherals, And Graphs](04-standard-nodes-buses-providers-and-subsystems.md)

### Common Peripheral Nodes

- [ ] [Common Peripheral Nodes](../common-peripheral-nodes.md)
- [ ] [UARTs, Consoles, And Serial Children](../common-peripheral-nodes/uarts-consoles-and-serial-children.md)
- [ ] [I2C Controllers, Devices, And Muxes](../common-peripheral-nodes/i2c-controllers-devices-and-muxes.md)
- [ ] [SPI Controllers, Chip Selects, And Peripherals](../common-peripheral-nodes/spi-controllers-chip-selects-and-peripherals.md)
- [ ] [CAN Controllers, Transceivers, And Bit Timing](../common-peripheral-nodes/can-controllers-transceivers-and-bit-timing.md)
- [ ] [USB Controllers, PHYs, Roles, And Connectors](../common-peripheral-nodes/usb-controllers-phys-roles-and-connectors.md)
- [ ] [PCIe Host Bridges, Windows, And Enumeration](../common-peripheral-nodes/pcie-host-bridges-windows-and-enumeration.md)
- [ ] [MMC, SD, SDIO, And eMMC](../common-peripheral-nodes/mmc-sd-sdio-and-emmc.md)
- [ ] [Ethernet MACs, MDIO, PHYs, And Fixed Links](../common-peripheral-nodes/ethernet-macs-mdio-phys-and-fixed-links.md)
- [ ] [Board Services: LEDs, Keys, Watchdogs, RTC, Hwmon, And NVMEM](../common-peripheral-nodes/board-services-leds-keys-watchdogs-rtc-hwmon-and-nvmem.md)
- [ ] [MTD, SPI NOR, And Fixed Partitions](../common-peripheral-nodes/mtd-spi-nor-and-fixed-partitions.md)
- [ ] [Peripheral Integration And Diagnosis Lab](../common-peripheral-nodes/peripheral-integration-and-diagnosis-lab.md)

### Graph Bindings And Complex Data Paths

- [ ] [Graph Bindings And Complex Data Paths](../graph-bindings-and-complex-data-paths.md)
- [ ] [Graph Vocabulary, Containers, And Numbering](../graph-bindings-and-complex-data-paths/graph-vocabulary-containers-and-numbering.md)
- [ ] [Endpoint Links And Interface Contracts](../graph-bindings-and-complex-data-paths/endpoint-links-and-interface-contracts.md)
- [ ] [Display Pipelines: Controllers, Bridges, Panels, And Connectors](../graph-bindings-and-complex-data-paths/display-pipelines-controllers-bridges-panels-and-connectors.md)
- [ ] [Camera Pipelines: Sensors, Receivers, ISPs, And Capture](../graph-bindings-and-complex-data-paths/camera-pipelines-sensors-receivers-isps-and-capture.md)
- [ ] [Audio Graphs, DAI Links, And Routing](../graph-bindings-and-complex-data-paths/audio-graphs-dai-links-and-routing.md)
- [ ] [Multi-Endpoint Topologies, Crossbars, And Shared Resources](../graph-bindings-and-complex-data-paths/multi-endpoint-topologies-crossbars-and-shared-resources.md)
- [ ] [Lifecycle, Ownership, And Pipeline Power](../graph-bindings-and-complex-data-paths/lifecycle-ownership-and-pipeline-power.md)
- [ ] [Graph Validation And Runtime Diagnosis](../graph-bindings-and-complex-data-paths/graph-validation-and-runtime-diagnosis.md)
- [ ] [Complex Pipeline Integration Lab](../graph-bindings-and-complex-data-paths/complex-pipeline-integration-lab.md)

Stage completion:

- [ ] I can select and validate the exact bus/peripheral binding rather than copying a nearby node.
- [ ] I can model, power, validate, and diagnose multi-device endpoint graphs end to end.

## Stage 5: Memory, DMA, Firmware, Remoteproc, And Secure Boundaries

Official tracker: [Memory, DMA, Firmware, Remoteproc, And Secure Boundaries](05-memory-dma-firmware-and-heterogeneous-socs.md)

### Memory, Firmware, And Heterogeneous SoCs

- [ ] [Memory, Firmware, And Heterogeneous SoCs](../memory-firmware-and-heterogeneous-socs.md)
- [ ] [Memory Ownership, RAM, And Reserved Regions](../memory-firmware-and-heterogeneous-socs/memory-ownership-ram-and-reserved-regions.md)
- [ ] [CMA, Shared DMA Pools, And Static Carveouts](../memory-firmware-and-heterogeneous-socs/cma-shared-dma-pools-and-static-carveouts.md)
- [ ] [DMA Addressing, Coherency, And IOMMU Topology](../memory-firmware-and-heterogeneous-socs/dma-addressing-coherency-and-iommu-topology.md)
- [ ] [Firmware Images, Resource Tables, And Host Contracts](../memory-firmware-and-heterogeneous-socs/firmware-images-resource-tables-and-host-contracts.md)
- [ ] [Remoteproc Topology, Boot, Stop, And Recovery](../memory-firmware-and-heterogeneous-socs/remoteproc-topology-boot-stop-and-recovery.md)
- [ ] [RPMsg, Mailboxes, Virtqueues, And Shared Memory](../memory-firmware-and-heterogeneous-socs/rpmsg-mailboxes-virtqueues-and-shared-memory.md)
- [ ] [PRU, R5/M4, DSP, And Cluster Modeling](../memory-firmware-and-heterogeneous-socs/pru-r5-m4-dsp-and-cluster-modeling.md)
- [ ] [Trusted Firmware, OP-TEE, And Secure-World Boundaries](../memory-firmware-and-heterogeneous-socs/trusted-firmware-op-tee-and-secure-world-boundaries.md)
- [ ] [Heterogeneous SoC Integration And Recovery Lab](../memory-firmware-and-heterogeneous-socs/heterogeneous-soc-integration-and-recovery-lab.md)

Stage completion:

- [ ] I can map every RAM, reservation, pool, DMA address, IOVA, and firmware carveout without overlap.
- [ ] I can validate remote firmware, resource tables, IPC, lifecycle, crash recovery, and secure-world ownership.

## Stage 6: U-Boot, Boot Handoff, Mutation, FIT, And Overlays

Official tracker: [U-Boot, Boot Handoff, Mutation, FIT, And Overlays](06-u-boot-handoff-mutation-fit-and-overlays.md)

### U-Boot And Bootloader Device Tree

- [ ] [U-Boot And Bootloader Device Tree](../u-boot-and-bootloader-device-tree.md)
- [ ] [Control, Working, SPL, And Linux Device Trees](../u-boot-and-bootloader-device-tree/control-working-spl-and-linux-device-trees.md)
- [ ] [U-Boot DT Sources, Upstream Sync, And Build Artifacts](../u-boot-and-bootloader-device-tree/u-boot-dt-sources-upstream-sync-and-build-artifacts.md)
- [ ] [Driver Model, Boot Phases, And Pre-Relocation Properties](../u-boot-and-bootloader-device-tree/driver-model-boot-phases-and-pre-relocation-properties.md)
- [ ] [TPL, SPL, SRAM Budgets, And Multi-DTB Selection](../u-boot-and-bootloader-device-tree/tpl-spl-sram-budgets-and-multi-dtb-selection.md)
- [ ] [FIT Configurations, DTB Selection, And Verified Boot](../u-boot-and-bootloader-device-tree/fit-configurations-dtb-selection-and-verified-boot.md)
- [ ] [Environment, Bootstd, Extlinux, And OS DTB Loading](../u-boot-and-bootloader-device-tree/environment-bootstd-extlinux-and-os-dtb-loading.md)
- [ ] [Bootloader Overlay Application And Working-FDT Safety](../u-boot-and-bootloader-device-tree/bootloader-overlay-application-and-working-fdt-safety.md)
- [ ] [Binman, Firmware Packaging, And DT-Based Image Layout](../u-boot-and-bootloader-device-tree/binman-firmware-packaging-and-dt-based-image-layout.md)
- [ ] [Bootloader DT Selection And Handoff Lab](../u-boot-and-bootloader-device-tree/bootloader-dt-selection-and-handoff-lab.md)

### Boot-Time Mutation And Ownership

- [ ] [Boot-Time Mutation And Ownership](../boot-time-mutation-and-ownership.md)
- [ ] [Mutation Provenance, Authorities, And Checkpoints](../boot-time-mutation-and-ownership/mutation-provenance-authorities-and-checkpoints.md)
- [ ] [Libfdt Capacity, Relocation, And Failure Atomicity](../boot-time-mutation-and-ownership/libfdt-capacity-relocation-and-failure-atomicity.md)
- [ ] [RAM Discovery, Reservations, And Memory Fixups](../boot-time-mutation-and-ownership/ram-discovery-reservations-and-memory-fixups.md)
- [ ] [/chosen, Boot Arguments, Initrd, Console, And Seeds](../boot-time-mutation-and-ownership/chosen-bootargs-initrd-console-and-seeds.md)
- [ ] [MAC Addresses, Serial Numbers, And Board Identity](../boot-time-mutation-and-ownership/mac-addresses-serial-numbers-and-board-identity.md)
- [ ] [Overlay Order, Composition, And Conflict Ownership](../boot-time-mutation-and-ownership/overlay-order-composition-and-conflict-ownership.md)
- [ ] [Firmware, Secure World, And Cross-Stage Ownership](../boot-time-mutation-and-ownership/firmware-secure-world-and-cross-stage-ownership.md)
- [ ] [Final-Tree Validation, Diffing, And Runtime Forensics](../boot-time-mutation-and-ownership/final-tree-validation-diffing-and-runtime-forensics.md)
- [ ] [Boot-Time Mutation Provenance Lab](../boot-time-mutation-and-ownership/boot-time-mutation-provenance-lab.md)

### Overlays In Depth

- [ ] [Overlays In Depth](../overlays-in-depth.md)
- [ ] [Overlay Source, Fragments, And Target Selection](../overlays-in-depth/overlay-source-fragments-and-target-selection.md)
- [ ] [Symbols, Fixups, Local Fixups, And Compilation](../overlays-in-depth/symbols-fixups-local-fixups-and-compilation.md)
- [ ] [Resolver, Phandle Relocation, And Merge Semantics](../overlays-in-depth/resolver-phandle-relocation-and-merge-semantics.md)
- [ ] [Base Compatibility, Versioning, And Overlay ABI](../overlays-in-depth/base-compatibility-versioning-and-overlay-abi.md)
- [ ] [Stacking, Dependencies, Conflicts, And Removal Order](../overlays-in-depth/stacking-dependencies-conflicts-and-removal-order.md)
- [ ] [Linux Runtime Overlays, Devices, Notifiers, And Lifetime](../overlays-in-depth/linux-runtime-overlays-devices-notifiers-and-lifetime.md)
- [ ] [Validation, Security, And Product Architecture](../overlays-in-depth/validation-security-and-product-architecture.md)
- [ ] [Overlay Composition And Lifecycle Lab](../overlays-in-depth/overlay-composition-and-lifecycle-lab.md)

Stage completion:

- [ ] I can identify every boot-stage tree, selected artifact, fixup, overlay, and final handoff.
- [ ] I can reason about FIT trust, libfdt capacity, overlay resolution, stacking, device lifetime, and safe removal.

## Stage 7: Binding ABI, Schema Authoring, Validation, And Upstreaming

Official tracker: [Binding ABI, Schema Authoring, Validation, And Upstreaming](07-binding-abi-schema-validation-and-upstreaming.md)

### Binding Design And Stable ABI

- [ ] [Binding Design And Stable ABI](../binding-design-and-stable-abi.md)
- [ ] [Hardware Description Contracts And Policy Boundaries](../binding-design-and-stable-abi/hardware-description-contracts-and-policy-boundaries.md)
- [ ] [Complete Bindings And Extensible Hardware Models](../binding-design-and-stable-abi/complete-bindings-and-extensible-hardware-models.md)
- [ ] [Property Design, Naming, Units, And Standard Reuse](../binding-design-and-stable-abi/property-design-naming-units-and-standard-reuse.md)
- [ ] [Compatible Identities, Fallbacks, And Variant Data](../binding-design-and-stable-abi/compatible-identities-fallbacks-and-variant-data.md)
- [ ] [Backward Compatibility, Deprecation, And Migration](../binding-design-and-stable-abi/backward-compatibility-deprecation-and-migration.md)
- [ ] [Board Revisions, Products, And Deployment Matrices](../binding-design-and-stable-abi/board-revisions-products-and-deployment-matrices.md)
- [ ] [Review Strategy And Upstream Submission Order](../binding-design-and-stable-abi/review-strategy-and-upstream-submission-order.md)
- [ ] [Binding Design And ABI Review Lab](../binding-design-and-stable-abi/binding-design-and-abi-review-lab.md)

### Writing And Validating Binding Schemas

- [ ] [Writing And Validating Binding Schemas](../writing-and-validating-binding-schemas.md)
- [ ] [Schema Anatomy, Identity, And Node Selection](../writing-and-validating-binding-schemas/schema-anatomy-identity-and-node-selection.md)
- [ ] [Property Types, Encodings, And Cardinality](../writing-and-validating-binding-schemas/property-types-encodings-and-cardinality.md)
- [ ] [References, Composition, Conditionals, And Closure](../writing-and-validating-binding-schemas/references-composition-conditionals-and-closure.md)
- [ ] [Child Nodes, Name Patterns, And Common Schemas](../writing-and-validating-binding-schemas/child-nodes-name-patterns-and-common-schemas.md)
- [ ] [Examples, Vendor Bindings, And Authoring Workflow](../writing-and-validating-binding-schemas/examples-vendor-bindings-and-authoring-workflow.md)
- [ ] [Validation Toolchain And Targeted Checks](../writing-and-validating-binding-schemas/validation-toolchain-and-targeted-checks.md)
- [ ] [Diagnosing Schema, Example, And DTB Failures](../writing-and-validating-binding-schemas/diagnosing-schema-example-and-dtb-failures.md)
- [ ] [Binding Schema Authoring And Validation Lab](../writing-and-validating-binding-schemas/binding-schema-authoring-and-validation-lab.md)

### Build And Diagnostic Tools

- [ ] [Build And Diagnostic Tools](../build-and-diagnostic-tools.md)
- [ ] [Build Pipeline, Preprocessing, And Artifact Provenance](../build-and-diagnostic-tools/build-pipeline-preprocessing-and-artifact-provenance.md)
- [ ] [dtc, Symbols, Warnings, And Round Trips](../build-and-diagnostic-tools/dtc-symbols-warnings-and-round-trips.md)
- [ ] [fdtdump And fdtget Binary Inspection](../build-and-diagnostic-tools/fdtdump-and-fdtget-binary-inspection.md)
- [ ] [fdtput, fdtoverlay, And Controlled Artifact Mutation](../build-and-diagnostic-tools/fdtput-fdtoverlay-and-controlled-artifact-mutation.md)
- [ ] [U-Boot fdt Commands And Handoff Inspection](../build-and-diagnostic-tools/u-boot-fdt-commands-and-handoff-inspection.md)
- [ ] [libfdt Programming, Capacity, And Error Discipline](../build-and-diagnostic-tools/libfdt-programming-capacity-and-error-discipline.md)
- [ ] [Diagnostic Workflow, Semantic Diffing, And CI Evidence](../build-and-diagnostic-tools/diagnostic-workflow-semantic-diffing-and-ci-evidence.md)
- [ ] [Device Tree Artifact Provenance And Diagnosis Lab](../build-and-diagnostic-tools/device-tree-artifact-provenance-and-diagnosis-lab.md)

Stage completion:

- [ ] I can design and evolve a stable hardware binding, author its schema, and diagnose every validation layer.
- [ ] I can reproduce, inspect, compare, mutate safely, validate, and upstream Device Tree artifacts and contracts.

## Stage 8: Runtime Diagnostics, Security, Production, And Board Porting

Official tracker: [Runtime Diagnostics, Security, Production, And Board Porting](08-runtime-security-production-and-board-porting.md)

### Runtime Inspection

- [ ] [Runtime Inspection](../runtime-inspection.md)
- [ ] [Runtime Tree Surfaces And Boot-FDT Identity](../runtime-inspection/runtime-tree-surfaces-and-boot-fdt-identity.md)
- [ ] [Binary-Safe Property Inspection And Decoding](../runtime-inspection/binary-safe-property-inspection-and-decoding.md)
- [ ] [Live-Tree Capture, Normalization, And Semantic Diffing](../runtime-inspection/live-tree-capture-normalization-and-semantic-diffing.md)
- [ ] [From Live Device Tree Node To Linux Device](../runtime-inspection/from-live-device-tree-node-to-linux-device.md)
- [ ] [Matching, Modaliases, Modules, And Bound Drivers](../runtime-inspection/matching-modaliases-modules-and-bound-drivers.md)
- [ ] [Probe Deferral, Supplier Links, And Resource State](../runtime-inspection/probe-deferral-supplier-links-and-resource-state.md)
- [ ] [Controlled Bind/Unbind And Runtime Forensics](../runtime-inspection/controlled-bind-unbind-and-runtime-forensics.md)
- [ ] [Runtime Device Tree And Probe Forensics Lab](../runtime-inspection/runtime-device-tree-and-probe-forensics-lab.md)

### Security And Production Lifecycle

- [ ] [Security And Production Lifecycle](../security-and-production-lifecycle.md)
- [ ] [Device Tree Threat Model And Trust Boundaries](../security-and-production-lifecycle/device-tree-threat-model-and-trust-boundaries.md)
- [ ] [FIT Authenticated Selection And Key Policy](../security-and-production-lifecycle/fit-authenticated-selection-and-key-policy.md)
- [ ] [Mutation, Overlay, And Fixup Chain Of Custody](../security-and-production-lifecycle/mutation-overlay-and-fixup-chain-of-custody.md)
- [ ] [Measured Boot, Attestation, And Runtime Evidence](../security-and-production-lifecycle/measured-boot-attestation-and-runtime-evidence.md)
- [ ] [Versioned Release Sets, Compatibility, And Rollback](../security-and-production-lifecycle/versioned-release-sets-compatibility-and-rollback.md)
- [ ] [Reproducible DTB Builds, Provenance, And Manifests](../security-and-production-lifecycle/reproducible-dtb-builds-provenance-and-manifests.md)
- [ ] [Field Updates, Recovery, And Key Rotation](../security-and-production-lifecycle/field-updates-recovery-and-key-rotation.md)
- [ ] [Secure Device Tree Release And Update Lab](../security-and-production-lifecycle/secure-device-tree-release-and-update-lab.md)

### Product-Scale Maintenance And Engineering

- [ ] [Product-Scale Maintenance And Engineering](../product-scale-maintenance-and-engineering.md)
- [ ] [Specification, Binding, And Toolchain Compatibility Baselines](../product-scale-maintenance-and-engineering/specification-binding-and-toolchain-compatibility-baselines.md)
- [ ] [Layering, Variants, Ownership, And Source Architecture](../product-scale-maintenance-and-engineering/layering-variants-ownership-and-source-architecture.md)
- [ ] [Multidimensional Review And Change Design](../product-scale-maintenance-and-engineering/multidimensional-review-and-change-design.md)
- [ ] [Upstream Development And Downstream Patch-Stack Discipline](../product-scale-maintenance-and-engineering/upstream-development-and-downstream-patch-stack-discipline.md)
- [ ] [Binding Evolution, Deprecation, And Migration](../product-scale-maintenance-and-engineering/binding-evolution-deprecation-and-migration.md)
- [ ] [Matrix CI, Artifact Validation, And Compatibility Testing](../product-scale-maintenance-and-engineering/matrix-ci-artifact-validation-and-compatibility-testing.md)
- [ ] [Hardware Coverage, Release Qualification, And Learning From Escapes](../product-scale-maintenance-and-engineering/hardware-coverage-release-qualification-and-learning-from-escapes.md)
- [ ] [Product-Family Maintenance And Regression Prevention Lab](../product-scale-maintenance-and-engineering/product-family-maintenance-and-regression-prevention-lab.md)

### Board Porting Workflow

- [ ] [Board Porting Workflow](../board-porting-workflow.md)
- [ ] [Reference Board Selection, Hardware Delta, And Evidence Baseline](../board-porting-workflow/reference-board-selection-hardware-delta-and-evidence-baseline.md)
- [ ] [Minimal DTB, Boot Handoff, Memory, Console, And Boot Storage](../board-porting-workflow/minimal-dtb-boot-handoff-memory-console-and-boot-storage.md)
- [ ] [Power, Clock, Reset, Pinctrl, And GPIO Bring-Up](../board-porting-workflow/power-clock-reset-pinctrl-and-gpio-bring-up.md)
- [ ] [Buses, Storage, Networking, And Peripheral Enablement](../board-porting-workflow/buses-storage-networking-and-peripheral-enablement.md)
- [ ] [DMA, IOMMU, Reserved Memory, And Remote Processor Integration](../board-porting-workflow/dma-iommu-reserved-memory-and-remote-processor-integration.md)
- [ ] [Board Revisions, Variants, Overlays, And Identity](../board-porting-workflow/board-revisions-variants-overlays-and-identity.md)
- [ ] [Validation, Upstreaming, And Production Handoff](../board-porting-workflow/validation-upstreaming-and-production-handoff.md)
- [ ] [Custom Board Porting Capstone Lab](../board-porting-workflow/custom-board-porting-capstone-lab.md)

Stage completion:

- [ ] I can prove the deployed/live tree and diagnose population, matching, probe, suppliers, and function.
- [ ] I can secure, release, maintain, update, recover, and port Device Tree platforms with auditable evidence.

## Overall Completion

- [ ] All 182 knowledge-guide page checkboxes are complete.
- [ ] All eight official-documentation stage checkboxes are complete.
- [ ] I have recorded version differences among DTSpec, upstream/vendor Linux, dtc/libfdt, dt-schema, and U-Boot.
- [ ] I have completed at least one binding/schema lab, one runtime forensics lab, and the board-porting capstone.
- [ ] I can trace one production board from hardware records through source, schema, build, boot selection/mutation, Linux devices, release security, and field recovery.
- [ ] I maintain a list of topics needing another pass or project-specific deep dive.

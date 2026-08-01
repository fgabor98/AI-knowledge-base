---
status: active
reviewed: false
domain: device-tree
difficulty: intermediate
last_reviewed: null
---

# Official Device Tree Documentation Reading Checklist

This is a progress tracker for a detailed reading of the authoritative Device Tree material spread across the [Devicetree Specification](https://www.devicetree.org/specifications/), Linux kernel documentation and bindings, the `dt-schema` and `dtc` projects, and U-Boot documentation.

There is no single official table of contents covering the full engineering lifecycle. This checklist therefore preserves each source's authority while ordering the reading so that tree semantics and binary encoding come first, Linux binding and driver contracts come next, and bootloader, tooling, runtime, security, and production concerns build on them.

## Personal Project Focus

The priority order is tailored to the work represented in this knowledge base:

- embedded Linux board bring-up and custom hardware ports
- DTS/DTSI composition and DTB/DTBO artifact provenance
- Linux device creation, driver matching, and deferred probe
- clocks, resets, regulators, pin control, GPIO, interrupts, DMA, and IOMMUs
- I2C, SPI, UART, CAN, MMC, Ethernet, USB, PCIe, MTD, audio, display, and camera
- U-Boot control/working trees, FIT selection, overlays, and fixups
- binding design, YAML schema authoring, `dtbs_check`, and upstream submission
- reserved memory, remoteproc, RPMsg, PRU, R5/M4, DSP, and secure firmware
- runtime forensics, verified/measured boot, field updates, and product maintenance

## Authority Model

Use the source that owns the contract:

| Question | Primary authority |
|---|---|
| generic tree model, standard properties, nodes, FDT/DTS format | released Devicetree Specification |
| Linux binding ABI and accepted hardware representation | binding schemas in the exact Linux tree plus current upstream bindings |
| Linux OF implementation and APIs | exact Linux source and matching `docs.kernel.org` version |
| DTS compiler behavior and libfdt APIs | exact `dtc`/libfdt source and version |
| schema tooling behavior | exact `dt-schema` release/source |
| U-Boot control tree, selection, FIT, fixups, and commands | exact U-Boot source/configuration and corresponding docs |
| SoC/board electrical facts | hardware manuals, schematics, BOM, errata, and firmware contracts |

Tutorials and vendor examples are useful orientation, but they do not override a binding, specification, or the source version actually deployed.

## How To Use The Checklists

For every reading session, record the baseline:

```text
DTSpec release:
Linux documentation/kernel tag and commit:
Vendor kernel and patch baseline, if applicable:
dtc/libfdt version and commit:
dt-schema version:
U-Boot tag/commit and configuration:
Architecture/SoC/board:
Date:
```

Checkbox priorities:

- **P0**: read closely during the main path.
- **P1**: read after the main path or when beginning the related project.
- **P2**: retain as a reference and read when a device, subsystem, or product requirement makes it relevant.

Mark a checkbox only after you can explain the document's contract in your own words. For P0 items, also locate the corresponding source/schema and inspect at least one real DTS plus one consumer.

Use the [Knowledge Guide Companion Checklist](knowledge-guide-companion.md) to track all 182 Device Tree knowledge-guide pages in the same eight-stage order.

## Recommended Path

- [ ] [Knowledge Guide Companion Checklist](knowledge-guide-companion.md)
- [ ] 1. [Specification Foundations And The Core Tree Model](01-specification-foundations-and-core-tree-model.md)
- [ ] 2. [DTS Source, FDT Binary Format, dtc, And libfdt](02-dts-fdt-dtc-and-libfdt.md)
- [ ] 3. [Linux OF Model, Devices, Matching, And Resource Relationships](03-linux-of-devices-matching-and-resources.md)
- [ ] 4. [Standard Nodes, Buses, Providers, Peripherals, And Graphs](04-standard-nodes-buses-providers-and-subsystems.md)
- [ ] 5. [Memory, DMA, Firmware, Remoteproc, And Secure Boundaries](05-memory-dma-firmware-and-heterogeneous-socs.md)
- [ ] 6. [U-Boot, Boot Handoff, Mutation, FIT, And Overlays](06-u-boot-handoff-mutation-fit-and-overlays.md)
- [ ] 7. [Binding ABI, Schema Authoring, Validation, And Upstreaming](07-binding-abi-schema-validation-and-upstreaming.md)
- [ ] 8. [Runtime Diagnostics, Security, Production, And Board Porting](08-runtime-security-production-and-board-porting.md)

## Official-Source Coverage

| Authoritative area | Checklist |
|---|---|
| DTSpec introduction, terminology, tree model, standard properties/nodes | 01 |
| DTSpec DTS and FDT formats; dtc/libfdt | 02 |
| Linux OF usage, APIs, device population, matching, and dependency behavior | 03 |
| Linux common/subsystem bindings and topology documentation | 04 |
| reserved memory, DMA/IOMMU, firmware, remoteproc/RPMsg, trusted firmware | 05 |
| U-Boot Device Tree control, selection, FIT, fixups, overlays; Linux overlays | 06 |
| DT ABI, design rules, YAML schema, tooling, style, submission process | 07 |
| sysfs/runtime inspection, testing, verified/measured boot, release practice | 08 |

## Deep-Reading Loop

For each P0 document or binding group:

```text
read the normative contract
-> record required, optional, ordered, and default behavior
-> locate the exact schema/source in the project tree
-> find two valid in-tree DTS users
-> trace one consumer from property to implementation
-> compile/inspect or design one focused experiment
-> record version and project-specific differences
```

Useful note fields:

```text
Normative source and version:
Binding/schema path:
Node selection / compatible contract:
Property types, ordering, cardinality, and defaults:
Provider/consumer cell decoding:
Address and interrupt parent context:
Bootloader mutation/ownership:
Linux device/driver path:
Validation commands and warnings:
Runtime evidence:
Compatibility/ABI implications:
Open questions:
```

## Refresh Policy

The released DTSpec, current upstream Linux bindings, `dt-schema`, `dtc`, and U-Boot evolve on independent schedules. At the start of a serious project:

1. read the released DTSpec baseline used by the project;
2. build documentation and inspect bindings from the exact kernel tree being shipped;
3. compare with current upstream guidance for fixes, new schemas, and deprecations;
4. pin the actual `dtc`, libfdt, `dt-schema`, and U-Boot versions;
5. record downstream bindings, warnings, quirks, and fixups explicitly.

Never mix an unversioned web page, an old vendor DTS, and a newer schema result without naming the version boundary.

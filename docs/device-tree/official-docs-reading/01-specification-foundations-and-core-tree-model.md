---
status: active
reviewed: false
domain: device-tree
difficulty: beginner
last_reviewed: null
---

# 1. Specification Foundations And The Core Tree Model

Official section: [Devicetree Specification releases](https://www.devicetree.org/specifications/)

Knowledge-guide companion: [Stage 1](knowledge-guide-companion.md#stage-1-specification-foundations-and-the-core-tree-model)

## Baseline And Orientation

- [ ] **P0** Download or bookmark the exact released DTSpec used for the project; start with [DTSpec v0.4](https://github.com/devicetree-org/devicetree-specification/releases/tag/v0.4) when no project baseline is prescribed.
- [ ] **P0** Read the [DTSpec introduction and terminology](https://devicetree-specification.readthedocs.io/en/stable/).
- [ ] **P1** Read [devicetree.org specifications](https://www.devicetree.org/specifications/) and record the current released version separately from the Read the Docs `latest` branch.
- [ ] **P1** Inspect the [DTSpec source and issue tracker](https://github.com/devicetree-org/devicetree-specification) when wording is ambiguous or newer work matters.
- [ ] **P2** Compare relevant release notes/diffs when supporting multiple DTSpec baselines.

## Logical Tree And Vocabulary

Official chapter: [The Devicetree](https://devicetree-specification.readthedocs.io/en/stable/devicetree-basics.html)

- [ ] **P0** Node names, unit addresses, full paths, and root-node rules.
- [ ] **P0** Property names, raw values, empty properties, strings, string lists, cells, encoded arrays, and phandles.
- [ ] **P0** `compatible` ordering from most specific to most general.
- [ ] **P0** `model`, `phandle`, and `status` contracts.
- [ ] **P0** `#address-cells`, `#size-cells`, `reg`, and the fact that child encodings are interpreted through the parent.
- [ ] **P0** `ranges` and nested address translation.
- [ ] **P0** `dma-ranges` and the distinction between CPU and DMA-visible address spaces.
- [ ] **P0** interrupt parent inheritance, `interrupts`, and `interrupts-extended`.
- [ ] **P1** `virtual-reg`, `dma-coherent`, `name`, and other less commonly used standard properties when present in project trees.
- [ ] **P1** Understand that labels are DTS source constructs while phandles are properties in the encoded tree.

## Standard Device Nodes

Official chapter: [Device Node Requirements](https://devicetree-specification.readthedocs.io/en/stable/devicenodes.html)

- [ ] **P0** Root node requirements and root `compatible`/`model`.
- [ ] **P0** `/aliases` syntax and client interpretation.
- [ ] **P0** `/memory` nodes and `device_type = "memory"`.
- [ ] **P0** `/chosen`, `bootargs`, `stdout-path`, and boot-program/client ownership.
- [ ] **P0** `/cpus`, CPU `reg`, enable methods, and timebase/clock properties relevant to the architecture.
- [ ] **P1** NUMA distance mapping when the platform uses it.
- [ ] **P1** Power ISA-specific nodes only when working on that architecture.

## DTSpec Device Bindings

Official chapter: [Device Bindings](https://devicetree-specification.readthedocs.io/en/stable/device-bindings.html)

- [ ] **P0** General binding principles and vendor-prefixed property conventions.
- [ ] **P0** `simple-bus` and its `ranges` requirement.
- [ ] **P1** Serial class and ns16550 binding as a compact example.
- [ ] **P1** Network class, MAC-address properties, PHY relationship, and interface type.
- [ ] **P2** Open PIC and Power ISA bindings only for relevant systems.

The DTSpec binding chapter is a base, not a replacement for Linux's actively maintained YAML schemas. When both apply, satisfy the normative generic model and the exact device schema.

## Linux Orientation

- [ ] **P0** [Linux and the Devicetree](https://docs.kernel.org/devicetree/usage-model.html): platform identification, early runtime configuration, and device population.
- [ ] **P0** [Linux Devicetree documentation index](https://docs.kernel.org/devicetree/index.html).
- [ ] **P1** Identify the architecture boot document that defines the actual DTB handoff, such as [Booting AArch64 Linux](https://docs.kernel.org/arch/arm64/booting.html).
- [ ] **P1** Locate `include/linux/of.h`, `drivers/of/`, and architecture early-DT code in the exact kernel tree.

## Source Exercises

- [ ] Draw one real tree with node names, paths, parent relationships, and property byte types.
- [ ] Decode a `reg` property using its parent's address/size cells without relying on decompiled formatting.
- [ ] Translate one child address through `ranges` to a CPU physical address.
- [ ] Decode one interrupt specifier using its resolved interrupt-parent binding.
- [ ] Explain why `compatible = "vendor,new", "vendor,old"` is a behavioral compatibility claim.
- [ ] Compare the DTSpec rule with the Linux schema for one root, CPU, memory, chosen, and simple-bus node.

## Stage Completion

- [ ] I can distinguish DTSpec, a project binding, DTS syntax, FDT encoding, and Linux implementation behavior.
- [ ] I can decode common property types and parent-relative cells manually.
- [ ] I can explain root identity, standard nodes, address translation, interrupt inheritance, and compatible fallback.
- [ ] I have recorded the exact specification and kernel documentation baseline for my project.


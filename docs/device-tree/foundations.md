---
status: draft
reviewed: false
domain: device-tree
difficulty: beginner
last_reviewed: null
---

# Device Tree Foundations

Device Tree becomes much easier once it stops looking like a collection of punctuation and starts looking like a hardware contract. This module builds that mental model before introducing the denser rules for buses, interrupts, clocks, schemas, and overlays.

## Learning Outcomes

After completing this module, you should be able to:

- explain why non-discoverable hardware needs a description outside the driver
- distinguish hardware facts from operating-system policy and driver implementation details
- explain the roles of DTS, DTSI, DTB, and DTBO files
- trace a tree from source through the bootloader to Linux's runtime representation
- read a small tree using nodes, properties, paths, labels, aliases, and phandles
- distinguish the logical tree from its flattened binary encoding
- use a binding to decide what a node means instead of guessing from its spelling
- compile, decompile, and inspect a small DTB
- recognize when a valid source file still describes invalid or incomplete hardware

## Prerequisites

You should be comfortable with:

- basic embedded hardware concepts such as buses, register ranges, interrupts, and GPIO signals
- hexadecimal numbers and simple command-line workflows
- the difference between source files and compiled artifacts
- the broad roles of firmware, a bootloader, and an operating-system kernel

You do not need prior Device Tree experience or kernel-driver experience.

## Learning Path

1. [The Hardware Description Model](foundations/hardware-description-model.md) — why Device Tree exists, what belongs in it, and why bindings are an ABI.
2. [Source And Binary Artifacts](foundations/source-and-binary-artifacts.md) — DTS, DTSI, DTB, DTBO, source composition, flattening, and boot-time handoff.
3. [Tree Anatomy And Vocabulary](foundations/tree-anatomy-and-vocabulary.md) — nodes, properties, values, names, paths, labels, aliases, and phandles.
4. [First Device Tree Lab](foundations/first-device-tree-lab.md) — compile, inspect, modify, and diagnose a small tree.

Follow the pages in order on a first pass. The examples deliberately reuse one fictional board so that each page adds a new layer without changing the hardware model.

## The Core Mental Model

```text
board schematic and SoC documentation
                |
                v
      Device Tree source + bindings
                |
                v
        compiled and selected DTB
                |
                v
       firmware/bootloader handoff
                |
                v
       operating system interprets
       the described hardware
                |
                v
      devices match drivers and the
      drivers request named resources
```

Three contracts meet in this path:

| Contract | Main question |
|---|---|
| hardware contract | What exists, where is it, and how is it wired? |
| binding contract | Which nodes and properties express those facts, and how are their values encoded? |
| software contract | Which driver understands the binding, and which resources does it request? |

Most Device Tree debugging is the process of finding which contract is inconsistent.

## Scope Of This Module

This foundations module explains enough syntax to read its examples, but it does not try to teach every source-language feature. Detailed cell encoding, deletion directives, preprocessing, and source layering continue in [Syntax, Values, And Source Composition](syntax-values-and-source-composition.md). Likewise, detailed bus translation, provider specifiers, driver matching, schema validation, overlays, and production diagnostics each have their own pages.

## Common Misconceptions To Leave Behind

- A DTB is not automatically the DTB that the board booted.
- A node does not make a disabled kernel driver exist.
- A syntactically valid DTS is not necessarily binding-correct.
- A label is a source convenience; a path identifies a node in the logical tree.
- A phandle is not a pointer or an address.
- A unit address is not necessarily a CPU physical address; its meaning comes from the parent bus.
- Device Tree describes hardware, but firmware and bootloaders may also modify the tree before Linux sees it.

## Completion Check

You are ready for the next module when you can answer all of these without referring to the text:

1. Why can PCI usually enumerate a child device while an SoC UART usually needs firmware description?
2. What is the difference between a binding and a driver?
3. Which artifact does a bootloader normally pass to Linux?
4. Why can editing a DTS have no effect on the running target?
5. How do `uart0:`, `/soc/serial@1000`, and `serial0` differ?
6. Why does `clocks = <&osc 0>;` require the clock provider's binding to decode?
7. What are the major blocks inside a flattened DTB?

## Authoritative References

- [Devicetree Specification releases](https://www.devicetree.org/specifications/)
- [Linux and the Devicetree](https://docs.kernel.org/devicetree/usage-model.html)
- [Devicetree bindings documentation](https://docs.kernel.org/devicetree/bindings/)
- [Linux DTS coding style](https://docs.kernel.org/devicetree/bindings/dts-coding-style.html)

## Related Topics

- [Device Tree](index.md)
- [Syntax, Values, And Source Composition](syntax-values-and-source-composition.md)
- [Driver Matching](driver-matching.md)
- [Build And Diagnostic Tools](build-and-diagnostic-tools.md)

---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# Device Tree Hardware Description

## What Problem Does This Solve?

Device Tree describes non-discoverable hardware so Linux can create devices and pass board-specific wiring to reusable drivers.

## Core Concepts

- DTS and DTSI files
- DTB
- nodes and properties
- `compatible`
- `reg`
- interrupts
- GPIOs
- clocks
- regulators
- pinctrl

## Mental Model

The board file moved into data. Device Tree should describe what exists and how it is connected; the driver should interpret only the binding-defined properties it supports.

## Practice Skeleton

- Find the runtime DTB used by the board.
- Add a simple platform device node.
- Add resources such as `reg`, interrupts, and GPIOs.
- Confirm Linux creates the device.

## Debugging Checklist

- Compare source DTS with `/proc/device-tree`.
- Check include-file layering.
- Check address and interrupt cells.
- Validate against binding documentation where possible.

## Related Topics

- [Device Tree](../../device-tree/index.md)
- [Device Tree Matching From Drivers](device-tree-matching.md)
- [Device Tree Builds](../../build-systems/advanced/linux-kernel/device-tree-builds.md)

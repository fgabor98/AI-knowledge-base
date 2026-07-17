---
status: draft
reviewed: false
domain: device-tree
difficulty: intermediate
last_reviewed: null
---

# Driver Matching

This page explains the binding contract that connects hardware descriptions to reusable drivers.

## Topics Covered

- `compatible`
- fallback compatible strings
- board-compatible vs SoC-compatible fallback chains
- backward compatibility and when to introduce a new `compatible`
- `of_match_table`
- platform devices
- modalias
- binding-driven driver expectations
- optional vs required properties

## Related Topics

- [Binding Design And Stable ABI](binding-design-and-stable-abi.md)
- [Linux Kernel Device Tree Matching From Drivers](../linux-kernel/fundamentals/device-tree-matching.md)

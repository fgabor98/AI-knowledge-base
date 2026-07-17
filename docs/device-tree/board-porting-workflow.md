---
status: draft
reviewed: false
domain: device-tree
difficulty: intermediate
last_reviewed: null
---

# Board Porting Workflow

This page provides a staged Device Tree workflow for moving from a reference board to custom hardware.

## Topics Covered

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

- [Common Peripheral Nodes](common-peripheral-nodes.md)
- [Runtime Inspection](runtime-inspection.md)
- [Custom Sitara Board Bring-Up](../build-systems/advanced/ti-processor-sdk/custom-sitara-board-bring-up.md)

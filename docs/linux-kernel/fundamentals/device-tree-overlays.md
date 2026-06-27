---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# Device Tree Overlays

## What Problem Does This Solve?

Device Tree overlays describe hardware additions or board variants without replacing the whole base Device Tree.

## Core Concepts

- overlay source
- fragments
- target nodes
- labels
- symbols
- bootloader-applied overlays
- kernel-applied overlays
- board variant policy

## Mental Model

An overlay is a patch to the hardware description. It must be compatible with the exact base tree it targets.

## Practice Skeleton

- Build a small overlay.
- Apply it through the board's supported boot flow.
- Confirm the node appears in the runtime tree.
- Confirm the target driver probes.

## Debugging Checklist

- Check base DTB compatibility.
- Check overlay application order.
- Check labels and symbols.
- Avoid using overlays to hide unresolved board ownership decisions.

## Related Topics

- [Device Tree](../../device-tree/index.md)
- [Device Tree Builds](../../build-systems/advanced/linux-kernel/device-tree-builds.md)
- [Device Tree Matching From Drivers](device-tree-matching.md)

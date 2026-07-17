---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# Overlays In Depth

This page covers overlay compilation, resolution, application, compatibility, and lifecycle hazards.

## Topics Covered

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

## Related Topics

- [Syntax, Values, And Source Composition](syntax-values-and-source-composition.md)
- [Boot-Time Mutation And Ownership](boot-time-mutation-and-ownership.md)

---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# Boot-Time Mutation And Ownership

This page traces how firmware and bootloaders select, relocate, and modify a Device Tree before Linux receives it.

## Topics Covered

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

## Related Topics

- [U-Boot And Bootloader Device Tree](u-boot-and-bootloader-device-tree.md)
- [Runtime Inspection](runtime-inspection.md)

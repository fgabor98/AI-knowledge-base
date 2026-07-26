---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# U-Boot And Bootloader Device Tree

A boot can involve several DTBs that look similar but have different consumers and lifetimes. U-Boot proper uses a control DTB for its driver model, SPL or TPL may use smaller phase-specific trees, and Linux receives a working DTB selected and prepared by the boot flow. Reliable bring-up starts by naming which tree a claim refers to.

## Learning Outcomes

After completing this module, you should be able to:

- distinguish U-Boot's control FDT, working FDT, SPL/TPL DTBs, FIT image tree, binman image description, and Linux-bound DTB
- trace the sources and build rules that produce each bootloader DT artifact
- explain how U-Boot-specific source fragments coexist with shared upstream hardware descriptions
- select `bootph-*` properties from actual pre-relocation dependencies
- reason about SRAM, code-size, stack, heap, and DTB limits in TPL and SPL
- audit multi-DTB and FIT configuration selection against reliable board identity
- explain why FIT verification keys belong to the trusted control tree rather than the untrusted OS DTB
- trace environment, bootstd, extlinux, and scripted paths that choose a Linux DTB
- apply overlays to a writable working FDT with correct symbols, space, order, and failure handling
- distinguish FIT's boot payload packaging from binman's firmware-media layout
- prove exactly which DTB U-Boot used internally and which final DTB Linux received

## Prerequisites

Complete [Memory, Firmware, And Heterogeneous SoCs](memory-firmware-and-heterogeneous-socs.md). This module assumes you can inspect DT artifacts, reason about memory ownership, and distinguish a static hardware description from a runtime software protocol.

## Learning Path

1. [Control, Working, SPL, And Linux Device Trees](u-boot-and-bootloader-device-tree/control-working-spl-and-linux-device-trees.md)
2. [U-Boot DT Sources, Upstream Sync, And Build Artifacts](u-boot-and-bootloader-device-tree/u-boot-dt-sources-upstream-sync-and-build-artifacts.md)
3. [Driver Model, Boot Phases, And Pre-Relocation Properties](u-boot-and-bootloader-device-tree/driver-model-boot-phases-and-pre-relocation-properties.md)
4. [TPL, SPL, SRAM Budgets, And Multi-DTB Selection](u-boot-and-bootloader-device-tree/tpl-spl-sram-budgets-and-multi-dtb-selection.md)
5. [FIT Configurations, DTB Selection, And Verified Boot](u-boot-and-bootloader-device-tree/fit-configurations-dtb-selection-and-verified-boot.md)
6. [Environment, Bootstd, Extlinux, And OS DTB Loading](u-boot-and-bootloader-device-tree/environment-bootstd-extlinux-and-os-dtb-loading.md)
7. [Bootloader Overlay Application And Working-FDT Safety](u-boot-and-bootloader-device-tree/bootloader-overlay-application-and-working-fdt-safety.md)
8. [Binman, Firmware Packaging, And DT-Based Image Layout](u-boot-and-bootloader-device-tree/binman-firmware-packaging-and-dt-based-image-layout.md)
9. [Bootloader DT Selection And Handoff Lab](u-boot-and-bootloader-device-tree/bootloader-dt-selection-and-handoff-lab.md)

## Name The Artifact

Use explicit names in reviews and logs:

| Artifact | Primary consumer | Main purpose |
|---|---|---|
| U-Boot control FDT | U-Boot driver model/configuration | make U-Boot's own devices available |
| TPL/SPL FDT | early boot phase | minimal devices needed before the next phase |
| working FDT | boot commands and OS handoff | mutable tree prepared for the selected OS |
| Linux DTB | Linux | final hardware and boot handoff |
| FIT | U-Boot image loader/verifier | group payload images and configurations |
| binman description/FDT map | build tooling and optionally firmware | place firmware components in media images |

A FIT is encoded as a flattened tree, but its `/images` and `/configurations` nodes describe an image container, not board hardware. A binman node describes build-time layout. Neither should be interpreted as the Linux hardware tree.

## A Typical Provenance Chain

```text
shared SoC/board DTS + U-Boot-specific source fragments
  -> U-Boot control DTB
  -> filtered SPL/TPL DTB where required

Linux DTS build or supplied OS artifact
  -> one or more Linux DTBs
  -> FIT/extlinux/filesystem/network selection
  -> U-Boot working FDT
  -> overlays and handoff preparation
  -> final FDT address passed to Linux
```

Some platforms deliberately share or reuse an artifact, but do not assume identity. Prove it from the build and runtime path.

## Control Plane Versus Payload

Changing the control FDT can affect storage, console, network, verification keys, and U-Boot's ability to find the OS. Changing the working FDT affects the next-stage OS. The `fdt` command distinguishes the two; modifying the live control FDT after driver-model binding is dangerous because U-Boot retains references derived from it.

Treat control-DTB integrity as part of the bootloader trust boundary. An attacker who can replace verification keys or boot policy in the control tree may bypass otherwise correct FIT signatures.

## Completion Check

You are ready for [Boot-Time Mutation And Ownership](boot-time-mutation-and-ownership.md) when you can:

- draw the artifact and address flow from ROM through TPL, SPL, U-Boot, and Linux
- identify the source and build rule for every DTB in the shipped image
- prove why each node survives or is removed from an early-phase DTB
- calculate whether the complete SPL image and runtime allocations fit their memory windows
- reproduce FIT or bootflow DTB selection from immutable board identity and policy
- prove the selected kernel, DTB, overlays, and configuration are covered by the intended trust policy
- inspect control and working trees without confusing their mutations
- recover the exact final DTB handed to Linux and compare it with its sources

## Authoritative References

- [U-Boot Devicetree documentation](https://docs.u-boot.org/en/latest/develop/devicetree/index.html)
- [U-Boot Devicetree Control](https://docs.u-boot.org/en/latest/develop/devicetree/control.html)
- [U-Boot generic SPL framework](https://docs.u-boot.org/en/latest/develop/spl.html)
- [U-Boot `fdt` command](https://docs.u-boot.org/en/latest/usage/cmd/fdt.html)
- [U-Boot FIT documentation](https://docs.u-boot.org/en/latest/usage/fit/index.html)

## Related Topics

- [Memory, Firmware, And Heterogeneous SoCs](memory-firmware-and-heterogeneous-socs.md)
- [Boot-Time Mutation And Ownership](boot-time-mutation-and-ownership.md)
- [Device Tree In U-Boot](../build-systems/advanced/u-boot/device-tree-in-u-boot.md)
- [Security And Production Lifecycle](security-and-production-lifecycle.md)

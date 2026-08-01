---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# Boot-Time Mutation And Ownership

The DTB compiled in CI is often not the tree Linux receives. Firmware can reserve memory, a bootloader can select and relocate a base, overlays can add hardware, board code can inject identity, and the final boot path can update `/chosen`. Correct engineering treats every change as an owned, ordered, testable transformation.

## Learning Outcomes

After completing this module, you should be able to:

- model a boot-time DT flow as immutable inputs, ordered transforms, checkpoints, and a final handoff artifact
- assign one authority and one data source to every mutation
- distinguish hardware discovery from policy, compatibility selection, and ephemeral boot handoff
- use libfdt/U-Boot resizing and relocation safely without overwriting adjacent images
- reconcile detected RAM with reservations, secure exclusions, usable ranges, and Linux's final memory view
- construct `/chosen` data with correct cell widths, half-open initrd ranges, console semantics, and seed handling
- inject MAC addresses and product identity with defined precedence, validation, privacy, and persistence
- prove overlay order and reject partial or contradictory merged states
- coordinate mutations made by ROM, trusted firmware, SPL, U-Boot, and Linux without duplicate ownership
- compare built, selected, post-overlay, pre-kernel, and Linux-live trees semantically
- preserve enough provenance to reproduce a field unit's final DTB

## Prerequisites

Complete [U-Boot And Bootloader Device Tree](u-boot-and-bootloader-device-tree.md). You should be able to distinguish the control FDT from the working FDT and prove how a base Linux DTB and overlays were selected.

## Learning Path

1. [Mutation Provenance, Authorities, And Checkpoints](boot-time-mutation-and-ownership/mutation-provenance-authorities-and-checkpoints.md)
2. [Libfdt Capacity, Relocation, And Failure Atomicity](boot-time-mutation-and-ownership/libfdt-capacity-relocation-and-failure-atomicity.md)
3. [RAM Discovery, Reservations, And Memory Fixups](boot-time-mutation-and-ownership/ram-discovery-reservations-and-memory-fixups.md)
4. [`/chosen`, Boot Arguments, Initrd, Console, And Seeds](boot-time-mutation-and-ownership/chosen-bootargs-initrd-console-and-seeds.md)
5. [MAC Addresses, Serial Numbers, And Board Identity](boot-time-mutation-and-ownership/mac-addresses-serial-numbers-and-board-identity.md)
6. [Overlay Order, Composition, And Conflict Ownership](boot-time-mutation-and-ownership/overlay-order-composition-and-conflict-ownership.md)
7. [Firmware, Secure World, And Cross-Stage Ownership](boot-time-mutation-and-ownership/firmware-secure-world-and-cross-stage-ownership.md)
8. [Final-Tree Validation, Diffing, And Runtime Forensics](boot-time-mutation-and-ownership/final-tree-validation-diffing-and-runtime-forensics.md)
9. [Boot-Time Mutation Provenance Lab](boot-time-mutation-and-ownership/boot-time-mutation-provenance-lab.md)

## Treat The DTB As A Build Product With A Timeline

Use named checkpoints:

```text
C0 compiled base
  -> select
C1 selected/authenticated base
  -> trusted-firmware reservations
C2 firmware-adjusted tree
  -> ordered overlays
C3 composed hardware tree
  -> board identity and memory fixups
C4 board-adjusted tree
  -> boot command/initrd/seed handoff
C5 final FDT passed to Linux
  -> Linux unflattening and early consumption
C6 Linux live tree
```

Real platforms may order steps differently. Record the actual sequence because two correct transformations can produce different results when reversed.

## Mutation Record

For every change, capture:

| Field | Question |
|---|---|
| owner | Which stage/function is allowed to write it? |
| input | EEPROM, fuse, firmware call, image metadata, environment, probe result? |
| validation | How are range, format, compatibility, and trust checked? |
| operation | Add, replace, delete, reserve, or apply overlay? |
| target | Exact node/property and intended prior state |
| lifetime | Fixed hardware, product identity, or this-boot handoff? |
| failure | Abort, recover, omit optional data, or use a specified fallback? |
| evidence | Log event and before/after artifact hash |

“U-Boot fixes the tree” is not sufficient ownership.

## Mutation Is Not Discovery

DT still describes hardware. Boot-time mutation is appropriate when a fact is genuinely unavailable at build time or belongs to this boot: installed RAM size, authenticated product configuration, initrd location, boot arguments, randomness, or secure reservations established by firmware.

Do not mutate around:

- a missing binding
- incompatible boards shipped under one guessed compatible
- a Linux driver bug
- unreviewed policy hidden in board code
- contradictory ownership between firmware and OS

Prefer stable base descriptions and narrow, auditable transforms.

## Security Boundary

If a signed base DTB is changed after verification, the final tree is trusted only to the extent that mutation code and all inputs are trusted. Unauthenticated EEPROM, environment, overlay lists, or network metadata can subvert an otherwise verified boot.

Measure or log both ordered inputs and the final tree when provenance matters. Redact secrets; the DTB should carry public handoff data, not private keys.

## Completion Check

You are ready for [Binding Design And Stable ABI](binding-design-and-stable-abi.md) when you can:

- reproduce the final DTB from versioned inputs and an ordered mutation manifest
- identify every writer and reject duplicate ownership of a property
- prove that every growth/relocation operation fits a reserved memory interval
- reconcile `/memory`, `/reserved-memory`, the FDT reservation map, and Linux logs
- explain the exact source and precedence of bootargs, console, initrd, seeds, MACs, and serial identity
- validate every supported overlay composition after merging
- demonstrate that secure-world exclusions and normal-world access controls agree
- explain every semantic difference between the packaged base and Linux's live tree
- diagnose a failure without editing the compiled DTS until provenance identifies the responsible stage

## Authoritative References

- [U-Boot `fdt` command](https://docs.u-boot.org/en/latest/usage/cmd/fdt.html)
- [U-Boot Device Tree overlays](https://docs.u-boot.org/en/latest/usage/fdt_overlays.html)
- [Linux Devicetree usage model](https://docs.kernel.org/devicetree/usage-model.html)
- [Upstream `/chosen` schema](https://github.com/devicetree-org/dt-schema/blob/main/dtschema/schemas/chosen.yaml)
- [Linux Devicetree overlay notes](https://docs.kernel.org/devicetree/overlay-notes.html)

## Related Topics

- [U-Boot And Bootloader Device Tree](u-boot-and-bootloader-device-tree.md)
- [Memory, Firmware, And Heterogeneous SoCs](memory-firmware-and-heterogeneous-socs.md)
- [Overlays In Depth](overlays-in-depth.md)
- [Runtime Inspection](runtime-inspection.md)
- [Security And Production Lifecycle](security-and-production-lifecycle.md)

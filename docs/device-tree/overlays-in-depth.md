---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# Overlays In Depth

An overlay is a relocatable mutation of another Device Tree, not a self-contained hardware description. Its meaning depends on a compatible base, exported symbols or stable paths, prior overlays, phandle relocation, merge order, validation of the final tree, and—when applied to Linux's live tree—the lifetime of every device and pointer created from it.

## Learning Outcomes

After completing this module, you should be able to:

- write overlay source using `/plugin/`, shorthand references, explicit fragments, `target`, `target-path`, and `__overlay__`
- choose label targets or path targets by treating both as explicit compatibility interfaces
- explain how `dtc -@` produces `__symbols__`, `__fixups__`, and `__local_fixups__`
- distinguish external references to the base from local references within an overlay
- trace local phandle relocation and external fixup resolution before tree changes are applied
- reason about property replacement, node addition, source-deletion limits, changeset reversion, and fragment order
- define a versioned base/overlay contract that includes labels, paths, provider cells, bindings, and resource ownership
- model overlay dependencies, conflicts, stacking, application order, and legal removal order
- distinguish bootloader/pre-boot composition from Linux live-tree changesets and device lifecycle effects
- use the in-kernel overlay API and notifier phases without retaining pointers beyond their valid lifetime
- identify drivers, subsystems, userspace, DMA, IRQ, workqueue, and power-management state that can make runtime removal unsafe
- validate every supported merged tree rather than assuming an isolated DTBO is meaningful
- decide when separate base DTBs or normal DTS layering are safer than a growing overlay matrix
- preserve selection, authentication, ordering, and final-tree provenance in a product release

## Prerequisites

Complete [Writing And Validating Binding Schemas](writing-and-validating-binding-schemas.md). You should be able to validate final DTBs, decode phandles and provider specifiers, distinguish source labels from runtime properties, and reason about boot-time mutation ownership.

## Learning Path

1. [Overlay Source, Fragments, And Target Selection](overlays-in-depth/overlay-source-fragments-and-target-selection.md)
2. [Symbols, Fixups, Local Fixups, And Compilation](overlays-in-depth/symbols-fixups-local-fixups-and-compilation.md)
3. [Resolver, Phandle Relocation, And Merge Semantics](overlays-in-depth/resolver-phandle-relocation-and-merge-semantics.md)
4. [Base Compatibility, Versioning, And Overlay ABI](overlays-in-depth/base-compatibility-versioning-and-overlay-abi.md)
5. [Stacking, Dependencies, Conflicts, And Removal Order](overlays-in-depth/stacking-dependencies-conflicts-and-removal-order.md)
6. [Linux Runtime Overlays, Devices, Notifiers, And Lifetime](overlays-in-depth/linux-runtime-overlays-devices-notifiers-and-lifetime.md)
7. [Validation, Security, And Product Architecture](overlays-in-depth/validation-security-and-product-architecture.md)
8. [Overlay Composition And Lifecycle Lab](overlays-in-depth/overlay-composition-and-lifecycle-lab.md)

## Three Different Operations

Do not collapse these workflows into one:

| Workflow | Tree being changed | Device lifecycle consequence |
|---|---|---|
| host-side composition | serialized DTB file | none until the result is deployed |
| bootloader/firmware application | writable flat blob before OS handoff | Linux sees one static final tree |
| Linux runtime application | live unflattened tree | nodes, devices, drivers, sysfs, and subsystem state can appear or disappear |

The same DTBO can be structurally applicable in all three contexts while being safe in only one. Pre-boot composition avoids dynamic device removal but still needs capacity, order, authentication, and final-tree validation.

## Overlay Resolution Pipeline

```text
base DTB symbols and phandles
              +
DTBO fragments, local phandles, fixup metadata
              |
              v
relocate overlay-local phandles above base range
patch overlay references to relocated local phandles
resolve external symbol references against base/live tree
construct ordered changeset or mutate working FDT
apply changes
populate/depopulate devices when operating on Linux live tree
```

Resolution proves that references can be made numeric. It does not prove that the resulting hardware description is electrically valid, schema-valid, secure, or removable.

## Overlay Contract Manifest

For every independently distributed overlay, preserve:

```text
overlay stable ID and version
DTBO hash/signature
supported base IDs and version range
required exported labels or exact target paths
required prior overlays and ordering
conflicting overlays and exclusive resources
bindings/compatible contracts introduced
bootloader, firmware, and kernel constraints
apply-only or removable lifecycle classification
final-tree validation and hardware test evidence
```

The binary overlay format does not carry this complete product policy. A deployment manifest must.

## Removal Is A System Claim

Removing overlay nodes from the live tree is safe only when all consequences have been unwound:

```text
users blocked
  -> device unbound/depopulated
  -> new I/O stopped
  -> IRQs, DMA, timers, work, and callbacks drained
  -> subsystem registrations and links removed
  -> references to overlay nodes/properties released
  -> dependent overlays removed
  -> overlay changeset reverted
  -> overlay memory freed
```

The overlay core prevents removal beneath a directly stacked overlay, but it cannot prove every driver or subsystem has released every pointer or external resource. Treat a boot-only overlay as non-removable unless the entire stack provides a tested teardown contract.

## Completion Check

You are ready for [Build And Diagnostic Tools](build-and-diagnostic-tools.md) when you can:

- reconstruct an overlay's fragment targets and local/external reference graph from a DTBO dump
- explain exactly why the base needs symbols for a label-targeted overlay
- predict which cells `__fixups__` and `__local_fixups__` cause the resolver to patch
- derive the merged final tree for ordered overlays that touch the same property
- reject an overlay/base pairing before application using a declared compatibility contract
- compute dependency-safe apply and removal orders
- distinguish a resolver failure from a successful merge followed by probe or lifecycle failure
- explain notifier pointer lifetimes through `OF_OVERLAY_POST_REMOVE`
- prove runtime teardown drains every device-side effect before freeing overlay-owned data
- validate supported merged compositions and deliberately rejected combinations
- justify overlays versus separate DTBs using bounded product complexity and lifecycle needs

## Authoritative References

- [Linux Devicetree Overlay Notes](https://docs.kernel.org/devicetree/overlay-notes.html)
- [Linux Devicetree Dynamic Resolver Notes](https://docs.kernel.org/devicetree/dynamic-resolution-notes.html)
- [Linux Devicetree Changesets](https://docs.kernel.org/devicetree/changesets.html)
- [Linux Devicetree Kernel API](https://docs.kernel.org/devicetree/kernel-api.html)
- [U-Boot Device Tree Overlays](https://docs.u-boot.org/en/latest/usage/fdt_overlays.html)
- [Devicetree compiler project](https://git.kernel.org/pub/scm/utils/dtc/dtc.git/)

## Related Topics

- [Syntax, Values, And Source Composition](syntax-values-and-source-composition.md)
- [Boot-Time Mutation And Ownership](boot-time-mutation-and-ownership.md)
- [U-Boot And Bootloader Device Tree](u-boot-and-bootloader-device-tree.md)
- [Writing And Validating Binding Schemas](writing-and-validating-binding-schemas.md)
- [Security And Production Lifecycle](security-and-production-lifecycle.md)

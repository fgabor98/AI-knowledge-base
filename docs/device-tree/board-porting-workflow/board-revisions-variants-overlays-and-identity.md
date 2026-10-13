---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# Board Revisions, Variants, Overlays, And Identity

A port is incomplete if it works only when an engineer manually chooses the right DTB. The platform must map trustworthy physical identity to an explicit, compatible hardware description and fail safely when identity is missing, corrupt, or unknown.

## Define Variant Dimensions

List differences that can affect hardware description:

- PCB revision and assembly variant
- SoC/package/silicon revision
- module or carrier revision
- memory size/topology
- PMIC or fitted component alternatives
- boot storage
- optional daughtercard/expansion
- PHY/interface routing
- reserved-memory/remote-firmware layout
- safety/thermal design

Do not encode operational modes or commercial entitlements as board hardware variants.

## Choose A Representation

Pick separate DTS files, shared physical `.dtsi` layers, compatible fallback, overlays, bootloader fixups, or userspace configuration for each delta using the criteria in [Represent Variants Deliberately](../product-scale-maintenance-and-engineering/layering-variants-ownership-and-source-architecture.md#represent-variants-deliberately).

## Design The Identity Chain

```text
physical assembly
  -> immutable or authenticated identity source
  -> early reader/controller availability
  -> parser and validity checks
  -> allowlisted product/revision/options
  -> signed DTB/FIT configuration or overlay composition
  -> final root compatible/model and manifest identity
  -> runtime/reporting evidence
```

Possible sources include SoC fuses, board OTP, authenticated EEPROM, secure element, straps, or management firmware. Evaluate mutability, spoofability, corruption detection, availability timing, and replacement/service behavior.

## Never Default Silently To A Dangerous Variant

Define outcomes:

| Identity state | Policy example |
|---|---|
| valid known revision | select exact signed configuration |
| valid known base + optional card | select allowed ordered composition |
| unknown newer revision | authenticated recovery or conservative explicitly compatible base |
| checksum/authentication failure | fail closed to recovery |
| identity device unavailable | retry or recovery; do not guess from prior environment |
| conflicting sources | record conflict and stop normal boot |

A “closest” DTB can apply wrong voltage, memory, IOMMU, or thermal policy.

## Keep Board Identity Stable

Define:

- product namespace and vendor prefix
- root compatible ordering per binding
- human-readable model policy
- numeric/encoded revision format
- which fields are hardware identity versus release identity
- factory programming and read-back verification
- replacement-board/service behavior
- privacy treatment for unique serials

Do not use `model` string parsing as the primary kernel binding contract.

## Plan Board Revisions Before They Multiply

For each revision delta classify:

```text
no DT impact
source-only correction shared by all revisions
new board DTS composition
new device compatible/binding
bootloader/firmware fixup change
overlay/base compatibility change
release/update incompatibility
```

Update the hardware delta ledger, inventory, ownership, artifact list, and CI matrix together.

## Engineer Overlay Selection

An overlay selector needs a base/overlay contract, authenticated digests, allowed combinations and order, conflicts, lifecycle, and final-tree evidence ([Overlay Contract Manifest](../overlays-in-depth.md#overlay-contract-manifest)). Manual lab application is useful for diagnosis but is not a production selector.

## Test Every Declared Composition

```bash
fdtoverlay \
  -i ti/k3-am625-beagleplay.dtb \
  -o composed/k3-am625-beagleplay-csi2-ov5640.dtb \
  ti/k3-am625-beagleplay-csi2-ov5640.dtbo

dtc -I dtb -O dts \
  -o composed/k3-am625-beagleplay-csi2-ov5640.dts \
  composed/k3-am625-beagleplay-csi2-ov5640.dtb
```

Test negative pairs too. Successful resolver application proves structural references, not electrical or driver compatibility.

## Verify Selection At Three Checkpoints

### Pre-boot

- raw identity bytes and validity/authentication
- selector decision and reason
- signed configuration/base/overlay IDs

### Boot handoff

- final root compatible/model
- applied overlay list/order
- final FDT digest or measurement

### Linux runtime

- live root compatible/model
- expected variant-specific properties/devices
- release/manifest/identity correlation

Support bundles should contain these without leaking unnecessary unique or secret data.

## Prevent Variant Drift

Generate or validate artifacts from one reviewed [variant inventory](../product-scale-maintenance-and-engineering/layering-variants-ownership-and-source-architecture.md#create-a-variant-inventory) that maps identity and option bits to base DTB, overlays, root compatible, release set, and owners. CI should fail on orphan DTBs, unlisted overlays, duplicate identity mappings, or ambiguous selectors.

## Stage Exit Gate

```text
[ ] all shipping revisions/options are inventoried
[ ] each real delta has a deliberate representation
[ ] identity source and parser are trustworthy and tested
[ ] unknown/corrupt/conflicting identity has safe behavior
[ ] DTB/overlay selection is allowlisted and authenticated as required
[ ] every allowed and forbidden composition is tested
[ ] final handoff/live identity correlates with release manifest
[ ] source layers and owners match physical assemblies
[ ] service/replacement/recovery cases are documented
```

## Further Reading

- [Overlays In Depth](../overlays-in-depth.md)
- [Boot-Time Mutation And Ownership](../boot-time-mutation-and-ownership.md)
- [Security And Production Lifecycle](../security-and-production-lifecycle.md)
- [Layering, Variants, Ownership, And Source Architecture](../product-scale-maintenance-and-engineering/layering-variants-ownership-and-source-architecture.md)
- [Validation, Upstreaming, And Production Handoff](validation-upstreaming-and-production-handoff.md)

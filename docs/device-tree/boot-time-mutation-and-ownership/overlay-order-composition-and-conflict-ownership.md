---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# Overlay Order, Composition, And Conflict Ownership

An overlay is a mutation package, not an independent hardware description. Its meaning depends on the base symbols, prior overlays, application order, and product conflict policy. Boot-time composition must be deterministic and validated as a complete final tree.

## Define A Layer Model

A useful product hierarchy is:

```text
SoC and baseboard
  -> board revision
  -> factory-fitted option
  -> replaceable boot-time module
  -> deployment policy
```

Every layer should own a bounded set of paths/properties. If two layers can write the same property, define an explicit rule or make the combination invalid.

Do not use overlay order as an undocumented way to “win” conflicting assignments.

## Selection Inputs

Overlay selection may depend on:

- authenticated product manifest
- EEPROM/OTP module identity
- straps
- boot configuration
- FIT configuration
- environment or operator choice

For each input, define authenticity, supported versions, and fallback. Directory enumeration, filesystem order, and user-supplied comma-separated lists are not product policy unless normalized and authorized.

## Dependency And Conflict Graph

Model `requires`, `before`, `conflicts`, and base support as an explicit graph and compute one canonical ordered list, rejecting missing requirements, cycles, duplicates, mutually exclusive modules, unsupported bases, and multiple owners for one exclusive resource ([Model Four Relations](../overlays-in-depth/stacking-dependencies-conflicts-and-removal-order.md#model-four-relations), [Canonical Application Order](../overlays-in-depth/stacking-dependencies-conflicts-and-removal-order.md#canonical-application-order)). Store stable overlay IDs and hashes, not only filenames that can be renamed. Libfdt merges structure but cannot judge electrical validity, so add the [hardware conflict checks](../overlays-in-depth/stacking-dependencies-conflicts-and-removal-order.md#hardware-conflict-inventory) on top of schema validation.

## Operations On The Same Property

When two overlays set the same property (for example `status`), the later one wins. If that is intentional, the manifest must name the owning layer; otherwise reject the combination ([Property Collision](../overlays-in-depth/stacking-dependencies-conflicts-and-removal-order.md#property-collision)). Prefer complete, positive hardware descriptions over chains of delete/re-add operations.

## Atomic Application

Authenticate and resolve the complete input set before applying anything, then apply to a disposable working copy and publish it only on success, as in the [safe U-Boot pattern](../u-boot-and-bootloader-device-tree/bootloader-overlay-application-and-working-fdt-safety.md#failure-is-not-transactional-recovery). Application errors can invalidate base and overlay blobs, so preserve pristine copies or reload capability.

## Checkpoints And Hashes

In development, save after each overlay to find the first unexpected change. In production, at least record:

- base ID/hash
- canonical ordered overlay ID/hash list
- manifest/policy version
- final packed-tree hash
- application return status

Because later `/chosen` seeds make the final blob intentionally nondeterministic, also compute a hardware-composition hash before ephemeral handoff data or use a redacted canonical form.

## Overlay ABI And Combination Testing

An independently shipped overlay depends on base labels, paths, and property contracts; a base refactor that preserves runtime semantics can still break label-based overlays ([Base Compatibility, Versioning, And Overlay ABI](../overlays-in-depth/base-compatibility-versioning-and-overlay-abi.md)). Reduce combinations with the declared dependency/conflict model, then merge and validate every supported combination and every intentionally rejected one ([Validation, Security, And Product Architecture](../overlays-in-depth/validation-security-and-product-architecture.md)).

## Runtime Overlay Boundary

Boot-time overlays produce a static tree before Linux unflattens it. Linux runtime overlays have additional device-population, driver binding, removal, and lifetime concerns. Do not infer runtime removal safety from successful U-Boot composition.

Use the later [Overlays In Depth](../overlays-in-depth.md) module for dynamic lifecycle treatment.

## Authoritative References

- [U-Boot Device Tree overlays](https://docs.u-boot.org/en/latest/usage/fdt_overlays.html)
- [U-Boot FIT overlay usage](https://docs.u-boot.org/en/latest/usage/fit/overlay-fdt-boot.html)
- [Linux Devicetree overlay notes](https://docs.kernel.org/devicetree/overlay-notes.html)
- [Devicetree dynamic resolver notes](https://docs.kernel.org/devicetree/dynamic-resolution-notes.html)
- [Devicetree compiler project](https://git.kernel.org/pub/scm/utils/dtc/dtc.git/)

## Continue

Proceed to [Firmware, Secure World, And Cross-Stage Ownership](firmware-secure-world-and-cross-stage-ownership.md).

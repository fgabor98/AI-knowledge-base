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

Model:

```text
requires(A, B)
before(B, C)
conflicts(C, D)
supports(overlay, base-family, revision-range)
```

Then compute one canonical ordered list. Reject:

- missing requirements
- cycles
- duplicate overlays
- mutually exclusive modules
- unsupported base versions
- multiple owners for one exclusive resource

Store stable overlay IDs and hashes, not only filenames that can be renamed.

## Structural Versus Hardware Conflicts

Libfdt can merge phandles and properties but cannot decide whether the result is electrically valid. Add semantic checks for:

- GPIO/pin ownership
- chip-select/address duplication
- regulator voltage/current compatibility
- clocks and exclusive rates
- reset ownership
- overlapping `reg` or reserved-memory ranges
- DMA/IOMMU stream IDs
- connector/lane routing
- aliases and default console

Schema validation catches many shapes, but product-resource conflict checks remain necessary.

## Operations On The Same Property

Overlays can replace a property by setting it again. The final value depends on order:

```text
revision overlay: status = "okay"
policy overlay:   status = "disabled"
```

If this is intentional, the policy layer owns final `status` and the manifest must express that. If not, reject the combination.

Deletion requires even more care: a later overlay that references a deleted node can fail or bind elsewhere after restructuring. Prefer complete, positive hardware descriptions over chains of delete/re-add operations.

## Atomic Application

Validate all inputs before applying any:

1. authenticate base, manifest, and overlays
2. resolve compatibility/dependency/order
3. calculate maximum capacity
4. copy the base to a disposable working buffer
5. apply in canonical order, checking every result
6. validate expected postconditions and final schema
7. publish the new working tree only on success

U-Boot documents that application errors can invalidate base and overlay blobs. Preserve pristine copies or reload capability.

## Checkpoints And Hashes

In development, save after each overlay to find the first unexpected change. In production, at least record:

- base ID/hash
- canonical ordered overlay ID/hash list
- manifest/policy version
- final packed-tree hash
- application return status

Because later `/chosen` seeds make the final blob intentionally nondeterministic, also compute a hardware-composition hash before ephemeral handoff data or use a redacted canonical form.

## Overlay ABI

An independently shipped overlay depends on base targets, labels/symbols, and property contracts. Treat these as a compatibility ABI:

- version supported base families
- retain required labels or target stable paths carefully
- test overlay against every supported base release
- update base and overlay atomically where possible
- do not promise indefinite compatibility accidentally

A base DTS refactor that preserves runtime hardware semantics can still break label-based overlays.

## CI Combination Testing

Exhaustive combinations grow exponentially. Reduce them with the declared dependency/conflict model, then test every supported combination:

```sh
fdtoverlay -i base.dtb -o merged.dtb rev4.dtbo module-x.dtbo
dtc -I dtb -O dts -o merged.dts merged.dtb
```

Run:

- schema validation
- resource-conflict checks
- required-node assertions
- semantic diff against approved baseline
- kernel/U-Boot smoke boot where representative

Also test every rejected combination and verify a deterministic safe failure.

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

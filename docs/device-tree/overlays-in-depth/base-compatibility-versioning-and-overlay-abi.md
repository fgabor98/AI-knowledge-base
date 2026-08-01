---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# Base Compatibility, Versioning, And Overlay ABI

An overlay compiled successfully only proves that its own source and unresolved references were representable. Compatibility exists between a particular overlay contract and a particular base contract. Labels are only the first layer.

## What The Overlay Depends On

Inventory every dependency:

| Dependency | Example | Breakage mode |
|---|---|---|
| exported symbol | `spi2`, `gpio1` | missing or renamed label |
| target path | `/soc/spi@2000000` | hierarchy/name/address changed |
| target binding | SPI controller child rules | child no longer valid |
| provider contract | `#gpio-cells = <2>` | specifier decoded differently |
| prior node state | bus disabled, CS0 unused | overwrite or duplicate resource |
| address/cell context | child `reg = <0>` | different bus encoding |
| electrical resource | pins, voltage, clock | structurally valid hardware conflict |
| schema/compatible | module device binding | final tree fails validation |
| lifecycle support | removable driver stack | apply works, teardown unsafe |

“All labels resolved” addresses only the first two rows.

## Labels Are Exported Interfaces

A stable base label promises:

- the label remains available in deployed `__symbols__`
- it continues to identify the same hardware role
- the target's binding and provider semantics remain compatible
- overlays may legally add or update the documented surface

Do not preserve a label by moving it to a different “close enough” node. That turns a clear resolution failure into a silent semantic misapplication.

Maintain an intentional exported-label allowlist for independently released overlays. Internal labels may change with source refactoring; public overlay targets need review.

## Paths Are Interfaces Too

`target-path` avoids label lookup, not compatibility. It couples the overlay to:

- every ancestor node name
- unit addresses
- bus hierarchy
- enabled/disabled target identity
- any path aliases stored in other metadata

A base refactor that nests a controller under a new bus breaks the path even if the hardware and compatible are unchanged. Use path targets only when the exact hierarchy is a managed contract.

## Define A Public Overlay Surface

Instead of letting overlays target arbitrary internals, publish a bounded surface:

```text
base family: acme,falcon
exported targets:
  expansion_spi -> SPI controller, CS0/CS1 available
  expansion_i2c -> I2C controller, addresses 0x20-0x6f subject to manifest
  expansion_gpio -> provider contract, allowed line set
  expansion_connector -> graph endpoint and lane mapping
forbidden:
  PMIC internals, secure peripherals, reserved-memory ownership, boot console
```

The surface should describe allowed operations and resources, not only label names.

## Compatibility Identifiers

The overlay format has no universal built-in product compatibility negotiation. A product manifest can add:

```text
base_abi: falcon-expansion-v3
overlay_id: acme,temp-module
overlay_version: 2
supports_base_abi: [falcon-expansion-v2, falcon-expansion-v3]
requires: [spi2-export-v1, gpio-export-v2]
conflicts: [display-module]
lifecycle: boot-only
```

Keep this metadata authenticated and bound to the DTBO hash. A filename containing `v3` is not enforceable compatibility.

The identifier must map to concrete tests. Increment or split it when an exported label, provider contract, resource reservation, or allowed mutation changes incompatibly.

## Compatible Strings Do Not Version Overlay Mechanics

An overlay may introduce a device with `compatible = "acme,temp100"`; that string defines the device binding. It does not state which Falcon base labels, pins, or overlay order are supported.

Likewise, adding `compatible` to the overlay root is not automatically interpreted by generic overlay resolvers as a base compatibility gate. Product tooling must define and enforce its manifest semantics before application.

## Base Evolution Cases

| Base change | Overlay impact |
|---|---|
| source files reorganized; final symbols and nodes identical | normally compatible |
| target node path moved; public label preserved and regenerated | label target can remain compatible; path target breaks |
| public label renamed | label overlays break |
| GPIO provider changes `#gpio-cells` or flags | old specifiers become invalid/unsafe |
| target bus `#address-cells` changes | child `reg` encoding may break |
| pinctrl state renamed but overlay references old label | fixup failure |
| CS/address now occupied in base | merge can succeed but hardware conflicts |
| base adds same child node | merge/duplicate semantics must be reviewed |
| target binding gains a new mandatory resource | old overlays may create incomplete final nodes |

Use semantic final-tree diffs and hardware-resource checks; a symbol-table diff alone is insufficient.

## Overlay Evolution Cases

When updating an overlay:

- retain its stable ID and increment version for backward-compatible improvements
- create a new incompatible ID/version range when base requirements change
- do not silently retarget a label to different hardware
- preserve bindings for already shipped compatibles
- state whether later overlays can depend on its exported labels
- test old base/new overlay and new base/old overlay combinations that the updater can produce
- include bootloader/kernel constraints where parsers differ

The DTBO binary itself should be immutable under a release hash.

## Four-Way Compatibility Matrix

| Base | overlay | Required result |
|---|---|---|
| old | old | released baseline |
| old | new | supported merge, explicit rejection, or unreachable update order |
| new | old | preserved overlay surface or explicit rejection |
| new | new | intended complete behavior |

Extend the matrix for prior overlays, bootloader versions, kernels, and module firmware. A base/overlay pair can merge identically yet behave differently under a kernel lacking the introduced binding or teardown support.

## Preconditions And Postconditions

For each fragment:

```text
precondition:
  target symbol/path resolves to expected compatible
  expected property has approved prior value
  exclusive resources are unclaimed
  provider cell contracts match

postcondition:
  exact child/property exists
  resulting node passes binding schema
  resource graph has no conflicts
  required driver/lifecycle capability exists
```

Generic resolver code rarely checks all of these. The product selection/validation layer must.

## Release Evidence

Preserve:

- base DTB and overlay DTBO hashes
- decoded/exported symbol inventory
- compatibility manifest and signature
- exact ordered overlay list
- merged DTB hash before ephemeral boot fixups
- schema and resource-conflict results
- cross-version matrix results
- hardware and lifecycle tests
- rejection tests for unsupported combinations

## Authoritative References

- [Linux Devicetree Overlay Notes](https://docs.kernel.org/devicetree/overlay-notes.html)
- [Linux Devicetree Dynamic Resolver Notes](https://docs.kernel.org/devicetree/dynamic-resolution-notes.html)
- [U-Boot Device Tree Overlays](https://docs.u-boot.org/en/latest/usage/fdt_overlays.html)
- [Devicetree ABI documentation](https://docs.kernel.org/devicetree/bindings/ABI.html)

## Continue

Proceed to [Stacking, Dependencies, Conflicts, And Removal Order](stacking-dependencies-conflicts-and-removal-order.md).

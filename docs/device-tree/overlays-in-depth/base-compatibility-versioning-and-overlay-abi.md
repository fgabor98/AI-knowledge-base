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
| exported symbol | `mcasp1`, `sii9022`, `codec_audio` | missing or renamed label |
| target path | `/` for the new sound card | hierarchy/name/address changed |
| target binding | `simple-audio-card` rules | card no longer valid |
| provider contract | `#sound-dai-cells = <0>` on McASP and SiI9022 | specifier decoded differently |
| prior node state | analog card enabled and using `mcasp1` | two cards claim one McASP |
| address/cell context | `reg` of the PCIe EP node under `cbass_main` | different bus encoding |
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

TI's `k3-am62x-sk-hdmi-audio.dtso` shows both the need and the risk. The kernel `Makefile` composes the same overlay with three bases:

```make
k3-am625-sk-hdmi-audio-dtbs := k3-am625-sk.dtb k3-am62x-sk-hdmi-audio.dtbo
k3-am62-lp-sk-hdmi-audio-dtbs := k3-am62-lp-sk.dtb k3-am62x-sk-hdmi-audio.dtbo
k3-am62a7-sk-hdmi-audio-dtbs := k3-am62a7-sk.dtb k3-am62x-sk-hdmi-audio.dtbo
```

So `mcasp1`, `sii9022`, and `codec_audio` form an unwritten interface. The AM625 and AM62 LP SKs get them from `k3-am62x-sk-common.dtsi`, but the AM62A7 SK defines its own `codec_audio`, `sii9022`, and `&mcasp1` in `k3-am62a7-sk.dts`. Nothing ties the two files together except convention: renaming a label, or wiring a different McASP to the bridge, in either file would break one composition while the other still builds. A written surface would state:

```text
bases: k3-am625-sk, k3-am62-lp-sk, k3-am62a7-sk
exported targets:
  mcasp1      -> McASP instance wired to the SiI9022 I2S input, #sound-dai-cells = <0>
  sii9022     -> SiI9022 HDMI bridge, #sound-dai-cells = <0>
  codec_audio -> the analog sound card that shares mcasp1; may be disabled
forbidden:
  pinmux, clocks, and supplies of either audio path
```

The surface should describe allowed operations and resources, not only label names.

## Compatibility Identifiers

The overlay format has no universal built-in product compatibility negotiation. A product manifest can add:

```text
base_abi: am62x-sk-audio-v1
overlay_id: ti,am62x-sk-hdmi-audio
overlay_version: 1
supports_base_abi: [am62x-sk-audio-v1]
requires: [mcasp1, sii9022, codec_audio]
conflicts: []
lifecycle: boot-only
```

This manifest is illustrative: mainline has no such file, which is exactly the gap it would close.

Keep this metadata authenticated and bound to the DTBO hash. A filename containing `v3` is not enforceable compatibility.

The identifier must map to concrete tests. Increment or split it when an exported label, provider contract, resource reservation, or allowed mutation changes incompatibly.

## Compatible Strings Do Not Version Overlay Mechanics

The HDMI audio overlay introduces a node with `compatible = "simple-audio-card"`; that string defines the card binding. It does not state which bases export the right labels, or that the analog card must be disabled first.

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

Apply the same old/new × old/new matrix used for [compatible evolution](../driver-matching/compatible-evolution-and-stable-abi-decisions.md#compatibility-in-both-directions) to base and overlay versions: each cell must yield intended behavior, a supported merge, or an explicit rejection. Extend it for prior overlays, bootloader versions, kernels, and module firmware; a pair can merge identically yet behave differently under a kernel lacking the introduced binding or teardown support.

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

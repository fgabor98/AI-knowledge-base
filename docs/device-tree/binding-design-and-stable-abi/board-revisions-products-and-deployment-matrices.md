---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# Board Revisions, Products, And Deployment Matrices

Product revisions turn binding theory into release engineering. The goal is to represent physical differences accurately while keeping source reuse high and ensuring that every bootable combination of base DTB, overlays, firmware, and software is intentional.

## Begin With A Physical Delta Table

For each revision, compare facts rather than file names:

| Area | revision A | revision B | DT consequence |
|---|---|---|---|
| Ethernet PHY | model X at address 1 | model Y at address 3 | PHY node identity/address changes |
| reset polarity | active low | active high | GPIO flag changes |
| regulator | fixed 3.3 V | controllable PMIC rail | supply provider/topology changes |
| storage | 4-bit SD | 8-bit eMMC | bus width, child/card properties, pinctrl |
| optional radio | absent | factory option | bounded variant or overlay decision |

If a revision changes only software packaging, there may be no DT change. If it changes wiring expressed by standard properties, change those properties; do not invent a revision boolean in every affected driver.

## Choose The Right Identity Level

Possible identity levels include:

- SoC/IP compatible for a silicon programming contract
- root compatible for a board or product software-visible identity
- peripheral compatible for the actual component
- module/carrier compatible when a separately composed hardware unit has a defined description

A root revision-specific compatible is useful when early firmware, platform policy, inventory, or non-discoverable board behavior needs to distinguish revisions. It does not require changing every unchanged peripheral compatible.

Avoid `board-revision = <...>` as a universal switch read by unrelated drivers. Describe each physical delta at its owning node. Central revision identity may still exist for product selection or diagnostics, but must not replace the hardware description.

## Base Trees, Overlays, And Source Layers

Use a separate base DTS for a coherent fixed variant with substantial topology changes, and overlays only for genuinely composable hardware with controlled interfaces and combinations ([Overlays Versus Separate Base DTBs](../overlays-in-depth/validation-security-and-product-architecture.md#overlays-versus-separate-base-dtbs)). Layer sources by physical ownership ([Hardware-Based Layering And Source Style](../syntax-values-and-source-composition/hardware-based-layering-and-source-style.md)); inclusion is source composition, not ABI inheritance, and the final flattened tree must describe exactly one physical assembly without contradictory leftovers.

## Status Is Not Variant Modeling By Itself

Shared `.dtsi` files may define disabled hardware blocks and board DTS files enable populated instances. But `status = "disabled"` cannot repair wrong resources inherited from an unrelated board. Override every changed physical fact or choose a cleaner source boundary.

Review the flattened DTB, because source layering can hide stale supplies, pin states, child nodes, or aliases.

## Define Supported Compositions

Declare per base which overlays are allowed, which combinations are forbidden, and which firmware is required (for example `product-reva.dtb: allowed radio-v1, display-a; forbidden radio-v1 + display-a (shared pins)`). “Any overlay applies to any base” is not a versioning strategy; release bases and overlays as a tested set and reject unsupported combinations before partial application.

## Expand The Deployment Matrix

For a product, the compatibility matrix may be:

| Artifact | Parses or mutates DT? | Independently updated? | Rollback? |
|---|---:|---:|---:|
| ROM/SPL | selects base | rarely | fixed/limited |
| trusted firmware | reserves resources | yes | controlled |
| U-Boot | selects/applies overlays, fixups | yes | yes |
| base DTB | primary hardware description | yes | yes |
| DTBO | adds module description | yes | yes |
| Linux kernel/modules | consumes final tree | yes | yes |
| remote firmware | shares memory/resources | yes | yes |

Test every reachable combination or enforce metadata that makes unsafe combinations unreachable.

## Revision Discovery And Trust

Board identity can come from straps, EEPROM, fuses, a system controller, or the selected image. Its authority, validation, mapping to an exact composition, and handling of unknown or contradictory values are covered in [Design The Identity Chain](../board-porting-workflow/board-revisions-variants-overlays-and-identity.md#design-the-identity-chain). An unauthenticated EEPROM must not select a tree that grants access to secure memory or unsafe voltages.

## Minimize Deltas Without Hiding Them

Source reuse is good when it follows common hardware. It is harmful when reviewers must simulate a long include chain to discover the final board.

For each revision:

- keep a human-readable physical delta list
- compile and inspect the final tree
- diff normalized final DTBs semantically
- explain every difference, including deletions and inherited properties
- test aliases, `/chosen`, reserved memory, and overlay targets

## Release Evidence

Preserve:

```text
board assembly/revision identifier
base DTB and overlay hashes
ordered composition manifest
compatible lists
firmware and bootloader constraints
kernel compatibility range
validation results
hardware test coverage
rollback and recovery result
```

This connects ABI reasoning to a reproducible field unit.

## Authoritative References

- [Linux DTS coding style](https://docs.kernel.org/devicetree/bindings/dts-coding-style.html)
- [Linux Devicetree overlay notes](https://docs.kernel.org/devicetree/overlay-notes.html)
- [Linux submitting Devicetree patches](https://docs.kernel.org/devicetree/bindings/submitting-patches.html)
- [Devicetree Specification](https://devicetree-specification.readthedocs.io/en/stable/)

## Continue

Proceed to [Review Strategy And Upstream Submission Order](review-strategy-and-upstream-submission-order.md).

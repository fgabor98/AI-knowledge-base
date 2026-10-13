---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# Layering, Variants, Ownership, And Source Architecture

Source reuse is safe only when it follows physical reuse. Put a fact in the highest layer where it is true for every inheritor and the lowest layer that owns it. A convenient shared include that describes non-common hardware creates distant, surprising regressions.

## Model The Physical Hierarchy

Map source files to physical units as in [Hardware-Based Layering And Source Style](../syntax-values-and-source-composition/hardware-based-layering-and-source-style.md), extended at product scale with package-specific pin layers, assembled-product root identity, and independently selected option overlays:

TI's AM62x starter-kit family in mainline Linux has this graph:

```text
k3-am62.dtsi, k3-am62-main/mcu/wakeup.dtsi   on-SoC blocks, addresses, interrupts
  <- k3-am625.dtsi                            AM625 variant: four A53 cores, OPP table
k3-am62x-sk-common.dtsi                       shared starter-kit wiring
  <- k3-am625-sk-common.dtsi                  AM625 SK and SiP SK only (1.4 GHz OPP, OSPI, second PHY)
       <- k3-am625-sk.dts                     root identity, eMMC pins
       <- k3-am6254atl-sk.dts                 SiP variant
  <- k3-am62-lp-sk.dts                        LP SK: TPS65219 PMIC, eMMC pins, root identity
k3-am62x-sk-hdmi-audio.dtso                   optional, composed with three bases
```

Not every family needs every layer. Each include must correspond to a stable physical unit or deliberately documented contract.

Not every family needs every layer. Each include must correspond to a stable physical unit or deliberately documented contract.

## Apply The Truth Test

Before moving content into a common `.dtsi`, ask:

1. Is this hardware present and wired identically on every inheritor?
2. Do provider references and electrical constraints remain identical?
3. Is its lifecycle owned with the shared assembly?
4. Would a new inheritor reasonably expect this node?
5. Can a reviewer enumerate the blast radius from the include graph?

If any answer is no, keep it lower or split the common layer. TI's family failed question 1 for three years. The eMMC pin group lived in `k3-am62x-sk-common.dtsi` from v5.19 with plain `PIN_INPUT` on all pins, and its comments even listed two packages' balls, such as `(Y3/V3) MMC0_CMD`. The AM625 SK needs internal pull-ups on DAT1–DAT7, as JESD84 recommends to keep unconnected lines from floating, while the LP SK has external pull-ups. Commit `a0b8da04153e` (v6.17, `Cc: stable`) moved the group into each board file, giving the SK `PIN_INPUT_PULLUP` and leaving the LP SK unchanged.

## Separate Description From Policy

DT describes hardware and stable integration contracts, not a product-feature database. Avoid properties such as:

```dts
/* Hypothetical, and none of these exist in TI's tree. */
ti,product-sku = <17>;
ti,enable-premium-mode;
ti,use-new-driver;
```

Use a specific compatible for real hardware programming differences, describe fitted devices, and keep commercial entitlement or driver-choice policy outside DT.

## Represent Variants Deliberately

### Separate board DTS

Use when assemblies differ in wiring, providers, addresses, fitted devices, memory, or safety behavior. Reuse common physical layers and keep explicit final compositions.

### Common DTS plus compatible fallback

Use when a newer hardware revision is backward-compatible and the binding defines the fallback relationship.

### Overlay

Use for independently selectable, well-bounded optional hardware when selection, authentication, ordering, compatibility, and test coverage are engineered.

### Bootloader fixup

Use for trustworthy runtime facts such as discovered memory or device identity when static artifacts cannot know them. Constrain, validate, and record the mutation.

### Userspace configuration

Use for operational policy that is not a hardware description.

## Avoid Preprocessor Variant Mazes

Conditional compilation can hide which tree exists:

```dts
#ifdef PRODUCT_REV_C
    status = "okay";
#else
    status = "disabled";
#endif
```

Prefer named build targets whose composition is inspectable. If generation is unavoidable, commit or publish the normalized input model, deterministic generator version, generated artifact inventory, and reviewable semantic diffs.

## Design Stable Labels And Includes

Labels are source-level handles and overlay dependencies, not runtime ABI by default. Still, changing widely consumed labels can break downstream includes and overlays. Maintain an explicit source/overlay ABI policy:

- public labels and symbols
- private/internal labels
- allowed overlay targets
- compatible base versions
- deprecation window
- validation of every declared overlay composition

Avoid reaching across layers to override internal child nodes when a stable higher-level composition point is available.

## Define Ownership By Contract

Example responsibility map:

| Area | Accountable owner | Required reviewers | Evidence owner |
|---|---|---|---|
| SoC DTSI and bindings | silicon/platform team | subsystem + DT maintainers | upstream CI owner |
| module DTSI | module team | power, memory, platform | module validation owner |
| carrier DTSI | carrier hardware team | signal/power/product | carrier lab owner |
| product DTS | product platform team | module + carrier owners | release qualification |
| boot fixups | boot firmware team | security + kernel platform | boot integration CI |
| overlays | option owner | base owner + security | composition matrix owner |
| release manifest | release engineering | all artifact owners | release approver |

One person or team must be accountable for the final composition. Shared responsibility without a decision owner is a gap.

## Create A Variant Inventory

```yaml
product: am62x-lp-sk-based
board_name: AM62-LP-SKEVM            # TI board EEPROM
soc: AM625                           # plus speed and temperature grade from eFuse
pmic: TPS65219 (VDD_CORE 0.75 V)
memory: 2GiB LPDDR4
fitted_options: [hdmi-audio]
base_dtb: ti/k3-am62-lp-sk.dtb
allowed_overlays: [ti/k3-am62x-sk-hdmi-audio.dtbo]
forbidden_overlays: [ti/k3-am62x-sk-csi2-imx219.dtbo]   # if no camera is fitted
uboot_defconfig: am62x_lpsk_a53_defconfig
identity_source: board-eeprom (CRC only)
owners:
  final_composition: product-platform
  carrier: carrier-hw
qualification_class: am62x-lp-2g-T
```

Generate the expected artifact set and test matrix from a reviewed inventory rather than discovering variants from filenames.

## Analyze Change Blast Radius

For a modified file, compute or document:

```text
direct include consumers
transitive board/product consumers
overlays targeting changed labels/nodes
bindings selecting affected compatibles
bootloader fixups addressing affected paths
supported release branches carrying the file
hardware equivalence classes requiring retest
```

A one-line shared DTSI change may require more validation than a large new board-only file.

## Refactor Safely

When extracting common content:

1. Capture exact built DTBs for all existing products.
2. Move only identical hardware facts in a mechanical patch.
3. Rebuild and compare decoded semantics for every consumer.
4. Keep behavior changes in later patches.
5. Verify overlay symbols/fixups if labels move.
6. Boot representatives when serialization or composition changes.

Do not mix cleanup, source movement, property correction, and new variant support in one unreviewable diff.

## Architecture Smells

- board-specific regulator in SoC `.dtsi`
- base include that enables every optional peripheral
- many boards overriding the same incorrect shared property
- include file named `common.dtsi` without a physical ownership definition
- overlay targeting deep private labels
- board selection based on mutable filename strings
- duplicated whole DTS files differing by two properties
- generated DTS with no reviewable input model
- final product DTS owned by nobody because every layer has an owner

## Further Reading

- [Linux DTS coding style: organizing DTSI and DTS](https://docs.kernel.org/devicetree/bindings/dts-coding-style.html)
- [Overlays In Depth](../overlays-in-depth.md)
- [Multidimensional Review And Change Design](multidimensional-review-and-change-design.md)

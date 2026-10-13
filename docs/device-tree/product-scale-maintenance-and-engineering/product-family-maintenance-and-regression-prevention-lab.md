---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# Product Family Maintenance And Regression Prevention Lab

This lab studies a real product family over three years of upstream history: TI's AM62x starter kits in mainline Linux. Four real commits each changed several boards at once, and one fixed a board-specific fact that had sat in a shared file since v5.19. You will rebuild the family model, review each change across dimensions, plan a downstream branch that carries them, and design CI that would have caught the failures earlier.

## The Family

| Board | Top-level file | Shared layers (v6.19) | Notable differences |
|---|---|---|---|
| AM625 SK | `k3-am625-sk.dts` | `k3-am625-sk-common.dtsi` → `k3-am62x-sk-common.dtsi` | 1.4 GHz OPP, OSPI NOR, two Ethernet PHYs, eMMC needs internal pull-ups |
| AM6254atl SiP SK | `k3-am6254atl-sk.dts` | same as AM625 SK | memory node in the SiP's SoC file |
| AM62x LP SK | `k3-am62-lp-sk.dts` | `k3-am62x-sk-common.dtsi` | TPS65219 PMIC (VDD_CORE 0.75 V), external eMMC pull-ups, MCU CAN wake pins |
| AM62A7 SK (related) | `k3-am62a7-sk.dts` | none of the above | redefines the labels the HDMI audio overlay needs |

## The Four Events

```text
73387da70f9c  v6.6-rc1   k3-am62x-sk-common: main_i2c1 400 kHz -> 100 kHz
                          (SiI9022 DDC transactions time out above 100 kHz)
69c570ebc396  v6.7-rc1   Makefile: compose k3-am62x-sk-hdmi-audio.dtbo with
                          AM625 SK and AM62-LP SK instead of leaving it "floating"
a0b8da04153e  v6.17-rc1  move eMMC pinmux from k3-am62x-sk-common.dtsi to each board;
                          AM625 SK gets PIN_INPUT_PULLUP on DAT1-7 (Fixes: d19a66ae488a, v5.19; Cc: stable)
2517e476b819  v6.18-rc1  new k3-am625-sk-common.dtsi shared by AM625 SK and SiP SK;
                          most of k3-am625-sk.dts moves into it
1446fc4dc072  v6.19-rc1  rgmii-rxid -> rgmii-id for all CPSW ports, 33 files
```

## Inputs

- a mainline Linux clone with history, and `git log`, `git show`, `git describe --contains`
- the ability to build DTBs for at least two tags (manual `cpp` + `dtc` is enough)
- `fdtget`, `dtc`, and optionally `dt-validate`

## Part 1: Build The Source Model

1. Draw the include graph for each board at v6.6 and at v6.19. Use `git show <tag>:arch/arm64/boot/dts/ti/<file> | grep '#include'`.
2. For `k3-am62x-sk-common.dtsi`, list every board in its blast radius at each tag.
3. List which nodes in the shared files are true for every inheritor, and mark the ones you would challenge. The eMMC pin group should be on your list; look for other per-board electrical facts.

## Part 2: Review Each Event Across Dimensions

For each commit, fill in:

| Dimension | Question |
|---|---|
| hardware truth | Is the change true for every board in the blast radius? |
| binding | Does any schema constrain the changed value? |
| driver | Does any driver behavior depend on it? |
| tooling | Would `dtc`, `W=1`, or `dtbs_check` have reported the old or new state? |
| deployed artifacts | Which shipped DTBs or overlays change meaning? |
| tests | Which hardware test proves the change? |

Expected findings to confirm:

- `73387da70f9c` lowers a shared bus speed for one device's sake, and the commit argues it does not affect “any other devices on the bus.” Verify that claim for each board's `main_i2c1` children.
- `69c570ebc396` changes no hardware description, yet it changes the bytes of `k3-am625-sk.dtb` and `k3-am62-lp-sk.dtb`, because they become `-@` overlay bases. Build both at v6.6 and v6.7 and compare `fdtget -p ... /__symbols__ | wc -l`.
- `a0b8da04153e` is invisible to every tool. Both `PIN_INPUT` and `PIN_INPUT_PULLUP` are valid. Decode the pad values from your built DTBs:

```sh
P=/bus@f0000/pinctrl@f4000/main-mmc0-default-pins
fdtget -tx k3-am625-sk.dtb  $P pinctrl-single,pins
fdtget -tx k3-am62-lp-sk.dtb $P pinctrl-single,pins
```

At v6.19 the SK shows `0x54000` for CMD, CLK, and DAT0 and `0x64000` for DAT1–DAT7, while the LP SK shows `0x54000` everywhere. From `k3-pinctrl.h`, `0x54000` is `PIN_INPUT` (input enable, Schmitt trigger, pull disabled) and `0x64000` is `PIN_INPUT_PULLUP`.
- `2517e476b819` moves content without changing the AM625 SK DTB's meaning. Prove that with a sorted semantic diff of the DTB built before and after.
- `1446fc4dc072` depends on earlier binding and driver changes. Find them from the commit message and order them.

## Part 3: Plan A Downstream Branch

A product team maintains products based on the AM625 SK and the LP SK on a v6.12 branch. Write a patch ledger entry (see [Maintain A Downstream Patch Ledger](upstream-development-and-downstream-patch-stack-discipline.md#maintain-a-downstream-patch-ledger)) for each event they need. Then answer:

1. `a0b8da04153e` is tagged for stable. On v6.12, which files does it touch, and where does the pin group live in v6.19 after `2517e476b819`? What `drop_when` rule still works after the move?
2. If the team had earlier fixed the SK by adding pull-ups in its own copy of `k3-am62x-sk-common.dtsi`, which product would have silently changed, and how would the upstream backport conflict?
3. Which of the four events can be carried independently, and which must travel with binding or driver changes?

## Part 4: Replace The CI Design

Design CI for the family that would have caught or contained each event:

- build every board in the include-graph blast radius of any changed file
- compose every `*-dtbs` overlay pair in the TI `Makefile` and fail on any overlay that is built but composed with no base (the v6.6 “floating” state)
- run `dtbs_check` without excluding schemas
- produce a sorted semantic diff of every affected DTB, and require an approved mutation ID for each changed property
- keep per-board semantic assertions for facts no tool checks, such as the eMMC pad values above

## Part 5: Define Semantic Assertions

Write assertions for at least: root `model` per board; the eMMC pad configuration per board; `main_i2c1` `clock-frequency`; the presence of `opp-1400000000` only on the AM625 SK and SiP SK; `phy-mode = "rgmii-id"` on every enabled CPSW port; and `__symbols__` present on every overlay base. Each assertion should name the hardware fact it protects.

## Part 6: Select Hardware Qualification

For each event, name the board and test that proves it: eMMC stress at HS200 on the AM625 SK with and without internal pull-ups, HDMI hot-plug and EDID reads at 100 kHz, overlay application in U-Boot on each base, and Ethernet traffic at 1 Gbit/s on both ports after the RGMII change.

## Part 7: Write The Postmortem For The eMMC Pull-Ups

Write a blameless postmortem for the fact that sat in a shared file from v5.19 to v6.17. Cover the detection gap (no tool can know which board has external pull-ups), why the comment `(Y3/V3)` was a warning sign that one node described two packages, and the process change that would catch the next such fact.

## Deliverables

1. include graphs and blast-radius lists at v6.6 and v6.19
2. the dimension tables for all five commits
3. the downstream patch ledger and branch answers
4. the CI design and semantic assertions
5. the hardware qualification plan
6. the eMMC postmortem

## Reference Analysis

The shared-file model was sound for wiring that is identical across boards, such as most of `main_i2c1`, which is why `73387da70f9c` could change one line and fix all of them. It failed for the eMMC pins, an electrical fact that depends on each board's resistors. No schema, `dtc` check, or boot test distinguishes `PIN_INPUT` from `PIN_INPUT_PULLUP`; only a pin review against each schematic does. `69c570ebc396` shows that build composition is part of the description: until an overlay is composed in Kbuild, its bases are not built with `-@`, so the overlay is untested and unusable at boot. `2517e476b819` shows the cost of refactoring: every downstream patch against `k3-am625-sk.dts` must be re-targeted. `1446fc4dc072` shows a migration done in the right order, binding and drivers first, then 33 DTS files, with downstream users testing before merge.

## Further Reading

- [Layering, Variants, Ownership, And Source Architecture](layering-variants-ownership-and-source-architecture.md)
- [Binding Evolution, Deprecation, And Migration](binding-evolution-deprecation-and-migration.md)
- [Matrix CI, Artifact Validation, And Compatibility Testing](matrix-ci-artifact-validation-and-compatibility-testing.md)
- [Hardware Coverage, Release Qualification, And Learning From Escapes](hardware-coverage-release-qualification-and-learning-from-escapes.md)

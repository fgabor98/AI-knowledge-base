---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# Device Tree Artifact Provenance And Diagnosis Lab

This lab follows one product's Device Tree from source to Linux on the TI AM62x LP SK and diagnoses an incident with three independent causes, each rooted in real behavior of mainline Linux and TI U-Boot. Your job is to preserve evidence and find each first divergence, rather than editing DTS until the symptom changes. Checkpoints use this module's `P0`–`P6` names.

## Incident

The product is an AM62x LP SK with HDMI audio, built from mainline `k3-am62-lp-sk.dts` and `k3-am62x-sk-hdmi-audio.dtso`. After an update:

- Linux reports model `Texas Instruments AM625 SK` instead of `Texas Instruments AM62x LP SK`
- there is no `AM62x-Sil9022-HDMI` sound card
- U-Boot prints an overlay error and still boots
- the engineer shows a freshly built `k3-am62-lp-sk.dtb` that has `__symbols__`
- the product's own firmware helper, which adds a `/chosen` property before boot, sometimes crashes

## Release Inputs

```text
U-Boot      built by the release job from am62x_evm_a53_defconfig
kernel      built from the current tree
/boot/dtb/  copied by the packaging job from out/release/arch/arm64/boot/dts/ti/
overlay     name_overlays=ti/k3-am62x-sk-hdmi-audio.dtbo in the U-Boot environment
```

`out/release` was last fully built from a v6.6 tree. That release contained `k3-am62x-sk-hdmi-audio.dtso` (added in `b50ccab9e07c`) but not the Makefile compositions that make Kbuild build `k3-am625-sk.dtb` and `k3-am62-lp-sk.dtb` with `-@`. Those came in v6.7 with `69c570ebc396`, “Fix HDMI Audio overlay in Makefile,” reported by Rob Herring because the overlay had been left “in a floating state.”

## U-Boot Log Excerpt

```text
Board: AM62-LP-SKEVM rev ...
...
failed on fdt_overlay_apply(): FDT_ERR_NOTFOUND
base fdt does not have a /__symbols__ node
make sure you've compiled with -@
...
Starting kernel ...
```

## The Helper

```c
/* Product helper: record the active root slot before boot. */
int add_boot_slot(void *fdt, const char *slot)
{
        int chosen = fdt_path_offset(fdt, "/chosen");
        int len;
        const char *stdout_path = fdt_getprop(fdt, chosen, "stdout-path", &len);

        fdt_setprop_string(fdt, chosen, "product,boot-slot", slot);
        log_info("console: %s", stdout_path);

        return 0;
}
```

The caller passes the DTB exactly as read from the file, in a buffer of the file's length.

## Objectives

Produce:

1. the `P0`–`P6` ledger for the shipped and the intended chain
2. exact commands that prove each artifact's identity and symbol state
3. an overlay reproduction that separates the two possible `FDT_ERR_NOTFOUND` causes
4. a U-Boot capture plan for the pre-handoff tree
5. a corrected helper
6. the three root causes, owners, and CI gates

## Task 1: Freeze Evidence

Copy and hash, read-only: the U-Boot binaries and its `.config`, the full serial log, `printenv` output, every file under `/boot/dtb/ti/`, the overlay, both `out/` trees with their `.cmd` records, the kernel `Image`, and `/sys/firmware/fdt` from the failing boot. Explain why rebuilding `out/release` or running `fdtput` on the shipped DTB before this step would destroy the evidence.

## Task 2: Trace The Build Rule

In the current kernel tree:

```sh
grep -n 'k3-am62-lp-sk' arch/arm64/boot/dts/ti/Makefile
grep -n -e '-@' scripts/Makefile.dtbs
make ARCH=arm64 V=1 ti/k3-am62-lp-sk.dtb
cat arch/arm64/boot/dts/ti/.k3-am62-lp-sk.dtb.cmd
```

Confirm that `k3-am62-lp-sk-hdmi-audio-dtbs := k3-am62-lp-sk.dtb k3-am62x-sk-hdmi-audio.dtbo` makes the base a member of `base-dtb-y`, and that the recorded `dtc` command contains `-@`. Then check out the v6.6 Makefile (`git show v6.6:arch/arm64/boot/dts/ti/Makefile | grep hdmi`) and explain why the same command line there would not contain `-@` (in v6.6 the rule lived in `scripts/Makefile.lib`, with the same `base-dtb-y` condition). State why the engineer's fresh build proves nothing about `out/release`.

## Task 3: Inspect Binary Artifacts

For each artifact, query:

```sh
fdtget  X.dtb / model
fdtget -p X.dtb /__symbols__ | grep -c .
fdtget  X.dtb /__symbols__ codec_audio
fdtget -p X.dtb /chosen
```

| Artifact | Model | Symbols | `codec_audio` | Interpretation |
|---|---|---|---|---|
| engineer's fresh `k3-am62-lp-sk.dtb` | | | | |
| `out/release/.../k3-am62-lp-sk.dtb` | | | | |
| `/boot/dtb/ti/k3-am625-sk.dtb` on the unit | | | | |
| `/sys/firmware/fdt` from the failing boot | | | | |

## Task 4: Find Why The Wrong Board Was Loaded

Read `board/ti/am62x/evm.c` in TI U-Boot. `setup_board_eeprom_env()` correctly sets `board_name=am62x_lp_skevm`, but `board_late_init()` then calls `ti_set_fdt_env(NULL, NULL)`: there is no board-to-file map on AM62x, so `fdtfile` always comes from `CONFIG_DEFAULT_DEVICE_TREE`. Compare `am62x_evm_a53_defconfig` (`ti/k3-am625-sk`) with `am62x_lpsk_a53_defconfig` (`ti/k3-am62-lp-sk`). Identify the first checkpoint where the shipped chain diverges from the intended one, and explain why the EEPROM log line `Board: AM62-LP-SKEVM` does not contradict this.

## Task 5: Reproduce The Overlay Failure

Run, on pristine copies:

```sh
fdtoverlay -i fresh/k3-am62-lp-sk.dtb   -o m1.dtb k3-am62x-sk-hdmi-audio.dtbo
fdtoverlay -i release/k3-am62-lp-sk.dtb -o m2.dtb k3-am62x-sk-hdmi-audio.dtbo
fdtoverlay -i fresh/k3-am642-sk.dtb     -o m3.dtb k3-am62x-sk-hdmi-audio.dtbo
```

Expected: the first succeeds; the second and third both print `Failed to apply 'k3-am62x-sk-hdmi-audio.dtbo': FDT_ERR_NOTFOUND`. `fdtoverlay` cannot tell you that the second base lacks `__symbols__` while the third lacks the labels. U-Boot's `fdt_overlay_apply_verbose()` can, which is why its log adds the `-@` hint. Confirm each cause with the Task 3 queries.

## Task 6: Capture The Pre-Handoff Tree

Write the U-Boot sequence that, after `get_overlay_mmc`, records `fdtfile`, `fdt addr`, `fdt header` (`totalsize`), `fdt print /model`, and `fdt list /chosen`, then saves exactly `totalsize` bytes ([Capture The Pre-Handoff FDT](u-boot-fdt-commands-and-handoff-inspection.md#capture-the-pre-handoff-fdt)). State which commands must not be run against the control FDT (`fdt addr -c`) during this diagnosis and why.

## Task 7: Review The Helper

The helper has at least these defects; demonstrate the first two with a small host program linked against the kernel's `scripts/dtc/libfdt`:

1. A tightly packed blob has no free space. `fdt_setprop_string()` returns `-3`, `FDT_ERR_NOSPACE`, and the helper ignores it.
2. After `fdt_open_into()` gives the tree headroom, the same call succeeds, but `stdout_path`, read *before* the write, no longer points at the property. Logging it afterwards reads stale memory, which is the intermittent crash.
3. `chosen` is not checked for a negative error code before use.
4. `len` is ignored, so the string is trusted to be terminated.
5. `slot` is not validated against an allowed set.
6. The function returns 0 regardless of what happened.

Sketch the corrected sequence: check the header, `fdt_open_into()` into a buffer sized for the worst case, look up `/chosen` and check it, validate `slot`, write and check the result, re-read any property you still need, and return the error.

## Task 8: Produce The Root-Cause Chain

| Cause | First divergence | Owner | Correction | Regression test |
|---|---|---|---|---|
| U-Boot built from the AM625 SK defconfig | `fdtfile` at P4 | boot firmware release | build `am62x_lpsk_a53_defconfig` | assert `fdtfile` and Linux `model` per board in CI |
| DTBs copied from a pre-v6.7 output tree | P2 artifact in P3 package | packaging | build DTBs in the release job | assert `__symbols__` in every base listed in a `*-dtbs` composition |
| helper ignores libfdt errors and keeps stale pointers | P5 mutation | product firmware | corrected helper | unit tests on a packed blob and a grown blob |

Explain why fixing any one cause leaves a visible symptom, and why the overlay error alone should have stopped the boot.

## Task 9: Build CI Gates

Define gates for: building and hashing every shipped DTB in the release job; requiring `__symbols__` in overlay bases; composing every supported base/overlay pair with `fdtoverlay` and validating the result; asserting per-board `fdtfile` and Linux `model`; failing the boot script when an overlay fails; and libfdt error-path tests.

## Completion Criteria

The lab is complete when you can show the frozen evidence and hashes, both `P0`–`P6` ledgers, the artifact table, the Makefile and defconfig evidence, the three overlay runs with their causes, the U-Boot capture plan, the helper analysis with host-program output, and the root-cause and CI tables.

## Authoritative References

- [Linux Kbuild documentation](https://docs.kernel.org/kbuild/)
- [Linux Devicetree schema testing](https://docs.kernel.org/devicetree/bindings/writing-schema.html)
- [U-Boot FIT documentation](https://docs.u-boot.org/en/latest/usage/fit/index.html)
- [U-Boot `fdt` command](https://docs.u-boot.org/en/latest/usage/cmd/fdt.html)
- [U-Boot Device Tree overlays](https://docs.u-boot.org/en/latest/usage/fdt_overlays.html)
- [Devicetree compiler and `libfdt` source](https://git.kernel.org/pub/scm/utils/dtc/dtc.git/)

## Next Step

Continue with [Security And Production Lifecycle](../security-and-production-lifecycle.md), treating DTB identity, overlay selection, mutation, and update compatibility as security-controlled product interfaces.

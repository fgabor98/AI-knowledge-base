---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# Diagnostic Workflow, Semantic Diffing, And CI Evidence

Tool output becomes useful when it narrows a hypothesis. Use immutable checkpoints, compare adjacent stages, and stop at the first unexplained divergence. Do not start by decompiling whichever `board.dtb` is easiest to find.

## Triage Questions

1. What exact observable failure occurred?
2. Which final node/property/resource would have to differ to cause it?
3. Which artifact did the failing stage actually consume?
4. What is the nearest earlier trustworthy checkpoint?
5. Which command or owner transformed one into the other?

Example:

```text
symptom: no HDMI audio card on an AM62x LP SK
required final fact: /sound-sii9022 exists and /sound is disabled
P2 built k3-am62-lp-sk.dtb: correct, has __symbols__
P3 packaged k3-am62-lp-sk.dtb: copied from a v6.6 build, no __symbols__
P4 U-Boot selected DTB: k3-am625-sk.dtb (wrong board), overlay failed
root causes: packaging input and U-Boot defconfig, not DTS or driver
```

The [lab](device-tree-artifact-provenance-and-diagnosis-lab.md) works through this incident.

## Artifact Ledger

| Checkpoint | Identity | Hash | Producer/selector | Expected changes |
|---|---|---|---|---|
| P0 | source commit + entry DTS | repository state | Kbuild target | includes/macros |
| P1 | preprocessed DTS | hash | cpp command | expansion only |
| P2 | built DTB | hash | dtc command | flatten/symbols |
| P3 | packaged DTB | extracted hash | packaging rule | ideally none |
| P4 | selected/merged | hash | boot config/overlays | declared overlay delta |
| P5 | pre-handoff FDT | hash | firmware/U-Boot | owned fixups |
| P6 | live tree | semantic capture | Linux | early-kernel effects only |

Do not compare P0 text hash with P2 binary hash. Compare like artifacts or decoded semantics.

## Exact Versus Semantic Comparison

Exact comparison:

```bash
sha256sum built.dtb packaged-extracted.dtb uboot-captured.dtb
cmp -s built.dtb packaged-extracted.dtb
```

Semantic normalization:

```bash
dtc -s -I dtb -O dts -o built.sorted.dts built.dtb
dtc -s -I dtb -O dts -o captured.sorted.dts uboot-captured.dtb
diff -u built.sorted.dts captured.sorted.dts
```

Exact mismatch with semantic equality can come from padding, order, symbols, or packing. Semantic mismatch requires an owner. Exact equality proves no byte mutation between those two captures.

## Normalize Carefully

Decompiled, sorted diffs contain noise from phandle renumbering, generated symbols, ordering, and tool-version formatting ([Semantic Diff](../boot-time-mutation-and-ownership/final-tree-validation-diffing-and-runtime-forensics.md#semantic-diff)). Use the same `dtc` version and flags on both sides, and query exact paths with `fdtget` for focused questions. Do not normalize away evidence needed for the hypothesis—for example, remove `__symbols__` only if overlay compatibility is not under investigation.

## Layered Diagnosis

### Build Layer

- correct target and entry DTS?
- expected include/macro branch?
- generated inputs current?
- output tree correct?
- `V=1` command and tool version expected?
- warnings/schema checks passed?

### Packaging Layer

- correct DTB copied/embedded?
- rename/configuration mapping correct?
- extracted payload hash equals built artifact?
- signing/compression uses expected input?

### Boot Selection Layer

- exact boot script/FIT/extlinux path?
- base identity and hash?
- overlays selected/authenticated in canonical order?
- working FDT address distinct from control FDT?

### Mutation Layer

- capacity and buffer interval safe?
- every mutation owner/status logged?
- pre-handoff capture taken after final `/chosen` changes?

### Runtime Bridge

- early kernel reports expected model/machine?
- live-tree property equals handoff capture?
- device created, matched, deferred, or failed?

[Runtime Inspection](../runtime-inspection.md) covers this layer in depth.

## Property Assertions

Automate high-value invariants:

```bash
test "$(fdtget final.dtb / model)" = 'Texas Instruments AM62x LP SK'
test "$(fdtget final.dtb /sound-sii9022 simple-audio-card,name)" = 'AM62x-Sil9022-HDMI'
test "$(fdtget final.dtb /sound status)" = 'disabled'
test "$(fdtget -tx final.dtb /sound-sii9022/simple-audio-card,cpu sound-dai)" = \
     "$(fdtget -tx final.dtb "$(fdtget final.dtb /__symbols__ mcasp1)" phandle)"
```

All four pass on `k3-am62-lp-sk.dtb` with `k3-am62x-sk-hdmi-audio.dtbo` applied. The last one checks the relationship, that the card's CPU DAI resolves to the node the base calls `mcasp1`, rather than a phandle number that changes between builds.

Shell output formatting varies. For robust CI, compare token counts/types or use `libfdt`/structured tooling rather than depending on display whitespace.

Assertions should cover:

- root compatible/model
- selected boot console
- critical memory/reservations
- boot storage and network paths
- required providers/consumers
- overlay postconditions
- absence of forbidden/conflicting nodes

## Warning Baselines

Run before and after with identical source/tool/configuration:

```bash
make O=build ARCH=arm64 W=1 dtbs_check 2>&1 | tee after.log
```

Compare categorized diagnostics:

- new: must be fixed or explicitly justified
- resolved: desired but ensure not hidden by loss of selection
- unchanged baseline: tracked debt, not silently attributed to patch
- tool-induced: validator/compiler version changed; review separately

Do not use a global warning-count threshold. One removed warning and one new warning can keep the count equal.

## CI Matrix

For each affected base/variant:

- build target from clean/reproducible inputs
- run ordinary and required warning levels
- run binding/schema checks
- inspect/assert critical properties
- compose all supported overlays
- reject unsupported overlay combinations
- extract packaged artifacts and compare hashes
- verify release manifest entries
- retain normalized semantic diffs against approved baselines
- boot representative hardware/emulation where available

Shared DTSI changes require building every dependent board, not only the author's board.

## Preserve Failure Evidence

Collect the failing artifact, its hash, boot configuration logs, selected base/overlay identities, and pre-handoff and runtime captures before rebuilding or overwriting anything (see the [field provenance bundle](../boot-time-mutation-and-ownership/final-tree-validation-diffing-and-runtime-forensics.md#field-provenance-bundle)). Keep a known-good comparison artifact, and never run `fdtput` on the evidence—copy it first.

## Decision Tree

```text
Does built DTB contain expected property?
  no -> source/preprocess/build/warning/schema path
  yes
    Does packaged extraction hash/semantics match?
      no -> packaging path
      yes
        Did bootloader select that artifact?
          no -> boot configuration/FIT/filename path
          yes
            Does pre-handoff tree contain expected property?
              no -> overlay/fixup/mutation path
              yes
                Does live tree contain it?
                  no -> handoff/early-kernel path
                  yes -> driver model/config/probe/resource diagnosis
```

## Completion Evidence

A diagnosis is complete when it states:

```text
first divergent checkpoint
exact before/after identities
responsible transform and owner
mechanism of failure
minimal correction at the owning source/policy
tests proving downstream checkpoints realign
regression assertion added to CI
```

## Authoritative References

- [Linux Kbuild documentation](https://docs.kernel.org/kbuild/)
- [Linux Devicetree schema testing](https://docs.kernel.org/devicetree/bindings/writing-schema.html)
- [Linux DTS coding style](https://docs.kernel.org/devicetree/bindings/dts-coding-style.html)
- [U-Boot `fdt` command](https://docs.u-boot.org/en/latest/usage/cmd/fdt.html)
- [Devicetree compiler project](https://git.kernel.org/pub/scm/utils/dtc/dtc.git/)

## Continue

Proceed to the [Device Tree Artifact Provenance And Diagnosis Lab](device-tree-artifact-provenance-and-diagnosis-lab.md).

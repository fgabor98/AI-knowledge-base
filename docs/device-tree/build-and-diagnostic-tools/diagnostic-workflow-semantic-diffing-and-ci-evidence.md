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
symptom: second clock missing in driver
required final fact: clocks has two entries named bus/sample
C2 built DTB: correct
C3 packaged FIT DTB: correct
C4 U-Boot selected DTB: wrong board variant
root cause: FIT configuration selection, not DTS or driver
```

## Artifact Ledger

| Checkpoint | Identity | Hash | Producer/selector | Expected changes |
|---|---|---|---|---|
| C0 | source commit + entry DTS | repository state | Kbuild target | includes/macros |
| C1 | preprocessed DTS | hash | cpp command | expansion only |
| C2 | built DTB | hash | dtc command | flatten/symbols |
| C3 | packaged DTB | extracted hash | packaging rule | ideally none |
| C4 | selected/merged | hash | boot config/overlays | declared overlay delta |
| C5 | pre-handoff FDT | hash | firmware/U-Boot | owned fixups |
| C6 | live tree | semantic capture | Linux | early-kernel effects only |

Do not compare C0 text hash with C2 binary hash. Compare like artifacts or decoded semantics.

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

A decompiled/sorted diff can contain noise:

- renumbered phandles
- generated symbols/fixups
- property/node order
- omitted source labels/comments
- tool-version formatting

Use the same `dtc` version and flags for both. For focused questions, query exact paths/properties with `fdtget` instead of diffing entire trees.

Do not normalize away evidence needed for the hypothesis—for example, remove `__symbols__` only if overlay compatibility is not under investigation.

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

The next module expands runtime inspection.

## Property Assertions

Automate high-value invariants:

```bash
test "$(fdtget final.dtb / model)" = 'Acme Falcon revision B'
test "$(fdtget final.dtb /soc/capture@48000000 status)" = 'okay'
test "$(fdtget final.dtb /soc/capture@48000000 clock-names)" = 'bus sample'
```

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

Collect before rebuilding or overwriting:

- failing artifact/container
- exact hash and size
- bootloader/environment/configuration logs
- selected base/overlay identities
- pre-handoff capture when possible
- runtime live-tree capture and logs
- tool versions and commands
- known-good comparison artifact

Do not run `fdtput` on the evidence. Copy it first.

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

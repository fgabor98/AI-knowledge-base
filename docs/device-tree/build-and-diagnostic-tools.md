---
status: draft
reviewed: false
domain: device-tree
difficulty: intermediate
last_reviewed: null
---

# Build And Diagnostic Tools

Device Tree debugging is an artifact-provenance problem. Engineers must prove which source entry point and includes produced a DTB, which compiler and flags transformed it, which file was packaged and selected, which overlays and fixups changed it, and which final blob reached the operating system. The tools are useful only when attached to those checkpoints.

## Learning Outcomes

After completing this module, you should be able to:

- trace a DTB from its Kbuild target through preprocessing, `dtc`, packaging, deployment, bootloader selection, mutation, and OS handoff
- reproduce and inspect the preprocessed DTS without confusing it with maintainable source
- use verbose Kbuild output and generated command/dependency records to identify exact inputs, flags, and outputs
- compile, decompile, sort, and inspect DT artifacts with `dtc` while preserving the distinction between semantic equivalence and source reconstruction
- interpret ordinary, `W=1`, and `W=2` build diagnostics without suppressing warnings blindly
- use `fdtdump` for structural inspection and reservation data
- use `fdtget` to query nodes and decode strings, cells, and byte-oriented properties
- use `fdtput` only on disposable copies with explicit types, preconditions, and validation
- compose overlays with `fdtoverlay` and validate the merged artifact in canonical order
- distinguish U-Boot's control FDT from its OS-bound working FDT and inspect the correct one
- use U-Boot `fdt` commands to establish identity, capacity, selected overlays, and pre-handoff state
- apply core `libfdt` read, traversal, mutation, open/pack, overlay, and error-handling patterns safely
- treat offsets, pointers, and property data returned by `libfdt` as invalidatable across structural mutations
- compare DTBs semantically while preserving hashes of exact release artifacts
- reduce a build/deployment/runtime mismatch to the earliest divergent checkpoint

## Prerequisites

Complete [Overlays In Depth](overlays-in-depth.md). You should understand DTS composition, DTB structure, schemas, phandles, overlays, boot-time mutation, and the distinction between the source tree and the final live tree.

## Learning Path

1. [Build Pipeline, Preprocessing, And Artifact Provenance](build-and-diagnostic-tools/build-pipeline-preprocessing-and-artifact-provenance.md)
2. [`dtc`, Symbols, Warnings, And Round Trips](build-and-diagnostic-tools/dtc-symbols-warnings-and-round-trips.md)
3. [`fdtdump` And `fdtget` Binary Inspection](build-and-diagnostic-tools/fdtdump-and-fdtget-binary-inspection.md)
4. [`fdtput`, `fdtoverlay`, And Controlled Artifact Mutation](build-and-diagnostic-tools/fdtput-fdtoverlay-and-controlled-artifact-mutation.md)
5. [U-Boot `fdt` Commands And Handoff Inspection](build-and-diagnostic-tools/u-boot-fdt-commands-and-handoff-inspection.md)
6. [`libfdt` Programming, Capacity, And Error Discipline](build-and-diagnostic-tools/libfdt-programming-capacity-and-error-discipline.md)
7. [Diagnostic Workflow, Semantic Diffing, And CI Evidence](build-and-diagnostic-tools/diagnostic-workflow-semantic-diffing-and-ci-evidence.md)
8. [Device Tree Artifact Provenance And Diagnosis Lab](build-and-diagnostic-tools/device-tree-artifact-provenance-and-diagnosis-lab.md)

## The Checkpoint Model

```text
C0 source entry point + includes + generated headers
  -> preprocess
C1 preprocessed DTS
  -> dtc with exact flags/version
C2 built DTB/DTBO
  -> package/copy/sign
C3 deployed or packaged artifact
  -> boot selection
C4 selected base + ordered overlays
  -> firmware/bootloader mutations
C5 final FDT at OS handoff
  -> unflatten/early kernel processing
C6 Linux live tree and device model
```

At each checkpoint, record identity, hash, producer, command/version, inputs, and expected semantic changes. Debug the first divergence, not the last visible symptom.

## Tool-To-Question Map

| Question | Primary evidence/tool |
|---|---|
| Which rule built this DTB? | Kbuild target, `V=1`, generated `.cmd` dependency record |
| Which includes/macros changed the source? | preprocessed DTS and compiler command |
| Is the blob structurally valid? | `fdt_check_header()`, `dtc`, `fdtdump` |
| What is one exact property value? | `fdtget` with explicit output type |
| What is in the reservation block? | `fdtdump` or `libfdt` reservation APIs |
| What does an overlay produce? | `fdtoverlay`, then final-tree inspection/validation |
| What will U-Boot pass to Linux? | working-FDT address, U-Boot `fdt` inspection, captured blob/hash |
| Which mutation changed the tree? | checkpointed hashes and normalized semantic diffs |
| Why did a libfdt operation fail? | checked return code and `fdt_strerror()` |
| Why did no driver appear? | first prove C5/C6 tree identity, then move to runtime diagnosis |

## Mutation Rule

Inspection tools are safe on immutable artifacts. Mutation tools require:

```text
known input hash
disposable working copy
validated target path/property and prior value
explicit type/encoding
checked command/API status
structural and schema validation afterward
semantic diff against intended change
new output identity/hash
```

Never use `fdtput` or an in-memory `fdt set` as an undocumented substitute for correcting source and rebuilding a release artifact.

## Completion Check

You are ready for [Runtime Inspection](runtime-inspection.md) when you can:

- reproduce the exact preprocessing and `dtc` command that built one target DTB
- identify every included/generated input from Kbuild evidence
- explain every new `W=1` or `W=2` diagnostic instead of globally disabling it
- distinguish DTB structural order, semantic content, source formatting, and binary identity
- decode string lists, cell arrays, phandle arrays, and byte arrays with the appropriate tool/type
- mutate a disposable copy and prove that exactly the intended semantic delta occurred
- merge overlays in product order and validate the final result
- capture and identify U-Boot's working FDT without altering the control FDT
- write a libfdt mutation path with capacity planning, checked errors, and no stale offsets/pointers
- compare built, packaged, selected, pre-handoff, and live checkpoints to locate the first mismatch
- produce CI evidence that includes both exact hashes and normalized semantic diffs

## Authoritative References

- [Devicetree compiler and `libfdt` source](https://git.kernel.org/pub/scm/utils/dtc/dtc.git/)
- [Linux Devicetree schema testing](https://docs.kernel.org/devicetree/bindings/writing-schema.html)
- [Linux DTS coding style and warning guidance](https://docs.kernel.org/devicetree/bindings/dts-coding-style.html)
- [Linux Kbuild documentation](https://docs.kernel.org/kbuild/)
- [U-Boot `fdt` command](https://docs.u-boot.org/en/latest/usage/cmd/fdt.html)
- [U-Boot Device Tree overlays](https://docs.u-boot.org/en/latest/usage/fdt_overlays.html)

## Related Topics

- [Writing And Validating Binding Schemas](writing-and-validating-binding-schemas.md)
- [Overlays In Depth](overlays-in-depth.md)
- [Boot-Time Mutation And Ownership](boot-time-mutation-and-ownership.md)
- [Runtime Inspection](runtime-inspection.md)
- [Device Tree Binding Validation](../build-systems/advanced/linux-kernel/device-tree-binding-validation.md)

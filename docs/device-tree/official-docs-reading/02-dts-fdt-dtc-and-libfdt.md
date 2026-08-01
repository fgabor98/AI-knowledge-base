---
status: active
reviewed: false
domain: device-tree
difficulty: intermediate
last_reviewed: null
---

# 2. DTS Source, FDT Binary Format, dtc, And libfdt

Official sections: [DTSpec DTS format](https://devicetree-specification.readthedocs.io/en/stable/source-language.html) and [DTSpec FDT format](https://devicetree-specification.readthedocs.io/en/stable/flattened-format.html)

Knowledge-guide companion: [Stage 2](knowledge-guide-companion.md#stage-2-dts-source-fdt-binary-format-dtc-and-libfdt)

## DTS Source Language

- [ ] **P0** `/dts-v1/`, root syntax, node/property definitions, and statement termination.
- [ ] **P0** String, string-list, cell, byte-array, empty/boolean, and incbin values.
- [ ] **P0** Labels and references; distinguish node references from value references.
- [ ] **P0** `/memreserve/` entries and their relationship to the FDT reservation block.
- [ ] **P0** `/bits/` encodings and multi-cell integers.
- [ ] **P1** `/delete-node/` and `/delete-property/` as implemented by the project's compiler/source flow.
- [ ] **P1** DTS `/include/` versus C preprocessor `#include`, macros, and generated headers.
- [ ] **P1** `/plugin/` syntax after completing the ordinary source language.

## Flattened Devicetree Format

- [ ] **P0** FDT header fields, big-endian representation, offsets, sizes, `version`, and `last_comp_version`.
- [ ] **P0** Memory reservation block and 64-bit terminator entry.
- [ ] **P0** Structure block tokens: `FDT_BEGIN_NODE`, `FDT_END_NODE`, `FDT_PROP`, `FDT_NOP`, and `FDT_END`.
- [ ] **P0** Strings block, `nameoff`, NUL termination, and alignment/padding.
- [ ] **P0** `totalsize` versus used structure/string sizes and available growth space.
- [ ] **P1** Binary-format version compatibility versus binding/content compatibility.
- [ ] **P1** Why exact byte identity is different from semantic tree identity.

## dtc Project

- [ ] **P0** Read the [upstream dtc repository README](https://git.kernel.org/pub/scm/utils/dtc/dtc.git/tree/README).
- [ ] **P0** Read the [Device Tree Compiler manual](https://git.kernel.org/pub/scm/utils/dtc/dtc.git/tree/Documentation/manual.txt).
- [ ] **P0** Record the exact dtc source/version used by the kernel and product build; do not assume the host `dtc` is used.
- [ ] **P0** Learn input/output modes `dts`, `dtb`, and `fs`; use `-I`/`-O` explicitly in notes and scripts.
- [ ] **P0** Learn `-@`, symbol generation, warning options, padding/space options, and output dependencies from the exact version.
- [ ] **P1** Read [dt-object internal format](https://git.kernel.org/pub/scm/utils/dtc/dtc.git/tree/Documentation/dt-object-internal.txt) before reasoning about overlays, symbols, and fixups.
- [ ] **P1** Inspect parser/check sources and tests for a warning or syntax behavior that affects the project.

## Utility Tools

- [ ] **P0** `fdtdump` for structural/binary inspection.
- [ ] **P0** `fdtget` for typed reads and path/property queries.
- [ ] **P0** `fdtput` for controlled copies, not direct mutation of sole release artifacts.
- [ ] **P0** `fdtoverlay` for offline composition and negative tests.
- [ ] **P1** `convert-dtsv0` only when handling historical source.
- [ ] **P1** Compare decompiled DTS, `fdtdump`, and exact hashes; state what each proves.

## libfdt

- [ ] **P0** Read `libfdt/libfdt.h` from the exact dtc/U-Boot/kernel source version.
- [ ] **P0** Header validation, `fdt_check_header()`, `fdt_totalsize()`, and error-string handling.
- [ ] **P0** Path/node/property lookup and length-aware raw property access.
- [ ] **P0** Big-endian conversion helpers and cell-width discipline.
- [ ] **P0** Read-only versus sequential-write/read-write APIs.
- [ ] **P0** `fdt_open_into()`, `fdt_pack()`, growth capacity, and `-FDT_ERR_NOSPACE` handling.
- [ ] **P1** Node/property mutation, phandle lookup/allocation, and failure atomicity from implementation/tests.
- [ ] **P1** Overlay application APIs after reading the object format and resolver contract.

## Linux Build Integration

- [ ] **P0** Locate the architecture DTS Makefiles and exact board target.
- [ ] **P0** Inspect preprocessing commands, include paths, generated `dt-bindings` headers, dtc flags, and dependency files.
- [ ] **P1** Compare standalone dtc output with the kernel build output and explain every difference.
- [ ] **P1** Record source commit, preprocessed input, compiler version, flags, output hash, and packaging path.

## Practical Exercises

- [ ] Compile one DTS to DTB, inspect its header/blocks, and decode it back using the same tool version.
- [ ] Read a NUL-separated string list and big-endian cell array without treating either as ordinary text.
- [ ] Trigger and classify one dtc warning; find the check in source.
- [ ] Add controlled padding, mutate a copy with libfdt or fdtput, and demonstrate capacity failure.
- [ ] Build a base with `-@`, inspect `__symbols__`, and explain why ordinary DTB semantics do not require that node.
- [ ] Prove whether a round trip is semantically equivalent and whether it is byte-identical.

## Stage Completion

- [ ] I can move between DTS, preprocessed source, DTB, and filesystem-tree representations without confusing them.
- [ ] I can parse the FDT header and locate its reservation, structure, and strings blocks.
- [ ] I can use dtc tools and libfdt with explicit versions, types, capacity, and error handling.
- [ ] I can prove the exact compiler and command that produced a deployed DTB.

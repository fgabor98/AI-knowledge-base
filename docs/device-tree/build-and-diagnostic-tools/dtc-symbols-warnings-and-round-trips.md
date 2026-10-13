---
status: draft
reviewed: false
domain: device-tree
difficulty: intermediate
last_reviewed: null
---

# `dtc`, Symbols, Warnings, And Round Trips

`dtc` converts between supported Device Tree representations and performs structural checks. Successful compilation proves that the input is representable; it does not prove binding correctness, correct hardware, correct deployment, or driver support.

## Basic Compilation

```bash
dtc -I dts -O dtb -o board.dtb board.dts
```

Important dimensions:

- input format selected by `-I`
- output format selected by `-O`
- output path selected by `-o`
- include search paths and preprocessed input supplied by the build
- warning checks enabled/disabled by the build
- symbol generation for overlay workflows
- padding/minimum size policies where explicitly required

Prefer the project build system for production artifacts. Standalone commands are valuable for isolated experiments and inspection.

## Symbol Generation

```bash
dtc -@ -I dts -O dtb -o k3-am625-sk.dtb k3-am625-sk.pp.dts
dtc -@ -I dts -O dtb -o k3-am62x-sk-hdmi-audio.dtbo k3-am62x-sk-hdmi-audio.pp.dts
```

`-@` emits symbol/fixup information used by label-based overlays. It can change artifact contents and size: the AM625 SK DTB is 61,786 bytes with symbols and 46,668 without. Compare symbol-bearing and non-symbol-bearing DTBs semantically before treating a byte/hash difference as a hardware-description change.

Do not add `-@` casually to one ad hoc build while production uses different rules. Make symbol policy explicit in Kbuild or packaging.

## Decompilation

```bash
dtc -I dtb -O dts -o board.decoded.dts board.dtb
```

Decompilation reveals the flattened semantic tree, including numeric phandles and compiler-generated metadata. It cannot reconstruct:

- DTS/DTSI file boundaries
- original labels unless symbols preserve some mappings
- macro names and conditional branches
- comments
- formatting
- intent behind overrides
- packaging provenance

Use decompiled output as evidence of a binary artifact, never as proof of source ownership.

## Round Trips

```bash
dtc -I dtb -O dts -o roundtrip.dts original.dtb
dtc -I dts -O dtb -o roundtrip.dtb roundtrip.dts
```

Byte-for-byte equality is not guaranteed. Differences can include:

- structure/string ordering
- padding
- phandle spelling or assignment representation
- symbol/fixup metadata
- reservation ordering
- tool-version output choices

Compare decoded semantics and exact hashes separately. If signed/released bytes matter, use the original artifact, not a recompiled decompilation.

## Deterministic Sorting For Review

Some `dtc` versions support sorted output. A sorted decoded tree can make semantic diffs easier:

```bash
dtc -s -I dtb -O dts -o board.sorted.dts board.dtb
```

Sorting changes serialization/order and must not replace the release artifact. Normalize both sides with the same tool/version and record that the output is diagnostic.

## Warning Layers

Warnings include structural classes such as:

- unit-address and `reg` inconsistencies
- malformed or missing address/size-cell context
- graph endpoint issues
- duplicate or suspicious node/property patterns
- bus-specific conventions
- interrupt-provider relationships

The exact check set changes with `dtc` and kernel versions. A warning name from one release may be renamed, promoted, or split in another.

## Kernel Warning Levels

Use the platform build rather than guessing standalone `-W` flags:

```bash
make O=build ARCH=arm64 dtbs
make O=build ARCH=arm64 W=1 dtbs
make O=build ARCH=arm64 W=2 dtbs
```

`W=1` and `W=2` request increasingly broad build warnings according to current Kbuild policy. They may surface warnings outside the one DTS because included shared sources affect many boards.

For schema plus compiler validation:

```bash
make O=build ARCH=arm64 W=1 dtbs_check
```

Follow the affected maintainer tree's expectations. Some platforms require no new `W=1` DT warnings.

## Diagnose A Warning

For each warning:

1. capture exact tool version, target, and full message
2. identify the final node/property path
3. map it to preprocessed source and owning include
4. read the parent bus/provider binding
5. decide whether source, schema, or compiler expectation is wrong
6. fix the narrow owner
7. rebuild every affected board sharing that source
8. confirm no warning was merely hidden by selection changes

Do not globally disable a warning because one downstream tree has historical noise. If a narrowly justified suppression is required, document scope, tool-version rationale, and an exit plan.

Also check whether the input is complete. Two real overlays warn only because `dtc` compiles them without their base:

```text
k3-am642-evm-icssg1-dualemac.dtso:16.3-51: Warning (alias_paths): /fragment@0/__overlay__/aliases:ethernet1: aliases property is not a valid node (/icssg1-eth/ethernet-ports/port@1)
k3-am62x-sk-csi2-ov5640.dtso:90.8-105.4: Warning (graph_child_address): /fragment@2/__overlay__/ports: graph node has single child node 'port@0', #address-cells/#size-cells are not necessary
```

The alias target and the other four `port@N` nodes exist only after the overlay is applied. Decompiling the merged DTBs (`k3-am642-evm.dtb` with the ICSSG overlay, `k3-am625-sk.dtb` with the OV5640 overlay) produces neither warning. Re-check on the merged tree before deciding a warning is real.

## `dtc` Versus Schema Checks

| `dtc` can establish | `dtc` cannot establish alone |
|---|---|
| syntax and binary encodability | documented compatible exists |
| several structural/bus invariants | required binding properties are complete |
| phandle/reference resolvability in complete source | vendor property is documented |
| some graph/address conventions | electrical wiring is correct |
| DTB header/structure conversion | deployed artifact is the built artifact |

Run `dt_binding_check` before trusting `dtbs_check`, as covered in the schema module.

## Binary Validation As Input

Decompiling malformed input is not a substitute for checking API/tool status. `dtc` should reject invalid structures, but programmatic consumers should call `fdt_check_header()` and check totalsize/buffer bounds before traversal.

Never point diagnostic tools at arbitrary physical memory without first copying a bounded blob whose header and size have been validated.

## Useful Comparison Set

For one change, retain:

```text
before/after exact DTB hashes
before/after sorted decoded DTS
compiler and warning logs
preprocessed source around changed nodes
schema validation log
symbol/reservation inspection when relevant
```

This separates intended semantic change from build noise.

## Common Mistakes

- treating no compiler error as binding validation
- compiling raw kernel DTS without preprocessing
- forgetting `-@` for a label-targeted overlay base
- decompiling and then maintaining the flattened output
- diffing decoded files made by different tool versions
- disabling a warning without understanding the bus model
- building one board after editing a shared `.dtsi`
- assuming the DTB hash identifies the running tree after boot fixups

## Authoritative References

- [Devicetree compiler source and documentation](https://git.kernel.org/pub/scm/utils/dtc/dtc.git/)
- [Linux DTS coding style](https://docs.kernel.org/devicetree/bindings/dts-coding-style.html)
- [Linux Devicetree schema testing](https://docs.kernel.org/devicetree/bindings/writing-schema.html)
- [Linux Kbuild extra warnings](https://docs.kernel.org/kbuild/kbuild.html)

## Continue

Proceed to [`fdtdump` And `fdtget` Binary Inspection](fdtdump-and-fdtget-binary-inspection.md).

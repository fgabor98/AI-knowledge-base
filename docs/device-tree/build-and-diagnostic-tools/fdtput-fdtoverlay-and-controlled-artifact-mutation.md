---
status: draft
reviewed: false
domain: device-tree
difficulty: intermediate
last_reviewed: null
---

# `fdtput`, `fdtoverlay`, And Controlled Artifact Mutation

`fdtput` is useful for controlled experiments, fixture generation, and reproducing firmware changes. `fdtoverlay` composes overlays on a host. Both produce new diagnostic states; neither replaces the reviewed source/build/package pipeline for a release.

## Always Start From A Copy

```bash
cp -- board.dtb board.experiment.dtb
sha256sum board.dtb board.experiment.dtb
```

Record:

```text
input hash
tool version
exact command
expected prior value
intended result
output hash
validation results
```

Never mutate the only copy of a signed, deployed, captured, or field-failure artifact.

## Replace A String Property

```bash
fdtget board.experiment.dtb /chosen bootargs

fdtput -t s board.experiment.dtb /chosen bootargs \
  'console=ttyS0,115200 root=/dev/mmcblk0p2 ro'

fdtget board.experiment.dtb /chosen bootargs
```

Quoting belongs to the shell; string encoding belongs to the tool. Check that the output contains one intended string rather than shell-split fragments.

Use this to reproduce a bootloader change, not to establish permanent boot policy in an opaque binary.

## Replace Cells

```bash
fdtget -tx board.experiment.dtb /clock@0 clock-frequency
fdtput -t x board.experiment.dtb /clock@0 clock-frequency 16e3600
fdtget -tx board.experiment.dtb /clock@0 clock-frequency
```

This example uses a hexadecimal cell value. Exact type grammar, cell widths, and accepted value spelling depend on the installed dtc utilities; inspect `fdtput --help`.

Never change raw cells before deriving their grouping from the binding and parent/provider cell counts. A 64-bit quantity usually needs two 32-bit cells in DT encoding.

## Node And Property Operations

Common `fdtput` versions support modes to:

- create nodes, optionally creating parent paths
- set or replace a property
- delete a property
- remove a node

Flags differ across versions. Discover locally:

```bash
fdtput --help
```

Before deletion, prove the node/property exists and record its value. After structural mutation, inspect the full affected subtree; removing one node can leave aliases, phandle consumers, graph endpoints, or `*-names` lists dangling.

## Boolean Properties

A DT boolean is an empty property whose presence means true. Do not encode false as a numeric zero unless the binding defines a numeric property. Use the version-specific `fdtput` syntax for an empty property and verify presence with property listing/dump rather than interpreting a value.

## Mutation Preconditions

Automated mutation should fail unless:

- input hash/version is approved
- node path exists and has expected compatible
- prior property is absent or has an allowed exact value
- new encoding matches binding type/cardinality
- sufficient blob capacity/tool behavior is known
- output path differs from immutable input

Blind “set if present, create if absent” behavior can make an incompatible base look superficially patched.

## Semantic Diff After Mutation

```bash
dtc -s -I dtb -O dts -o before.sorted.dts board.dtb
dtc -s -I dtb -O dts -o after.sorted.dts board.experiment.dtb
diff -u before.sorted.dts after.sorted.dts
```

Confirm exactly the intended node/property changed. Then run structural and schema checks appropriate to the artifact.

The decoded diff is diagnostic normalization. Preserve exact before/after hashes separately.

## Apply Overlays On The Host

```bash
fdtoverlay -i base.dtb -o merged.dtb \
  board-revision.dtbo module-power.dtbo module-sensor.dtbo
```

Overlay order is the command-line order and part of the resulting semantics. Start from the pristine base for each test. Check command status before inspecting the output.

Inspect:

```bash
sha256sum base.dtb board-revision.dtbo module-power.dtbo \
  module-sensor.dtbo merged.dtb
dtc -s -I dtb -O dts -o merged.sorted.dts merged.dtb
fdtdump merged.dtb
```

Then run binding/schema and product resource-conflict validation on the merged tree.

## Check Overlay Inputs Before Applying

```bash
fdtdump base.dtb > base.dump.txt
fdtdump module-sensor.dtbo > module-sensor.dump.txt
rg -n '__symbols__|expansion_spi' base.dump.txt
rg -n '__fixups__|expansion_spi' module-sensor.dump.txt
```

Verify:

- base exports every required external symbol
- target paths exist
- prior overlays supply stack dependencies
- DTBO is compiled as a plugin with expected metadata
- manifest permits the base/version/order
- output buffer/file is disposable

`fdtoverlay` can resolve and merge a composition that is electrically impossible. It is not a pin, address, voltage, or ownership solver.

## Negative Fixture Generation

Controlled mutation is valuable for proving checks:

```text
copy valid DTB
remove one required property
run validator and observe expected failure
discard copy
```

Examples:

- delete AXC200 `reset-names`
- reverse `clock-names`
- change provider `#gpio-cells`
- add an undocumented property
- change root compatible to an unsupported base

Do not commit a binary mutation without a reproducible fixture generator and reason.

## Do Not Patch Signed Inputs After Verification

Changing a verified DTB creates a new unverified artifact unless the mutation itself is inside the trusted, measured policy. Host-side experiments must never be substituted into a secure boot package without the normal signing/release path.

The same applies to overlays: authenticate the ordered input set and, where required, measure the final result.

## Failure Policy

If `fdtput` or `fdtoverlay` fails:

- treat output as invalid or absent
- retain pristine inputs
- capture status and stderr
- do not continue a multi-step mutation chain
- identify capacity, structure, target, symbol, or argument error
- restart from a clean copy after correction

Do not attempt to repair a partly changed product artifact in place.

## Controlled-Mutation Checklist

- [ ] immutable input hash recorded
- [ ] command/tool version recorded
- [ ] target and prior value asserted
- [ ] binding-derived type/cardinality used
- [ ] command exit status checked
- [ ] exact output hash recorded
- [ ] normalized semantic diff reviewed
- [ ] structural/schema validation passed
- [ ] overlay/resource conflict checks passed
- [ ] experiment cannot overwrite release/deployed inputs

## Authoritative References

- [Devicetree compiler utilities and source](https://git.kernel.org/pub/scm/utils/dtc/dtc.git/)
- [U-Boot Device Tree overlays](https://docs.u-boot.org/en/latest/usage/fdt_overlays.html)
- [Linux Devicetree schema testing](https://docs.kernel.org/devicetree/bindings/writing-schema.html)

## Continue

Proceed to [U-Boot `fdt` Commands And Handoff Inspection](u-boot-fdt-commands-and-handoff-inspection.md).

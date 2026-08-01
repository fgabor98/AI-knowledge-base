---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# Specification, Binding, And Toolchain Compatibility Baselines

“Device Tree support” is not one version number. A product release depends on an FDT binary format, a Devicetree Specification interpretation, binding contracts, dtc/libfdt/dt-schema behavior, bootloader features, kernel consumers, and project-specific conventions. Record these separately before reasoning about compatibility.

## Separate The Standards Layers

| Layer | Governs | Typical compatibility concern |
|---|---|---|
| FDT binary format | header, blocks, tokens, versions | producer emits a format an older consumer can parse |
| Devicetree Specification | tree model, base types, standard nodes/properties | implementation supports the constructs actually used |
| subsystem/device binding | hardware-specific property contract | old/new producer-consumer behavior remains compatible |
| DTS source language and dtc | includes, labels, overlays, warnings, output layout | build inputs and flags work with pinned tool versions |
| dt-schema tooling | schema dialect, transformations, validation | CI versions produce understood and reproducible results |
| bootloader integration | selection, fixups, overlays, handoff | deployed firmware supports generated artifacts and policy |
| kernel implementation | matching, parsing, providers, quirks | supported DTBs work with supported kernels |
| product convention | file layering, identity, manifest, ownership | teams interpret and maintain sources consistently |

Do not claim “DTSpec vX compatibility” when the actual dependency is a Linux binding or U-Boot overlay feature.

## Inventory Consumers And Producers

For each release line, identify:

```text
source producers:
  kernel DTS tree/commit
  vendor generator or board database
  overlay source repository

build consumers/producers:
  C preprocessor
  dtc
  dt-schema
  libfdt tools
  FIT/package builder

runtime consumers/mutators:
  ROM/SPL/U-Boot/firmware
  secure monitor/hypervisor
  Linux kernel and modules
  userspace overlay manager
```

A tool may be both consumer and producer: U-Boot reads a packaged DTB, mutates it, then produces the kernel handoff.

## Create A Release Baseline

Use exact identities rather than “recent dtc”:

```yaml
baseline_id: axc-family-k6.12-r4
dt_spec_target: "v0.4 plus documented platform constraints"
source:
  linux_commit: 0123456789abcdef0123456789abcdef01234567
tools:
  cpp: "aarch64-linux-gnu-gcc 14.2 preprocessor"
  dtc: "1.7.2 commit ..."
  dtschema: "2025.8"
  libfdt: "1.7.2 commit ..."
build_flags:
  dtc: ["-@", "-Wno-interrupt_provider"]
runtime:
  u_boot: "v2025.07 + product patches 1..6"
  kernel_lines: ["6.6-product", "6.12-product"]
supported_artifacts:
  fdt_versions: [17]
  overlays: true
```

An exception flag is part of the baseline; document its reason, owner, affected sources, and removal criterion.

## Test Syntax, Semantics, And Implementation

Compatibility questions occur at three levels:

1. **Syntax:** can the tool parse or emit the representation?
2. **Semantics:** do producer and consumer agree on property meaning?
3. **Implementation:** does this exact version correctly implement that meaning?

A property can be valid under the specification and schema but ignored by an older driver. A bootloader can parse FDT v17 while lacking overlay fixup support. A newer schema can expose defects that always existed in source; that is not automatically a product regression.

## Maintain A Capability Table

```text
capability: overlay external-symbol resolution
required by: radio-option.dtbo
producer: dtc -@
consumer: U-Boot libfdt overlay apply
minimum tested versions: dtc 1.6.1 / U-Boot product-7.2
negative test: missing __symbols__ rejected before release
fallback: precomposed board DTB
owner: boot-platform
```

Repeat for:

- schema constructs used by bindings
- FDT header/version handling
- phandle and symbol generation
- overlay apply/remove
- FIT packaging and selection
- reserved-memory and `/chosen` fixups
- target kernel binding behavior
- runtime inspection interfaces needed for support

## Upgrade Tools As A Product Change

When upgrading dtc or dt-schema:

1. Pin old and candidate environments.
2. Build the complete declared DT artifact set in both.
3. Categorize new warnings/errors instead of suppressing them globally.
4. Compare exact hashes and decoded semantic output.
5. Validate overlay symbol/fixup sections and composition.
6. Boot representative variants if output or warning policy changed.
7. Update the baseline, provenance, and exception register together.

Possible difference classes:

| Difference | Response |
|---|---|
| newly detected real source defect | fix source and add regression coverage |
| stricter schema reveals undocumented property | decide binding correction or DTS removal |
| serialization-only difference | prove semantic equivalence; update exact hashes/signatures deliberately |
| phandle/order change | inspect overlays and consumers of unstable details |
| tool regression | minimize reproducer, pin old version, report upstream |
| removed local warning suppression | resolve debt or retain narrow documented exception |

## Track Specification Use, Not Only Version

Create a profile of constructs the product actually relies on:

- cell and property encodings
- `/aliases`, `/chosen`, `/reserved-memory`
- interrupt and address translation rules
- generic node names
- phandles and provider specifiers
- overlays and symbols (where used by platform tooling)
- boot-program mutations

This supports focused compatibility tests and avoids treating every specification change as equally relevant.

## Cross-Version Test Directions

At minimum consider:

```text
old supported DTB -> new kernel
new DTB -> oldest supported kernel (if promised)
old bootloader -> new release DTB/DTBO
new bootloader -> old rollback DTB/DTBO
old toolchain -> unchanged source (reproduction baseline)
new toolchain -> unchanged source (upgrade impact)
```

State unsupported directions explicitly. Silence becomes an accidental compatibility promise.

## Baseline Review Checklist

```text
[ ] specification, bindings, tools, bootloader, and kernel identified separately
[ ] exact source and tool versions are immutable
[ ] build flags and exceptions are recorded
[ ] every required capability maps to producer and consumer
[ ] compatibility directions and unsupported combinations are explicit
[ ] tool upgrades rebuild the complete artifact inventory
[ ] semantic and byte differences are classified
[ ] representatives boot when generated artifacts change
[ ] baseline update is tied to release provenance
```

## Further Reading

- [Devicetree Specification](https://devicetree-specification.readthedocs.io/en/stable/)
- [Linux DTS coding style](https://docs.kernel.org/devicetree/bindings/dts-coding-style.html)
- [Writing Devicetree bindings](https://docs.kernel.org/devicetree/bindings/writing-bindings.html)
- [Build Pipeline, Preprocessing, And Artifact Provenance](../build-and-diagnostic-tools/build-pipeline-preprocessing-and-artifact-provenance.md)

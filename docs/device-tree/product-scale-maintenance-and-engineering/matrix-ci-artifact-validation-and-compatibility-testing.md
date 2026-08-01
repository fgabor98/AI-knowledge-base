---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# Matrix CI, Artifact Validation, And Compatibility Testing

Product DT validation spans many dimensions: source layers, boards, revisions, options, overlays, architectures, toolchains, kernels, bootloaders, and release branches. Exhaustive Cartesian testing is usually impossible. Build the full artifact inventory cheaply, then use change impact, risk, and equivalence classes to select deeper jobs.

## Define The Dimensions

```text
products and board revisions
SoC/package/module/carrier combinations
memory and fitted-device variants
base DTBs and allowed ordered overlays
architectures and defconfigs
old/current/candidate dtc and dt-schema
supported kernel lines
supported bootloader/firmware lines
release, recovery, and factory modes
hardware equivalence classes
```

Distinguish supported, build-only, negative, and impossible combinations.

## Generate From A Reviewed Inventory

Example source of truth:

```yaml
variants:
  - id: axc200-revb-base
    dtb: acme/axc200-revb.dtb
    overlays: []
    kernels: [product-6.6, product-6.12]
    bootloaders: [boot-7.2, boot-7.3]
    hardware_class: axc200-revb
  - id: axc200-revc-radio
    dtb: acme/axc200-revc.dtb
    overlays: [acme/radio-v1.dtbo]
    kernels: [product-6.12]
    bootloaders: [boot-7.3]
    hardware_class: axc200-revc-radio
negative_compositions:
  - [acme/axc200-revb.dtb, acme/radio-v2.dtbo]
```

CI should fail if a released artifact, manifest entry, or allowed overlay lacks an inventory record.

## Build A Validation Pyramid

### Layer 1: source and metadata

- YAML/schema syntax and meta-schema
- include/generator determinism
- inventory uniqueness and references
- ownership and patch-ledger completeness
- style/checkpatch rules

### Layer 2: full builds

- all declared DTBs/DTBOs with warning policy
- `dt_binding_check`
- `dtbs_check`
- overlay symbol/fixup presence
- artifact manifest and exact hashes

### Layer 3: semantic assertions

- decoded tree invariants
- expected root compatibles/models
- memory/reserved-memory overlap rules
- critical provider/consumer completeness
- forbidden debug/status/bootargs properties
- semantic diff allowlist by product

### Layer 4: composition and cross-version

- every allowed base/overlay sequence composes
- forbidden combinations fail
- old-DTB/new-kernel and promised reverse directions
- old/new bootloader handoff
- release/recovery artifact compatibility

### Layer 5: virtual boot

- kernel reaches expected milestone
- chosen/console/root identity
- expected devices and absence of critical deferrals
- bootloader selection/fixup tests where sandbox supports them

### Layer 6: physical hardware

- power/reset/clock sequencing
- real interrupts and DMA/IOMMU
- storage/network/update/recovery
- thermal/power safety
- option and revision-specific subsystem function

Cheap layers run broadly; expensive layers are selected intelligently but never replaced by a green schema job.

## Derive Change Impact

Build a dependency graph:

```text
changed schema -> matching compatibles -> DTBs/DTBOs containing them
changed DTSI   -> transitive include consumers
changed label  -> overlays and includes referencing it
changed driver -> compatible/resource consumers
changed tool   -> complete artifact inventory
changed boot fixup -> products using that firmware path
```

Add risk multipliers for memory, DMA/IOMMU, interrupts, regulators, thermal, boot storage, console, watchdog, and update/recovery.

## Always Run Broad Build Coverage

Incremental impact selection can miss dependency-graph errors. On each relevant change:

- build every declared artifact for the affected architecture/family
- validate all relevant schemas
- compose every declared overlay pair/order
- compare the produced artifact set with inventory

Use impact selection primarily for cross-version, virtual, and hardware depth.

## Make Semantic Diffs A Reviewed Artifact

Exact DTB hashes change for meaningful and non-meaningful serialization reasons. Produce both:

```text
exact artifact delta:
  added/removed/changed files, size, sha256

normalized semantic delta per product:
  added/removed nodes
  added/removed/changed properties
  provider reference changes
  status/compatible/reg/interrupt/memory changes highlighted
```

An allowlist should state the expected semantic change and expire with the change, not suppress a path forever.

## Target Checks Precisely

Illustrative commands:

```bash
make dt_binding_check DT_SCHEMA_FILES=/acme,axc-capture/
make ARCH=arm64 dtbs_check DT_SCHEMA_FILES=/acme,axc-capture/
make ARCH=arm64 W=1 acme/axc200-revc.dtb

fdtoverlay \
  -i acme/axc200-revc.dtb \
  -o composed/axc200-revc-radio.dtb \
  acme/radio-v1.dtbo
```

Also schedule periodic complete validation so targeted filters cannot conceal failures outside an author's expected scope.

## Test Negative Policy

CI should prove rejection of:

- unknown board/product identity
- unlisted DTB or overlay
- incompatible base/overlay version
- duplicate/conflicting overlays
- missing required supplier
- forbidden debug device enabled in production
- DTB outside release manifest
- unsupported kernel/DTB or bootloader/overlay combination
- stale security/recovery release tuple

Negative tests prevent a permissive fallback from turning missing metadata into a seemingly successful build.

## Handle Flakes And Lab Scarcity

For every hardware test record:

- board asset and revision
- cabling/instrumentation fixture version
- firmware and release manifest
- power-cycle mechanism
- test attempts and raw logs
- environmental constraints
- owner and quarantine state

Do not automatically retry until green and discard the first failure. Preserve evidence, classify infrastructure versus product behavior, and cap retries.

## CI Result Contract

```yaml
variant: axc200-revc-radio
source_commit: ...
baseline_id: axc-family-k6.12-r4
artifact_manifest_digest: ...
jobs:
  schema: pass
  full_build: pass
  overlay_composition: pass
  semantic_diff: approved-change-174
  old_dtb_new_kernel: pass
  virtual_boot: pass
  hardware_capture_dma: pass
  hardware_thermal: pass
exceptions: []
evidence_uri: immutable://...
```

Green without exact inputs and retained evidence is weak release proof.

## Matrix Review Checklist

```text
[ ] inventory declares every supported and negative composition
[ ] full artifact build is broader than impact-selected hardware tests
[ ] change graph includes transitive DTSI, schema, label, driver, and fixup consumers
[ ] exact and semantic artifact diffs are reviewed
[ ] compatibility directions have explicit expected outcomes
[ ] negative tests reject permissive fallbacks
[ ] virtual tests do not claim physical coverage
[ ] hardware retries preserve first-failure evidence
[ ] periodic complete jobs audit targeted selection
[ ] results bind exact source, tools, manifests, hardware, and exceptions
```

## Further Reading

- [Writing Devicetree bindings in json-schema](https://docs.kernel.org/devicetree/bindings/writing-schema.html)
- [Diagnostic Workflow, Semantic Diffing, And CI Evidence](../build-and-diagnostic-tools/diagnostic-workflow-semantic-diffing-and-ci-evidence.md)
- [Hardware Coverage, Release Qualification, And Learning From Escapes](hardware-coverage-release-qualification-and-learning-from-escapes.md)

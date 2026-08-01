---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# Product-Family Maintenance And Regression Prevention Lab

This lab treats a field regression as a product-system failure, not a one-line DTS exercise. You will reconstruct source inheritance, binding history, patch-stack drift, CI selection, hardware coverage, and release decisions; then design an upstreamable correction and durable prevention controls.

## Scenario

Acme maintains the AXC family:

| Product | Module | Carrier | Board revision | Capture hardware | Kernel |
|---|---|---|---|---|---|
| AXC200 | MX2 revB | CA1 | revB | capture v3, stream `0x31` | 6.6 LTS |
| AXC200 | MX2 revB | CA1 | revC | capture v3, stream `0x47` | 6.6/6.12 |
| AXC210 | MX2 revB | CA2 | revA | no capture fitted | 6.12 |
| AXC220 | MX3 revA | CA2 | revA | capture v4, stream `0x52` | 6.12 |

The source tree grew from AXC200 revB. To add revC quickly, an engineer changed the stream ID in `acme-mx2-common.dtsi`. AXC210 overrides capture to `status = "disabled"`, and AXC200 revB is no longer present in the main lab. AXC220 work later copied the capture node into another file.

A dt-schema tightening patch then marked `acme,stream-id` invalid because the upstream binding expects standard `iommus`. Downstream suppresses the warning globally. The driver still reads `acme,stream-id` first, then `iommus`. The vendor has 27 DT patches over upstream 6.12; nine lack owners, six have no upstream status, and three modify the same binding differently across 6.6 and 6.12 branches.

Release R48 changes the common stream ID to `0x47`, updates dtc/dt-schema, and refactors file ordering. CI builds only AXC200 revC and AXC220. It runs `dtbs_check` with the capture schema excluded and boots one AXC200 revC. AXC200 revB devices receive the release and suffer DMA failure. Rollback succeeds, but support initially cannot identify the DTB because filenames and release manifests omit board revision.

## Learning Objectives

You will:

- map the physical product hierarchy to source layers
- calculate the transitive impact of a common include change
- separate hardware correctness, ABI, style, maintainability, and process findings
- reconcile an undocumented downstream property with upstream binding design
- create a compatible migration from `acme,stream-id` to `iommus`
- design a reviewable upstream/downstream patch sequence
- replace warning suppression with classified validation
- derive an inventory-driven CI and hardware matrix
- write a blameless causal postmortem with verifiable actions

## Evidence A: Source Graph

```text
acme-ax9.dtsi
  <- acme-mx2-common.dtsi
       <- acme-axc200-revb.dts
       <- acme-axc200-revc.dts
       <- acme-axc210-reva.dts

acme-ax10.dtsi
  <- acme-mx3-common.dtsi
       <- acme-axc220-reva.dts
```

### Shared include excerpt before R48

```dts
capture0: capture@4a100000 {
    compatible = "acme,axc-capture-v3";
    reg = <0x0 0x4a100000 0x0 0x10000>;
    interrupts = <GIC_SPI 91 IRQ_TYPE_LEVEL_HIGH>;
    acme,stream-id = <0x31>;

    status = "disabled";
};
```

### R48 common change

```diff
 &capture0 {
-    acme,stream-id = <0x31>;
+    acme,stream-id = <0x47>;
 };
```

### Board fragments

```dts
/* AXC200 revB */
&capture0 {
    status = "okay";
};

/* AXC200 revC */
&capture0 {
    status = "okay";
};

/* AXC210 revA: capture block not routed or populated */
&capture0 {
    status = "disabled";
};
```

## Evidence B: Hardware Record

```text
AXC200 revB schematic: capture master connects IOMMU input 0x31
AXC200 revC ECO-17: capture master rerouted to IOMMU input 0x47
MX2 module: contains SoC and RAM; capture connector/routing is on CA1 carrier
AXC210 CA2 BOM: no capture device fitted; pins repurposed
AXC220: new SoC capture v4 and IOMMU input 0x52
```

## Evidence C: Binding And Driver Drift

### Downstream binding fragment on 6.6

```yaml
properties:
  compatible:
    const: acme,axc-capture-v3

  acme,stream-id:
    $ref: /schemas/types.yaml#/definitions/uint32
```

### Upstream/current design

```yaml
properties:
  compatible:
    enum:
      - acme,axc-capture-v3
      - acme,axc-capture-v4

  iommus:
    maxItems: 1

required:
  - compatible
  - reg
  - interrupts
  - iommus
```

### Driver behavior

```c
if (!of_property_read_u32(np, "acme,stream-id", &stream_id))
        return axc_configure_stream(dev, stream_id);

return axc_configure_standard_iommu(dev);
```

The downstream property shipped in releases R22 through R48 and exists in recovery DTBs embedded in boot firmware.

## Evidence D: CI Configuration

```yaml
dtbs:
  - acme/axc200-revc.dtb
  - acme/axc220-reva.dtb

schema_excludes:
  - acme,axc-capture.yaml

hardware:
  smoke:
    asset: AXC200-C-0481
    assertions:
      - reaches-userspace
      - capture-driver-bound
```

No machine-readable product inventory exists. A release script finds DTBs using a filename glob and packages the newest matching file as `axc200.dtb`.

## Evidence E: Field Incident

```text
release: R48
affected: AXC200 revB, estimated 1,840 units
symptom: capture frames time out; CPU load rises due to retry loop
live compatible: acme,axc200-revb
driver log: configured legacy stream ID 0x47
rollback: R47 successful
detection: customer report after 11 hours
initial support bundle: kernel version and dmesg, no DTB hash or board OTP revision
```

## Part 1: Build The Product And Source Model

Create a table with:

| Hardware fact | Correct layer | Owner | Products inheriting it |
|---|---|---|---|
| AX9 capture block address/IRQ |  |  |  |
| CA1 revB stream routing |  |  |  |
| CA1 revC stream routing |  |  |  |
| AXC210 capture absence |  |  |  |
| AX10 capture v4 block |  |  |  |

Then propose a source layout. Decide whether revB/revC routing belongs in separate board DTS files, carrier-revision includes, or another physical layer. Explain why it must not remain in the MX2 module include.

List every artifact and hardware class transitively affected by the original common-file edit.

## Part 2: Review R48 Across Dimensions

Classify at least two findings in each category:

```text
hardware correctness
binding/ABI
style/source organization
maintainability
integration/release
test evidence
```

For each finding state severity, evidence, requested change, and verification. Do not call the field failure merely “wrong stream ID”; include why the source layer, warning policy, inventory, qualification, and artifact identity allowed it to escape.

## Part 3: Design The Binding Migration

You cannot instantly remove `acme,stream-id` because deployed normal, recovery, and bootloader-embedded DTBs use it.

Define behavior for:

| DTB | Old 6.6 driver | Migrating driver | Final driver during support window |
|---|---|---|---|
| legacy property only |  |  |  |
| `iommus` only |  |  |  |
| both and agree |  |  |  |
| both and disagree |  |  |  |
| neither |  |  |  |

Decide:

- whether current compatibles can safely support both representations
- whether v4 needs a new compatible for hardware reasons independent of property migration
- how schema documents/deprecates the legacy property
- which property wins during transition or whether dual presence is rejected
- how old-DTB/new-kernel compatibility is tested
- whether new-DTB/old-kernel is supported by the product update sequence
- what evidence permits eventual legacy parser removal

## Part 4: Create A Patch And Branch Plan

Build a bisectable plan for:

1. incident fix on supported 6.6 and 6.12 product branches
2. schema migration
3. driver compatibility
4. source-layer correction for revB/revC
5. AXC220 v4 cleanup
6. CI/inventory additions
7. upstream submission
8. later downstream convergence and patch removal

For each patch state target tree/branch, dependency, affected artifacts, validation, upstream status, and drop condition. Keep the urgent property correction separate from file reordering and common-layer refactoring.

## Part 5: Replace The CI Design

Write a variant inventory covering all four table rows in the scenario. Include:

- stable product/revision ID and identity source
- SoC/module/carrier composition
- exact base DTB
- allowed overlays
- supported kernel and bootloader lines
- hardware equivalence class
- required functional tests
- final owner

Create change-impact rules:

```text
acme-ax9.dtsi change -> ?
acme-mx2-common.dtsi change -> ?
CA1 revC layer change -> ?
capture binding change -> ?
capture driver change -> ?
dtc/dt-schema baseline change -> ?
packaging selector change -> ?
```

Design validation layers from schema through physical hardware. The full build must include AXC200 revB even when no lab asset is available. State how missing hardware coverage blocks, waives, or changes release risk.

## Part 6: Define Semantic Assertions

Create machine-checkable assertions such as:

```text
AXC200 revB:
  compatible contains acme,axc200-revb
  capture status is okay
  effective IOMMU stream is 0x31

AXC200 revC:
  compatible contains acme,axc200-revc
  capture status is okay
  effective IOMMU stream is 0x47

AXC210 revA:
  capture device is absent or disabled by defined architecture

AXC220 revA:
  compatible selects capture v4
  effective IOMMU stream is 0x52
```

Add artifact rules so the package name, manifest, board identity, DTB hash, and runtime evidence cannot lose revision identity.

## Part 7: Select Hardware Qualification

Given only three available assets—AXC200 revC, AXC210 revA, and AXC220 revA—decide:

- which tests can run now
- which risks remain unproven for AXC200 revB
- whether revC can represent revB for any checks
- what fixture or asset must be acquired
- what release containment applies until then

Define more than `driver-bound`: exercise DMA data integrity, interrupt counts, IOMMU mapping/fault behavior, stress, error recovery, suspend/resume if supported, and concurrent subsystem load.

## Part 8: Write The Postmortem

Produce:

- impact and affected population
- evidence-backed causal timeline
- technical, introduction, escape, amplification, and recovery causes
- containment and correction
- what worked (rollback)
- at least eight prevention/detection actions
- owner, due release, verification, and effectiveness review for each action
- affected branch/backport/upstream plan

At minimum, actions must cover source layering, product inventory, warning suppression, semantic assertions, hardware coverage, patch ownership, artifact provenance, and support evidence.

## Deliverables

1. physical/source/ownership architecture
2. multidimensional R48 review
3. legacy-property migration matrix
4. bisectable upstream/downstream patch plan
5. variant inventory and impact-derived CI matrix
6. semantic artifact assertions
7. hardware qualification and gap decision
8. complete postmortem and verified action register

## Reference Analysis

The immediate defect is that AXC200 revB receives stream ID `0x47` instead of `0x31`. The source architecture caused it: a carrier-revision routing fact was stored in a module-common file inherited by both revisions. The correct common AX9 layer owns the capture block's address and interrupt; the CA1 board/carrier revision layer owns the IOMMU routing difference.

The legacy property cannot be deleted immediately because shipped DTBs form part of the supported ABI, including recovery and bootloader-embedded artifacts. A migrating driver should continue accepting known legacy trees while adopting the standard `iommus` contract. Dual-property precedence or rejection must be unambiguous, schema-documented, and tested. v4 should receive a distinct compatible only for real programming-model differences, not merely to rename the property.

The escape was systemic: no complete variant inventory, glob-based packaging that erased revision identity, excluded schema, global warning suppression, no revB build or hardware representative, a health test that stopped at driver binding, and an unowned divergent patch stack. Reordering files and upgrading tools in the same release obscured the functional delta.

Containment is to stop R48 for revB, retain the successful rollback, preserve artifacts, and add revision/hash evidence. Correction is a minimal branch fix followed by source-layer repair and migration. Prevention requires inventory-generated artifacts, full-family builds, zero anonymous schema exclusions, semantic stream assertions per revision, a revB hardware gate, owned patch ledger, revision-specific manifests, and support bundles containing OTP identity plus boot/live DT hashes.

## Further Reading

- [Layering, Variants, Ownership, And Source Architecture](layering-variants-ownership-and-source-architecture.md)
- [Binding Evolution, Deprecation, And Migration](binding-evolution-deprecation-and-migration.md)
- [Matrix CI, Artifact Validation, And Compatibility Testing](matrix-ci-artifact-validation-and-compatibility-testing.md)
- [Hardware Coverage, Release Qualification, And Learning From Escapes](hardware-coverage-release-qualification-and-learning-from-escapes.md)

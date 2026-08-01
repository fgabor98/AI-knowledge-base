---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# Binding Evolution, Deprecation, And Migration

Bindings are contracts consumed beyond one kernel tree. Evolve them by adding expressiveness while preserving the meaning of deployed trees. When hardware semantics truly change incompatibly, introduce a new compatible and support an intentional migration window.

## Classify The Proposed Change

### Clarification

Prose or schema is made precise without changing which historical trees are valid or how properties are interpreted.

### Additive extension

A new optional property or compatible describes previously unexpressed hardware. Old DTBs keep old behavior when the property is absent.

### Constraint correction

Schema is tightened or relaxed to match the binding's intended historical contract. This requires checking deployed DTS, other operating systems, and boot firmware—not merely current mainline files.

### Deprecation

An old representation remains accepted for compatibility but new source should use a replacement.

### Incompatible semantic change

Existing syntax would acquire a different meaning or an old valid tree would no longer work. Usually requires a new compatible and driver support for both contracts.

## Preserve Missing-Property Behavior

Suppose an old binding implied one DMA channel and new hardware supports two. Safe evolution may be:

```text
old DTB: no dma-names -> driver retains legacy single-channel behavior
new DTB: dma-names = "rx", "tx" -> new compatible/defined behavior
```

Unsafe evolution makes a new property required for an old compatible and then fails probe for every deployed old DTB.

## Choose A New Compatible When Meaning Changes

Use a new specific compatible when:

- register programming model differs
- property interpretation would differ
- required resources or sequencing change incompatibly
- a hardware revision cannot safely use the old behavior
- fallback is not truly programming-compatible

Example:

```dts
compatible = "acme,axc-capture-v4", "acme,axc-capture-v3";
```

Include the fallback only if v4 hardware can operate correctly under the v3 programming contract. A fallback is an executable compatibility claim, not version decoration.

## Model A Migration Window

```text
Phase 0: discover deployed usage and supported version combinations
Phase 1: schema documents new form; driver accepts old + new
Phase 2: new DTS uses new form; old DTBs remain supported
Phase 3: supported release lines and boot firmware adopt compatible consumers
Phase 4: warnings/telemetry identify remaining old source where appropriate
Phase 5: source cleanup after field and branch exit criteria
Phase 6: code removal only if ABI policy truly permits it
```

Source migration and runtime support removal are different decisions. Removing the last old DTS from the repository does not prove no deployed bootloader still supplies it.

## Build A Deployed-Artifact Inventory

Before tightening a schema or removing support, search:

- all maintained upstream/downstream branches
- shipped DTB and DTBO manifests
- bootloader-embedded trees
- recovery images
- factory and service media
- hypervisor/firmware-provided trees
- third-party boards using the binding
- overlay repositories and customer extensions

Exact artifact hashes and release-set records from the security lifecycle chapter make this tractable.

## Use Schema Deprecation Carefully

JSON Schema can mark a property or compatible deprecated, but operational policy must define what that means:

```yaml
properties:
  acme,legacy-threshold:
    $ref: /schemas/types.yaml#/definitions/uint32
    deprecated: true
    description: Use the standard threshold-microvolt property for new designs.
```

Deprecation should not make known old DTBs fail validation unexpectedly unless the project intentionally changes its validation baseline. Keep historical validation, new-source policy, and runtime compatibility distinct.

## Migrate Property Encodings Without Ambiguity

Avoid supporting two properties with unclear precedence forever. Define:

- which compatibles permit each form
- whether both may coexist
- precedence if coexistence is temporarily allowed
- how disagreement is handled
- old-driver behavior with new DTB
- new-driver behavior with old DTB
- removal criteria for transitional parsing

Often a new compatible plus one unambiguous encoding is safer than probing for multiple interpretations.

## Coordinate Across Components

A migration plan covers:

| Component | Required change |
|---|---|
| binding schema | documents both contracts and selection |
| driver(s) | accepts deployed old form and new form |
| DTS/DTSI | migrates only after consumer support exists |
| bootloader | fixups/embedded DTs understand or preserve form |
| overlays | declare base/binding compatibility |
| release manifest | records artifact/compatibility versions |
| CI | tests old/new directions and rejects ambiguous combinations |
| support tooling | decodes both during field window |

Partial migrations cause failures far from the patch that introduced them.

## Test The Compatibility Quadrants

```text
old DTB + old kernel  -> baseline
old DTB + new kernel  -> normally required stable direction
new DTB + new kernel  -> target
new DTB + old kernel  -> required only if product update policy promises it
```

Add old/new bootloader, overlays, and firmware when they participate. Record explicit expected results, including intentional rejection.

## Handle Existing Invalid Trees

Sometimes deployed trees violate the intended binding. Options include:

- driver quirk scoped to a specific compatible/product
- schema allowance documenting real ABI
- bootloader fixup under controlled product policy
- coordinated artifact replacement where updates are atomic
- rejection if safety/security requires it, with recovery plan

Do not silently redefine a general property to legitimize one vendor mistake. Bound compatibility workarounds narrowly and document their exit conditions.

## Deprecation Register

```yaml
contract: acme,legacy-threshold
replacement: threshold-microvolt
affected_compatibles: [acme,axc-sensor-v1]
first_deprecated_release: product-42
runtime_support: required-through-product-55
source_policy: forbidden-for-new-boards
known_artifact_count: 1842
owners: [sensor-subsystem, product-platform]
tests:
  - old-dtb-new-kernel
  - dual-property-conflict-rejected
removal_gate:
  - all supported products past release 55
  - recovery/factory images updated
  - field inventory shows no required artifacts
```

## Migration Review Checklist

```text
[ ] change class is explicit
[ ] historical contract and deployed artifacts were researched
[ ] property meaning is never silently repurposed
[ ] fallback compatible is justified by real compatibility
[ ] old DTB behavior under new driver is defined
[ ] new DTB behavior under old driver is promised or rejected explicitly
[ ] schema, driver, DTS, bootloader, overlay, and tooling order is planned
[ ] deprecation warning does not accidentally break release validation
[ ] source cleanup is separate from runtime-support removal
[ ] removal has measurable field and release exit criteria
```

## Further Reading

- [Linux Devicetree ABI guidance](https://docs.kernel.org/devicetree/bindings/ABI.html)
- [Writing Devicetree bindings](https://docs.kernel.org/devicetree/bindings/writing-bindings.html)
- [Binding Design And Stable ABI](../binding-design-and-stable-abi.md)
- [Matrix CI, Artifact Validation, And Compatibility Testing](matrix-ci-artifact-validation-and-compatibility-testing.md)

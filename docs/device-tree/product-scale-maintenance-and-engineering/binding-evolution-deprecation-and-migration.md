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

Use a new specific compatible when programming model, property interpretation, or required resources change incompatibly ([Compatible Evolution And Stable ABI Decisions](../driver-matching/compatible-evolution-and-stable-abi-decisions.md)). Include a fallback such as `"ti,am64-pcie-host", "ti,j721e-pcie-host"` only if the newer hardware operates correctly under the older contract.

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

A real family-wide example finished in Linux v6.19. TI's CPSW MACs always add the RGMII TX delay, so `phy-mode = "rgmii-rxid"` never described the board correctly. The migration went in order: the binding example changed to `rgmii-id`; `am65-cpsw-nuss` learned to accept `rgmii-id` and pass `rgmii-rxid` to the PHY (“fixup PHY mode for fixed RGMII TX delay”); `phy-gmii-sel` began always writing the RGMII ID setting; and finally commit `1446fc4dc072` changed 33 DTS and overlay files across AM62, AM64, AM65, J7, and third-party boards. It carries `Tested-by` lines from TQ-Systems and Toradex, owners of downstream products in the blast radius. The driver still accepts `rgmii-rxid`, with a warning, because old DTBs exist.

## Build A Deployed-Artifact Inventory

Before tightening a schema or removing support, inventory the [compatibility population](../binding-design-and-stable-abi/backward-compatibility-deprecation-and-migration.md#define-the-compatibility-population) at product scale: maintained branches, shipped DTB/DTBO manifests, bootloader-embedded trees, recovery, factory, and service media, firmware-provided trees, third-party boards, and overlay repositories. Exact artifact hashes and release-set records from the security lifecycle chapter make this tractable.

## Use Schema Deprecation Carefully

JSON Schema can mark a property or compatible deprecated, but operational policy must define what that means:

From `gpu/img,powervr-rogue.yaml`:

```yaml
# This legacy combination of compatible strings was introduced early on
# before the more specific GPU identifiers were used.
- items:
    - enum:
        - ti,am62-gpu
    - const: img,img-axe
  deprecated: true
```

With `dt-schema` 2026.9, a DTB using the deprecated form still validates without any warning, so deprecation is visible only to people reading the schema.

Deprecation should not make known old DTBs fail validation unexpectedly unless the project intentionally changes its validation baseline. Keep historical validation, new-source policy, and runtime compatibility distinct.

## Migrate Property Encodings Without Ambiguity

Avoid supporting two properties with unclear precedence forever. Define the [dual-property precedence](../binding-design-and-stable-abi/backward-compatibility-deprecation-and-migration.md#dual-property-precedence), which compatibles permit each form, and removal criteria for transitional parsing. Often a new compatible plus one unambiguous encoding is safer than probing for multiple interpretations.

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

Test the old/new DTB × old/new kernel quadrants ([Compatibility In Both Directions](../driver-matching/compatible-evolution-and-stable-abi-decisions.md#compatibility-in-both-directions)), adding old/new bootloader, overlays, and firmware when they participate. Old DTB + new kernel is normally required; new DTB + old kernel only if the product update policy promises it. Record explicit expected results, including intentional rejection.

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
contract: '"ti,am62-gpu", "img,img-axe"'
replacement: '"ti,am62-gpu", "img,img-axe-1-16m", "img,img-axe", "img,img-rogue"'
affected_compatibles: [ti,am62-gpu]
first_deprecated_release: linux-6.16
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

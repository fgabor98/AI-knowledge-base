---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# Validation, Security, And Product Architecture

An isolated DTBO lacks the complete target context needed for many checks. Product validation must select, authenticate, apply, validate, and exercise every supported final composition. It must also decide whether overlays are the right architecture at all.

## Validation Ladder

```text
overlay source syntax
  -> DTBO structure and fixup metadata
base symbol/target compatibility
  -> resolver/application success
merged DTB structural checks
  -> binding/schema validation
product-wide resource conflict checks
  -> boot/device/probe validation
power, error, suspend, and removal lifecycle tests
```

Passing one level does not imply the next.

## Compile And Inspect

```bash
dtc -@ -I dts -O dtb -o module.dtbo module.dtso
fdtdump module.dtbo
dtc -I dtb -O dts -o module.decoded.dts module.dtbo
```

Inspect:

- fragment targets
- external symbol names under `__fixups__`
- local fixup coverage
- introduced compatible strings
- resource specifiers and names
- accidental exported labels

Treat `dtc` warnings as defects to classify, not decoration to suppress.

## Apply On The Host

```bash
fdtoverlay -i base.dtb -o merged.dtb \
  revision.dtbo module-power.dtbo module-sensor.dtbo

dtc -I dtb -O dts -o merged.dts merged.dtb
```

Use the canonical product order. Apply to a new output, retain the pristine base, and fail the job on any error.

Create checkpoints after each overlay in development to find the first unexpected semantic delta. The single final invocation proves only the combined order.

## Validate The Merged Tree

Run the project's current dt-schema workflow against the merged artifact or an equivalent composed DTS/DTB fixture. Validate the same bindings and tool versions used for platform DTBs.

Check:

- every introduced compatible has a valid binding
- variant-required resources exist
- no undocumented properties remain
- graph endpoints and bus children are valid
- provider specifiers match provider cell contracts
- address and size encodings use the merged parent context

An overlay can be impossible to validate fully in isolation because its target supplies parent cells, common properties, and provider nodes. The final tree is authoritative.

## Product Conflict Checks

Schema cannot express every cross-tree constraint. Add checks for:

```text
pin/GPIO exclusive ownership
bus address and chip-select uniqueness
regulator electrical compatibility
clock exclusivity and supported rates
MMIO/reserved-memory overlap
DMA/IOMMU identifier ownership
connector/lane routing
aliases and boot console uniqueness
secure/normal-world access boundaries
wakeup and suspend-state compatibility
```

Derive these from schematics and product manifests, not only DT property names.

## Validate Rejections

CI should reject intentionally:

- overlay for wrong base family/version
- missing exported target
- wrong provider cell contract
- missing prerequisite overlay
- dependency cycle
- conflicting module pair
- duplicate overlay ID
- wrong application order
- invalid signature/hash
- unsupported kernel or bootloader version
- removal of a boot-only overlay

Safe refusal is a tested feature.

## Trust Boundary

An overlay can alter:

- which MMIO devices Linux accesses
- DMA masters and IOMMU attachments
- reserved-memory exposure
- interrupt routing
- regulator voltages and enablement
- clocks, resets, and power domains
- console and boot-related properties
- device compatibles and driver binding

Treat DTBOs and selection manifests as privileged boot/runtime inputs. Authenticate both content and ordering before application. Do not allow an unauthenticated module EEPROM to select arbitrary overlay paths or values.

If a verified base is mutated by an unverified overlay, the final tree is not protected by verification of the base alone.

## Measurement And Provenance

Record or measure:

- authenticated base DTB ID/hash
- ordered overlay stable IDs, versions, and hashes
- compatibility/conflict manifest version
- application engine and version
- each application return status
- final merged hardware-tree hash
- later boot fixups separately

Measuring only the unordered set loses semantics because order can change the result.

## Overlays Versus Separate Base DTBs

Prefer overlays when:

- hardware is genuinely optional/composable
- the attachment interface is stable and bounded
- selection is reliable and authenticated
- combinations are few enough to validate
- the base exports an intentional overlay ABI
- lifecycle is boot-only or genuinely hot-removable end to end

Prefer separate complete DTBs when:

- variants are fixed products or board revisions
- many overlays rewrite the same core nodes
- composition order acts as hidden policy
- labels/paths cannot remain stable
- combinations grow faster than test coverage
- secure, memory, or power topology differs substantially
- runtime removal is not supported

Prefer ordinary `.dtsi` source layering when variants are known at build time and do not require independently distributed binary composition.

## Complexity Budget

Track:

```text
number of bases
number of overlays
supported combinations
ordered dependencies
conflict edges
public labels/paths
independently updated artifacts
runtime-removable stacks
required hardware test cases
```

Set product limits. When adding one overlay multiplies release combinations and ownership exceptions, convert canonical assemblies into full DTBs.

## Release Gate

- [ ] overlay source and DTBO metadata inspected
- [ ] every supported base/overlay order merges from pristine inputs
- [ ] merged trees pass structural and schema validation
- [ ] product resource checks pass
- [ ] unsupported compositions reject before mutation
- [ ] selection manifest, overlays, and order are authenticated
- [ ] final composition is measured/logged reproducibly
- [ ] bootloader and Linux results agree where both apply overlays
- [ ] normal, error, suspend, recovery, and removal paths are tested
- [ ] boot-only overlays cannot be removed through exposed interfaces
- [ ] rollback selects a compatible base/overlay/kernel set

## Authoritative References

- [Linux Devicetree Overlay Notes](https://docs.kernel.org/devicetree/overlay-notes.html)
- [Linux Devicetree Dynamic Resolver Notes](https://docs.kernel.org/devicetree/dynamic-resolution-notes.html)
- [Linux binding schema guide](https://docs.kernel.org/devicetree/bindings/writing-schema.html)
- [U-Boot Device Tree Overlays](https://docs.u-boot.org/en/latest/usage/fdt_overlays.html)
- [U-Boot FIT overlay usage](https://docs.u-boot.org/en/latest/usage/fit/overlay-fdt-boot.html)

## Continue

Proceed to the [Overlay Composition And Lifecycle Lab](overlay-composition-and-lifecycle-lab.md).

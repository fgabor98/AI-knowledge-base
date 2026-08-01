---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# Binding Schema Authoring And Validation Lab

This lab encodes the AXC100/AXC200 hardware contract from the previous module as a schema, proves that real nodes select it, and diagnoses failures at each validation gate. Complete the design tasks before reading the reference analysis.

## Contract To Encode

The binding design review established:

### Common Contract

- one register region
- two clock inputs ordered and named `bus`, `sample`
- an optional standard graph `ports` structure for the connected sensor
- no custom polling, DMA-buffer, version, fast-mode, or clock-rate properties

### AXC100

- compatible is exactly `acme,axc100`
- one interrupt named `completion`
- no reset input

### AXC200

- compatible is exactly `acme,axc200`
- two interrupts ordered and named `completion`, `error`
- one reset named `core`
- no AXC100 fallback because old software cannot recover safely from DMA faults

The driver reads FIFO depth and silicon revision from hardware. Those are not DT properties.

## Starting Schema

An engineer proposes this file at `Documentation/devicetree/bindings/media/acme,ax-capture.yaml`:

```yaml
# SPDX-License-Identifier: GPL-2.0-only
%YAML 1.2
---
$id: http://devicetree.org/schemas/media/acme,capture.yaml#
$schema: http://json-schema.org/draft-07/schema#

title: Acme Linux capture driver configuration

maintainers:
  - Old Developer <old@example.com>

select: false

properties:
  compatible:
    enum: [acme,axc100, acme,axc200]

  reg: true

  interrupts:
    maxItems: 1

  interrupt-names:
    items:
      enum: [completion, error]

  clocks:
    maxItems: 2

  clock-names:
    items:
      enum: [bus, sample]

  resets:
    maxItems: 1

  acme,dma-buffer-size:
    $ref: /schemas/types.yaml#/definitions/uint32

required:
  - compatible
  - reg

additionalProperties: true

examples:
  - |
    capture@48000000 {
        compatible = "acme,axc200";
        reg = <0x48000000 0x1000>;
        interrupts = <80>;
        clocks = <&ccu 12>;
        acme,dma-buffer-size = <1048576>;
    };
```

Assume the vendor prefix is registered and the maintainer address is obsolete.

## Lab Objectives

Produce:

1. a defect inventory classified by pipeline stage
2. a corrected schema
3. one valid AXC100 example and one valid AXC200 example
4. positive and negative instance fixtures
5. targeted and broad validation commands
6. a diagnostic interpretation worksheet
7. evidence that the binding was selected rather than skipped
8. a submission-readiness checklist

## Task 1: Audit Identity And Selection

Answer:

- Does `$id` mirror the actual path and filename?
- Is `$schema` the DT binding meta-schema?
- Does the title describe hardware?
- Is the license the preferred form for a new binding?
- Can the listed maintainer review changes?
- Will `select: false` ever apply this schema to AXC nodes in platform DTBs?
- Should an explicit selector exist at all?

Classify which problems should fail `dt_binding_check` and which can remain semantically wrong while tools are quiet.

## Task 2: Audit Properties And Required Sets

For every property, write:

```text
hardware meaning:
standard or vendor-specific type owner:
allowed cardinality:
semantic order:
variant conditions:
required/optional/forbidden state:
absence meaning:
```

Pay particular attention to:

- one versus two interrupt entries
- fixed ordering of `interrupt-names` and `clock-names`
- the missing second clock in the example
- AXC200 reset presence and `reset-names`
- AXC100 reset prohibition
- graph child ownership
- removal of the policy property
- top-level closure

## Task 3: Encode Variants

Define broad common limits under top-level `properties`. Then use compatible-specific conditions to narrow AXC100 and AXC200.

Your schema must reject:

- AXC100 with two interrupts
- AXC100 with any reset
- AXC200 with one interrupt
- AXC200 with no reset
- either variant with reversed clock names
- AXC200 with reversed interrupt names
- the old `acme,dma-buffer-size` property
- an unknown property typo
- an invented compatible fallback list

Do not solve variants by copying the whole schema into two large `oneOf` branches unless you can justify the maintenance cost.

## Task 4: Add The Graph

The optional sensor connection uses the standard graph binding:

```dts
ports {
        #address-cells = <1>;
        #size-cells = <0>;

        port@0 {
                reg = <0>;

                capture_in: endpoint {
                        remote-endpoint = <&sensor_out>;
                };
        };
};
```

Reference the standard graph schema rather than reproducing endpoint fields. Decide whether the example should include the graph, and explain how remote labels/providers are handled in an isolated schema example.

## Task 5: Design Examples

Create the smallest useful examples for both variants. Include only headers required by symbolic constants. If you omit the graph from the core example, explain how a separate example or real DTB provides coverage.

For each example, predict:

- preprocessor dependencies
- parent address/size-cell assumptions
- `dtc` warnings that could occur
- schema branch selected
- exact required resources

## Task 6: Run The Validation Ladder

Write commands for:

1. targeted binding and example validation
2. full binding validation
3. affected ARM64 DTB validation against the target schema
4. broader affected DTB validation
5. an expected maintainer warning level

Use `O=build` and record the kernel commit, architecture, configuration, `dtschema`, and `dtc` versions.

## Task 7: Prove Negative Coverage

Create a mutation table:

| Fixture | Mutation | Expected owner/rule |
|---|---|---|
| AXC100 valid | add `resets` | AXC100 conditional forbids property |
| AXC100 valid | reverse clock names | fixed positional `items` |
| AXC200 valid | remove error IRQ | AXC200 `minItems` |
| AXC200 valid | remove `reset-names` | AXC200 `required` |
| AXC200 valid | add `acme,dma-buffer-size` | top-level closure |
| AXC200 valid | use AXC200, AXC100 fallback list | compatible shape |

For one mutation, show how you will prove the reported schema filename and instance path correspond to this binding.

## Task 8: Diagnose Hypothetical Output

Classify and resolve each message:

```text
.../acme,ax-capture.yaml: $id: relative path/filename doesn't match actual path
```

```text
.../capture@48000000: interrupts: [[80]] is too short
  from schema .../acme,ax-capture.yaml
```

```text
.../capture@48000000: 'acme,dma-bufer-size' does not match any allowed property
```

```text
.../capture@48000000: failed to match any schema with compatible: ['acme,axc200']
```

For the last message, do not assume the compatible is undocumented until checking whether the schema failed and was skipped.

## Reference Analysis

### Defect Inventory

The starting proposal has both tool-detectable and semantic problems:

| Defect | Likely gate | Consequence |
|---|---|---|
| `$id` omits `ax-` from filename | meta-schema/path check | wrong global identity/reference base |
| generic JSON Schema `$schema` | DT meta-schema | bypasses/violates DT binding contract |
| single GPL license | review policy | new bindings normally use preferred dual license |
| Linux-driver title | semantic review | documents implementation rather than hardware |
| obsolete maintainer | review/maintenance | no credible owner |
| `select: false` | selection/semantic | platform nodes never use this concrete binding |
| `reg: true` | meta-schema/semantic | no device-specific cardinality |
| one interrupt for all variants | example/DTB validation | AXC200 contract incomplete |
| unordered resource names | semantic/negative tests | reversals and duplicates may pass |
| clocks not required or exact | semantic/DTB validation | incomplete nodes pass |
| reset optional for all variants | semantic/DTB validation | unsafe AXC200 and false AXC100 accepted |
| no `reset-names` | semantic | role is not fixed |
| DMA buffer policy property | ABI review | wrong interface, description missing |
| `additionalProperties: true` | semantic/negative tests | typos and undocumented data pass |
| incomplete AXC200 example | example validation after fixes | canonical example violates contract |

Some meta-schema versions will produce several errors at once. Fix identity and structure first, then re-run before interpreting later messages.

### Corrected Schema

One defensible schema is:

```yaml
# SPDX-License-Identifier: (GPL-2.0-only OR BSD-2-Clause)
%YAML 1.2
---
$id: http://devicetree.org/schemas/media/acme,ax-capture.yaml#
$schema: http://devicetree.org/meta-schemas/core.yaml#

title: Acme AX100 and AX200 capture engines

maintainers:
  - Current Maintainer <maintainer@example.com>

description: |
  AX capture engines receive samples from an external interface and transfer
  them to memory. AX200 adds a DMA-fault interrupt and reset recovery input.

properties:
  compatible:
    enum:
      - acme,axc100
      - acme,axc200

  reg:
    maxItems: 1

  interrupts:
    minItems: 1
    maxItems: 2

  interrupt-names:
    minItems: 1
    maxItems: 2

  clocks:
    minItems: 2
    maxItems: 2

  clock-names:
    items:
      - const: bus
      - const: sample

  resets:
    maxItems: 1

  reset-names:
    items:
      - const: core

  ports:
    $ref: /schemas/graph.yaml#/properties/ports

required:
  - compatible
  - reg
  - interrupts
  - interrupt-names
  - clocks
  - clock-names

allOf:
  - if:
      properties:
        compatible:
          contains:
            const: acme,axc100
    then:
      properties:
        interrupts:
          maxItems: 1
        interrupt-names:
          items:
            - const: completion
        resets: false
        reset-names: false

  - if:
      properties:
        compatible:
          contains:
            const: acme,axc200
    then:
      properties:
        interrupts:
          minItems: 2
        interrupt-names:
          items:
            - const: completion
            - const: error
      required:
        - resets
        - reset-names

additionalProperties: false

examples:
  - |
    capture@48000000 {
        compatible = "acme,axc100";
        reg = <0x48000000 0x1000>;
        interrupts = <80>;
        interrupt-names = "completion";
        clocks = <&capture_bus_clk>, <&capture_sample_clk>;
        clock-names = "bus", "sample";
    };

  - |
    capture@48000000 {
        compatible = "acme,axc200";
        reg = <0x48000000 0x1000>;
        interrupts = <80>, <81>;
        interrupt-names = "completion", "error";
        clocks = <&capture_bus_clk>, <&capture_sample_clk>;
        clock-names = "bus", "sample";
        resets = <&capture_reset 7>;
        reset-names = "core";
    };
```

The exact accepted form must be tested against the current kernel meta-schemas. If the relevant subsystem has an additional common device schema, reference it and revisit top-level closure—composition may require `unevaluatedProperties: false`. The graph `$ref` is attached to the `ports` property itself, so the parent still explicitly owns that property name.

The conditions could include `required: [compatible]` inside `if`, but top-level `required` already guarantees `compatible` exists. Keeping the discriminator explicit can improve clarity in extracted helper conditions.

### Example Strategy

Two examples are justified because the resource contracts differ materially. The examples use numeric interrupts to avoid an interrupt-header dependency and dummy phandle labels supplied by the example harness. If a current kernel's example harness requires explicit provider scaffolding for these references, follow the errors and nearby binding conventions rather than bloating the binding preemptively.

Graph coverage can come from a third minimal example or a real platform DTB. If included in an isolated example, both endpoint labels must resolve, so a small peer node or accepted dummy topology may be necessary. Do not include an entire sensor binding merely to make the example look realistic.

### Validation Commands

```bash
make O=build dt_binding_check \
  DT_SCHEMA_FILES=Documentation/devicetree/bindings/media/acme,ax-capture.yaml

make O=build dt_binding_check

make O=build ARCH=arm64 dtbs_check \
  DT_SCHEMA_FILES=Documentation/devicetree/bindings/media/acme,ax-capture.yaml

make O=build ARCH=arm64 dtbs_check

make O=build ARCH=arm64 W=1 dtbs_check
```

Use the configured cross-compiler/environment required by the tree. A fixed-string directory fragment may be more appropriate during iteration, but record exactly what it selects.

### Diagnostic Interpretation

- `$id` mismatch: binding identity/meta-schema failure; correct the URI to mirror the file path.
- AXC200 interrupt list too short: the binding selected correctly and its AXC200 condition rejected an incomplete instance; fix DTS unless the hardware contract is wrong.
- `acme,dma-bufer-size` rejected: closure caught either a typo or an undocumented policy property; remove it rather than documenting the typo.
- no matching schema: first run targeted `dt_binding_check`. If the schema is invalid or `select: false`, `dtbs_check` can lack the processed binding. Only after proving schema health should you investigate compatible spelling and selection.

### Selection Proof

Use a temporary negative mutation on a representative AXC200 node:

```text
remove reset-names
run targeted dtbs_check
observe instance path /.../capture@48000000
observe schema path .../media/acme,ax-capture.yaml
restore reset-names
rerun and confirm that diagnostic disappears
```

This proves the schema participated. Keep the valid fixture and automate the negative case outside committed production DTS when the project has a suitable test-fixture framework.

## Submission Readiness

Before the binding patch is ready:

- [ ] hardware contract and ABI review are complete
- [ ] `$id`, filename, license, maintainers, and vendor prefix are correct
- [ ] every property has an owner, exact type, cardinality, and absence meaning
- [ ] resource order and parallel names agree
- [ ] variants require and forbid the correct resources
- [ ] schema and every nested object are closed appropriately
- [ ] examples compile and exercise meaningful valid variants
- [ ] targeted and full `dt_binding_check` pass
- [ ] affected DTBs select the schema and pass
- [ ] negative fixtures fail for the intended rules
- [ ] `dtc`/maintainer warning levels add no unexplained diagnostics
- [ ] binding patch precedes its driver and DTS users in the series
- [ ] exact commands, versions, hardware tests, and baseline warnings are reported

## Completion Criteria

You have completed the lab when you can explain not only why the corrected schema passes, but also why each invalid mutation fails, which schema branch reports it, and how you know the binding was not skipped.

## Authoritative References

- [Linux schema-writing guide](https://docs.kernel.org/devicetree/bindings/writing-schema.html)
- [Linux annotated example schema](https://github.com/torvalds/linux/blob/master/Documentation/devicetree/bindings/example-schema.yaml)
- [`dt-schema` project](https://github.com/devicetree-org/dt-schema)
- [Linux binding patch submission guide](https://docs.kernel.org/devicetree/bindings/submitting-patches.html)

## Next Step

Continue with [Overlays In Depth](../overlays-in-depth.md), applying the same schema-selection and final-tree validation discipline to composed base DTBs and DTBOs.

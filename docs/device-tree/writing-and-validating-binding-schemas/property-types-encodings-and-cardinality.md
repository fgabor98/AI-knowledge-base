---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# Property Types, Encodings, And Cardinality

Schema validation does not operate on DTS tokens exactly as they appear in source. `dtc` and `dt-schema` expose a normalized data shape in which strings are arrays and numeric cell groups are matrices. Correct constraints require understanding both the source encoding and processed shape.

## Reuse Common Property Types

Standard properties such as `reg`, `interrupts`, `clocks`, `resets`, and `status` already have type definitions. A device binding usually adds only device-specific constraints:

```yaml
properties:
  reg:
    maxItems: 1

  interrupts:
    minItems: 1
    maxItems: 2

  clocks:
    minItems: 2
    maxItems: 2
```

Do not repeat their generic type definitions. Redefinition can conflict with common schemas and hides which semantics are shared.

Vendor-specific properties normally reference a DT type from `/schemas/types.yaml` unless they are boolean:

```yaml
properties:
  acme,burst-length:
    $ref: /schemas/types.yaml#/definitions/uint32
    enum: [4, 8, 16]
    description: Number of samples transferred in one fixed hardware burst.
```

## DTS And Validation Shapes

Conceptually:

```dts
label = "capture";
rates = <100 200>;
matrix = <1 2>, <3 4>;
```

is normalized into array-oriented data roughly like:

```text
label  -> ["capture"]
rates  -> [[100, 200]]
matrix -> [[1, 2], [3, 4]]
```

DT schema transformations let authors use compact idioms for common single values and fixed tuples, but diagnostics often reveal the processed array/matrix model. When an error says an array is too short or an item is itself an array, trace it back to cell grouping in DTS.

## Boolean Properties

```yaml
properties:
  acme,external-trigger:
    type: boolean
    description: The external trigger input is physically connected.
```

In DTS, presence means true:

```dts
acme,external-trigger;
```

There is no `= <0>` false form for a boolean binding. Absence must have a defined meaning. Do not add a boolean to select a software implementation path.

## Strings And String Arrays

One exact string:

```yaml
mode:
  $ref: /schemas/types.yaml#/definitions/string
  const: parallel
```

A finite vocabulary:

```yaml
mode:
  $ref: /schemas/types.yaml#/definitions/string
  enum: [parallel, serial]
```

An ordered string list:

```yaml
clock-names:
  items:
    - const: bus
    - const: sample
```

The explicit `items` list fixes order and, through DT schema processing, normally fixes cardinality. Use `minItems`/`maxItems` where a range is intentional. Avoid a free-form `type: string` when the hardware vocabulary is closed.

## Scalars And Ranges

```yaml
acme,burst-length:
  $ref: /schemas/types.yaml#/definitions/uint32
  minimum: 1
  maximum: 16
```

Use `enum` for sparse valid values and `minimum`/`maximum` for a continuous range. Add `multipleOf` only when the hardware truly requires a step. State inclusive boundaries and unit semantics in the property description.

For properties with recognized unit suffixes, common schemas may already provide type and unit expectations. Constrain the valid device-specific range without inventing another scale.

## Fixed Tuples

Use positional items when each cell has a different meaning:

```yaml
acme,dma-window:
  $ref: /schemas/types.yaml#/definitions/uint32-array
  items:
    - description: First channel index
    - description: Number of consecutive channels
```

Before defining such a tuple, ask whether it should be a phandle-array, `reg`, `dma-ranges`, or another existing structure. An opaque vendor cell tuple is hard to extend.

## Resource Lists And Parallel Names

Keep resource and name cardinality aligned:

```yaml
properties:
  interrupts:
    minItems: 2
    maxItems: 2

  interrupt-names:
    items:
      - const: completion
      - const: error
```

Names describe roles without suffixes such as `-irq` or `-clk`. If variants have different list lengths, give broad limits at the top level and narrow both lists in the same compatible-specific condition.

## Phandles And Phandle Arrays

A single reference and a provider specifier are different shapes:

- phandle: reference to one node
- phandle-array: one or more entries, each containing a provider phandle plus provider-defined argument cells

Prefer existing property definitions. For custom relationships, define why a standard provider class does not apply and reference the correct type:

```yaml
acme,peer:
  $ref: /schemas/types.yaml#/definitions/phandle
  description: Reference to the physically paired capture engine.
```

The consumer schema usually constrains entry count; the provider binding defines `#*-cells` and specifier meaning.

## Byte Arrays

Use byte-array definitions for actual byte sequences such as hardware identifiers or calibration blobs, not for numeric values that happen to fit in bytes. Constrain exact or bounded length:

```yaml
acme,calibration-id:
  $ref: /schemas/types.yaml#/definitions/uint8-array
  minItems: 8
  maxItems: 8
  description: Factory-programmed public calibration identifier.
```

Do not place large opaque configuration blobs in DT merely because a schema can bound their size.

## Compatible Lists

One identity:

```yaml
compatible:
  const: acme,axc100
```

Several independent identities:

```yaml
compatible:
  enum:
    - acme,axc100
    - acme,axc200
```

Specific fallback sequences:

```yaml
compatible:
  oneOf:
    - const: acme,axc100
    - items:
        - const: acme,axc110
        - const: acme,axc100
```

This accepts exactly the documented order rather than any permutation of known strings.

## Cardinality Audit

For every array-like property, answer:

- exact, minimum, and maximum number of entries
- semantic order
- whether duplicates are meaningful
- relationship to a parallel `*-names` property
- variant-specific narrowing
- meaning of absence

Unbounded arrays and unconstrained strings are usually incomplete ABI definitions.

## Authoritative References

- [Linux schema-writing guide: property schemas](https://docs.kernel.org/devicetree/bindings/writing-schema.html)
- [`dt-schema` type definitions](https://github.com/devicetree-org/dt-schema/blob/main/dtschema/schemas/types.yaml)
- [`dt-schema` property unit schemas](https://github.com/devicetree-org/dt-schema/blob/main/dtschema/schemas/property-units.yaml)

## Continue

Proceed to [References, Composition, Conditionals, And Closure](references-composition-conditionals-and-closure.md).

---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# Child Nodes, Name Patterns, And Common Schemas

Child nodes are object-valued properties in the schema data model. Their names, contents, required fields, and ownership all need constraints. A parent-level `additionalProperties: false` does not automatically close each nested object.

## Fixed-Name Child Nodes

Use `properties` when a child has one exact semantic name:

```yaml
properties:
  control:
    type: object
    additionalProperties: false

    properties:
      acme,timeout-us:
        minimum: 1
        maximum: 1000

    required:
      - acme,timeout-us
```

This is appropriate only when `control` is a binding-defined child node representing a real substructure. Do not create configuration containers merely to group unrelated driver parameters.

## Unit-Addressed Or Repeated Children

Use `patternProperties` for names that follow a defined regex:

```yaml
patternProperties:
  "^channel@[0-3]$":
    type: object
    additionalProperties: false

    properties:
      reg:
        maximum: 3

      acme,input:
        $ref: /schemas/types.yaml#/definitions/uint32
        maximum: 7

    required:
      - reg
      - acme,input
```

Anchor patterns with `^` and `$` unless substring matching is intentional. JSON Schema regex matching is not implicitly anchored. A pattern such as `channel@` can match `not-a-channel@garbage`.

## Pattern The Name, Constrain The Address

A regex constrains spelling; the child's `reg` constrains its encoded address/identifier. Both matter.

```yaml
"^channel@[0-9a-f]+$":
  type: object
  properties:
    reg:
      minimum: 0
      maximum: 3
  required:
    - reg
  additionalProperties: false
```

Do not try to prove with regex alone that the unit address equals `reg`; generic DT checks and bus schemas participate in unit-address consistency. Keep the regex broad enough for canonical hexadecimal spelling and the property constraint exact.

## Bound Child Count

When hardware has a maximum number of children, constrain it. Depending on schema shape, use `minProperties`/`maxProperties` carefully because standard properties such as `#address-cells` also count as object properties. Often a dedicated container makes counting clearer:

```yaml
properties:
  channels:
    type: object
    additionalProperties: false
    minProperties: 1
    maxProperties: 4

    patternProperties:
      "^channel@[0-3]$":
        $ref: "#/$defs/channel"
```

If the container has its own fixed properties, account for them rather than treating `maxProperties` as synonymous with number of children.

## Local Definitions

Use `$defs` for a repeated structure within one binding:

```yaml
$defs:
  channel:
    type: object
    additionalProperties: false
    properties:
      reg:
        maximum: 3
      label:
        $ref: /schemas/types.yaml#/definitions/string
    required:
      - reg

patternProperties:
  "^channel@[0-3]$":
    $ref: "#/$defs/channel"
```

Use an external common schema when multiple bindings share a genuine hardware-class contract. Do not extract a helper merely to reduce a few lines if its semantics have no independent meaning.

## Child With Its Own Compatible

Sometimes a child is an independently bound device. The parent may constrain only its identity while its own selected schema validates the rest:

```yaml
properties:
  codec:
    type: object
    additionalProperties: true
    properties:
      compatible:
        const: acme,ax-codec
    required:
      - compatible
```

`additionalProperties: true` is intentional here because another schema selected by `acme,ax-codec` owns the child's complete property set. Verify that the child schema really is selected; otherwise this becomes an unvalidated hole.

If the parent also owns integration-specific child properties, decide whether they belong in the child binding, a referenced common schema, or the parent. Two schemas must not silently give conflicting meanings to the same field.

## Standard Child Structures

Prefer standard schemas for established structures:

- graph `ports`, `port`, and `endpoint`
- GPIO, interrupt, clock, reset, and power controllers
- I2C/SPI bus children
- fixed partitions
- reserved-memory children
- connector and PHY graph nodes

Reference the owning schema and add only device-specific bounds. Copying standard graph or bus properties into a vendor schema creates incomplete local versions.

## Bus Children Versus Functional Children

A controller can both have registers of its own and enumerate child devices on a bus. In that case:

- the controller binding references the bus-controller schema
- child name patterns follow bus conventions
- child devices are validated by their own compatible schemas
- controller-specific child constraints remain narrow
- closure must allow the properties evaluated by the referenced bus schema

Do not validate arbitrary bus children by listing every possible child compatible in the controller binding.

## `properties` And `patternProperties` Can Both Apply

If a name matches both an exact property and a regex, both schemas apply. This can be useful, but overlapping patterns often cause accidental contradictions.

Audit each possible child name against every pattern. Prefer mutually clear patterns and place broad common behavior in a shared definition rather than stacking several near-duplicate regexes.

## Forbid Unknown Children

Close each nested object:

```yaml
additionalProperties: false
```

or, when composed with referenced schemas:

```yaml
unevaluatedProperties: false
```

Without nested closure, a typo such as `channe1@0` or an undocumented child property may pass even though the parent itself is closed.

## Child-Node Review Checklist

- Does every child represent hardware or an established standard structure?
- Is the name exact or regex-bound and anchored?
- Are unit address and `reg` semantics constrained by the correct bus model?
- Is the number of children physically bounded?
- Is every nested object closed at its own level?
- If another compatible schema owns a child, was that selection proven?
- Are standard graph, bus, partition, or provider schemas reused?
- Can two patterns match the same name and conflict?
- Are required child properties distinct from merely permitted properties?

## Authoritative References

- [Linux schema-writing guide: child nodes and pattern properties](https://docs.kernel.org/devicetree/bindings/writing-schema.html)
- [`dt-schema` common schemas](https://github.com/devicetree-org/dt-schema/tree/main/dtschema/schemas)
- [Devicetree Specification: node names](https://devicetree-specification.readthedocs.io/en/stable/devicetree-basics.html)

## Continue

Proceed to [Examples, Vendor Bindings, And Authoring Workflow](examples-vendor-bindings-and-authoring-workflow.md).

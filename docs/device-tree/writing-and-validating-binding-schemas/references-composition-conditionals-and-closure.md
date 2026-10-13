---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# References, Composition, Conditionals, And Closure

Real bindings reuse bus, class, graph, and common-device schemas. JSON Schema composition combines constraints; it does not copy and paste property definitions. Closure must therefore account for properties evaluated by every participating schema.

## `$ref` Reuses One Schema

```yaml
allOf:
  - $ref: /schemas/media/video-interface-devices.yaml#
```

A leading `/schemas/` reference resolves from the Devicetree schema root. A relative reference resolves against the current document's `$id`. Fragments after `#` select definitions inside a document.

Use references to share established semantics. Do not reproduce a common schema locally and then let the copies diverge.

## `allOf` Means Every Constraint Applies

From `gpu/img,powervr-rogue.yaml` (trimmed):

```yaml
allOf:
  - if:
      properties:
        compatible:
          contains:
            enum:
              - ti,am62-gpu
              - ti,j721s2-gpu
    then:
      properties:
        clocks:
          maxItems: 1

  - if:
      properties:
        compatible:
          contains:
            const: img,img-axe-1-16m
    then:
      properties:
        power-domains:
          maxItems: 1
        power-domain-names:
          maxItems: 1
      required:
        - power-domains
        - power-domain-names
```

The AM62x GPU node contains both `ti,am62-gpu` and `img,img-axe-1-16m`, so both branches apply to it at once.

The instance must satisfy both branches and the top-level schema. `allOf` is intersection, not ordered inheritance; later branches do not override earlier constraints.

Contradictory branches produce an empty accepted set. When a property is fixed differently by two schemas, changing order cannot resolve it.

## `oneOf`, `anyOf`, And `allOf`

| Keyword | Meaning | Typical DT use |
|---|---|---|
| `allOf` | all branches must validate | common schema plus device constraints; multiple independent conditions |
| `oneOf` | exactly one branch validates | distinct compatible-list shapes or mutually exclusive encodings |
| `anyOf` | one or more branches validate | alternatives that may overlap legitimately |

`oneOf` fails when zero branches match and also when multiple branches match. Branches should be distinguishable. If two permissive branches accept the same node, a valid-looking instance can fail unexpectedly.

Do not use `oneOf` merely to organize text. Use it when exclusivity is part of the contract.

## Conditionals Narrow Top-Level Definitions

Define a property's broadest permitted shape in `properties`, then narrow it in conditions. The same PowerVR binding declares the widest clock list once:

```yaml
properties:
  clocks:
    minItems: 1
    maxItems: 3

  clock-names:
    items:
      - const: core
      - const: mem
      - const: sys
    minItems: 1
```

The branches above then cut it to one clock for TI integrations, while another branch requires all three for the Renesas and T-Head GPUs. Removing `power-domain-names` from the AM625 SK DTB produces `gpu@fd00000 (ti,am62-gpu): 'power-domain-names' is a required property`, and adding a second clock produces `clocks: [[2, 187, 0], [2, 187, 1]] is too long`.

Do not introduce an otherwise unknown property only inside `then`; the top-level closure may reject it, and the overall schema becomes harder to understand. Declare the broad property once and specialize it.

## Conditions Need Presence Guards

A condition written only with `properties` can be true when the property is absent, because `properties` alone does not require presence. When presence is the trigger, guard it with `required`. `mfd/ti,tps6594.yaml` uses this to keep multiphase buck combinations exclusive:

```yaml
allOf:
  - if:
      required:
        - buck12
    then:
      properties:
        buck123: false
        buck1234: false
  - if:
      required:
        - buck123
    then:
      properties:
        buck34: false
```

If a board declares a dual-phase `buck12` regulator node, the schema forbids `buck123` and `buck1234` nodes, because the same physical phases cannot be in two groups.

For compatible conditions, `compatible` is normally top-level required, but including an explicit presence guard can still make a standalone conditional clearer when context is uncertain.

## `else` Can Forbid Variant-Only Data

`thermal/ti,j72xx-thermal.yaml` uses `else` to give the two VTM generations different register-window counts:

```yaml
allOf:
  - if:
      properties:
        compatible:
          contains:
            const: ti,j721e-vtm
    then:
      properties:
        reg:
          minItems: 3
    else:
      properties:
        reg:
          maxItems: 2
```

Without the `else`, a `ti,j7200-vtm` node (used by AM62x) could carry a third window that its hardware does not have. The DSS binding uses the other form: a `then` that sets `oldi-transmitters: false` for `ti,am62a7-dss`, whose SoC has no OLDI. Decide whether variant-only data is legitimate optional wiring or a contradiction, then encode the decision.

## Dependencies Express Co-Presence

Some properties form pairs or groups independent of a variant. Examples include data plus names, or a mode marker plus its associated resource. Use the dependency vocabulary supported by current DT meta-schemas and neighboring bindings.

From `regulator/ti,tps65132.yaml` and `display/bridge/ti,sn65dsi86.yaml`:

```yaml
dependencies:
  active-discharge-gpios: [ 'ti,active-discharge-time-us' ]
```

```yaml
dependencies:
  lane-polarities: [data-lanes]
```

Both are one-way: a discharge GPIO needs its timing, and lane polarities make sense only after lanes are mapped, but either right-hand property can appear alone. The two directions mean both are present or both absent. A one-way dependency means only that one property requires the other. Use conditionals instead when value-specific narrowing is needed.

## Close Every Object

At a schema object boundary, choose one ownership rule:

```yaml
additionalProperties: false
```

Use this common case when the current schema owns the complete set and does not need properties admitted by referenced schemas.

```yaml
unevaluatedProperties: false
```

Use this when referenced schemas under composition evaluate properties that must remain allowed. It closes whatever none of the participating schemas evaluated.

```yaml
additionalProperties: true
```

This is rare at a concrete device top level. It is appropriate for reusable common schemas intended to be referenced, or certain child nodes whose independently selected compatible schema owns the remaining properties.

## Closure Is Not Style

Choosing the wrong keyword causes two opposite failures:

- too open: undocumented properties and typos pass
- too closed: standard properties from a referenced schema are rejected

Do not switch blindly from `additionalProperties` to `unevaluatedProperties` to silence an error. Identify which schema is supposed to evaluate the reported property and whether that schema is actually referenced.

## Split When Conditions Become A Program

One schema may cover close variants. Split bindings when:

- conditionals repeat most properties
- modes have different child topology
- compatible branches barely share a contract
- review requires simulating deeply nested `oneOf`/`allOf` logic
- common and concrete schemas have confused closure ownership

A small shared helper schema plus clear concrete bindings can be more maintainable than one universal schema.

## Composition Review Method

For each representative variant, list:

1. schemas selected directly
2. schemas referenced by `allOf` or property `$ref`
3. applicable conditional branches
4. properties each schema evaluates
5. final required properties
6. final forbidden properties
7. closure keyword and owner

This turns an abstract composition error into a deterministic evaluation trace.

## Authoritative References

- [Linux schema-writing guide](https://docs.kernel.org/devicetree/bindings/writing-schema.html)
- [JSON Schema: schema composition](https://json-schema.org/understanding-json-schema/reference/combining)
- [`dt-schema` core schemas](https://github.com/devicetree-org/dt-schema/tree/main/dtschema/schemas)

## Continue

Proceed to [Child Nodes, Name Patterns, And Common Schemas](child-nodes-name-patterns-and-common-schemas.md).

---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# Schema Anatomy, Identity, And Node Selection

A binding schema has two jobs before it constrains any property: identify itself unambiguously and select the DT nodes to which it applies. If selection is wrong, a perfect-looking `properties` block can validate nothing—or validate unrelated nodes.

## File And Document Preamble

New Linux bindings normally use the dual-license SPDX expression and YAML 1.2 marker:

```yaml
# SPDX-License-Identifier: (GPL-2.0-only OR BSD-2-Clause)
%YAML 1.2
---
```

YAML indentation uses spaces, conventionally two per schema level. Tabs are invalid indentation, including inside DTS examples where YAML still owns the outer block.

The filename normally follows a compatible-style name such as:

```text
Documentation/devicetree/bindings/media/acme,ax-capture.yaml
```

If one file covers several compatibles, choose a representative compatible or established generic name consistent with neighboring bindings.

## `$id`: Global Schema Identity

```yaml
$id: http://devicetree.org/schemas/media/acme,ax-capture.yaml#
```

The ID:

- begins with `http://devicetree.org/schemas/`
- mirrors the path below `Documentation/devicetree/bindings/`
- normally ends in the filename plus `#`
- forms the base URI for relative `$ref` resolution
- must be unique across the schema set

The URI is an identifier; it does not imply that an HTTP request fetches the file during a normal kernel build. A copied schema whose `$id` still names the old file creates confusing duplicate or misresolved identities.

## `$schema`: The Binding Meta-Schema

```yaml
$schema: http://devicetree.org/meta-schemas/core.yaml#
```

This declares the DT binding meta-schema. It constrains the permitted JSON Schema vocabulary and DT-specific top-level fields. Do not substitute a generic JSON Schema draft URI: a DT binding is checked against DT's additional rules and transformations.

## Human Ownership And Description

```yaml
title: Acme AX100 and AX200 capture engines

maintainers:
  - Ada Maintainer <ada@example.com>

description: |
  AX capture engines receive parallel sensor samples and transfer them to memory.
  AX200 adds a dedicated DMA-fault interrupt and reset recovery input.
```

Use `title` for one concise device-class description. Use `description` for hardware purpose, standards, important topology, and public data-sheet links. Do not describe Linux driver behavior or restate every property.

Maintainership is part of the operational quality of a binding. Choose people or lists able to review ABI evolution, not a historical author who cannot be reached.

## Default Selection

Most device bindings do not need an explicit `select`. Tooling derives selection from compatible values constrained by the schema, and in some cases from node-name constraints.

```yaml
properties:
  compatible:
    enum:
      - acme,axc100
      - acme,axc200
```

This is preferable to duplicating the same compatible logic under `select`. Duplicate selectors can drift from the property contract.

## Explicit `select`

Use `select` only when normal compatible or node-name extraction cannot express which nodes the binding owns. It is itself a schema applied to candidate nodes.

```yaml
select:
  properties:
    compatible:
      contains:
        const: acme,legacy-capture
  required:
    - compatible
```

The `required` is important. In JSON Schema, a `properties` constraint does not require the named property to exist; without `required`, a node lacking `compatible` can satisfy the selector vacuously.

`select: false` intentionally prevents automatic application. It is used for helper/common schemas meant to be referenced by another schema, or special cases where direct selection is inappropriate.

## Selection Is Separate From Validation

These are distinct questions:

1. Does this schema apply to the node?
2. If it applies, does the node satisfy the schema?

Putting restrictive conditions only in `select` can exclude invalid nodes instead of reporting them. Selection should identify the population; `properties`, `required`, and conditions should validate that population.

Bad strategy:

```yaml
select:
  properties:
    compatible:
      const: acme,axc200
    resets: true
  required: [compatible, resets]
```

An AXC200 node missing `resets` would simply fail selection. The schema should select AXC200 by identity and require `resets` in its validation body.

## Top-Level Structure

A typical device binding contains:

```yaml
$id: ...
$schema: ...

title: ...
maintainers: ...
description: ...

allOf: ...          # common schemas or conditionals, when needed

properties: ...
patternProperties: ...
required: ...

additionalProperties: false
# or unevaluatedProperties: false when composition requires it

examples: ...
```

Order is for readability, not mapping semantics. Follow the current style of nearby accepted bindings and keep `properties` and `required` entries in corresponding logical order.

## `properties` Does Not Imply `required`

```yaml
properties:
  compatible:
    const: acme,axc100
  reg:
    maxItems: 1

required:
  - compatible
  - reg
```

The first block says what values are valid if the properties appear. The second says they must appear. Forgetting `required` is one of the easiest ways to create a schema that accepts an empty or incomplete node.

## Prove Selection

For each compatible or name pattern:

- include at least one real or example node that should match
- introduce one deliberate violation and confirm this schema reports it
- test a nearby unrelated node and confirm this schema does not claim it
- inspect diagnostics for the schema filename and instance path

A clean run with no matching nodes is not successful coverage.

## Common Mistakes

- `$id` path differs from the actual binding path
- copied title, maintainers, or description still name another device
- unnecessary `select` duplicates compatible rules
- selector conditions hide invalid nodes
- `properties` constraints are mistaken for presence requirements
- `select: false` is copied from a helper schema into a concrete device binding
- description specifies driver behavior instead of hardware
- an overly broad node-name selector captures unrelated devices

## Authoritative References

- [Linux schema-writing guide](https://docs.kernel.org/devicetree/bindings/writing-schema.html)
- [Linux annotated example schema](https://github.com/torvalds/linux/blob/master/Documentation/devicetree/bindings/example-schema.yaml)
- [`dt-schema` core meta-schema](https://github.com/devicetree-org/dt-schema/blob/main/dtschema/meta-schemas/core.yaml)

## Continue

Proceed to [Property Types, Encodings, And Cardinality](property-types-encodings-and-cardinality.md).

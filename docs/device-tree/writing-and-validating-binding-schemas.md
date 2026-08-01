---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# Writing And Validating Binding Schemas

A Devicetree binding schema is executable ABI documentation. It explains a hardware contract to humans, selects the nodes to which that contract applies, constrains their encoded properties, compiles its examples, and validates real DTBs. A schema that merely accepts one sample node is not finished: it must reject plausible mistakes without excluding legitimate hardware.

## Learning Outcomes

After completing this module, you should be able to:

- map a reviewed hardware contract into the YAML subset and JSON Schema vocabulary used by `dt-schema`
- construct `$id`, `$schema`, `title`, `maintainers`, `description`, `select`, `properties`, `required`, and `examples` correctly
- distinguish source-level DTS syntax from the array and matrix shapes seen by schema validation
- constrain strings, cells, byte arrays, phandles, phandle arrays, resource lists, and names with exact cardinality
- encode compatible strings and fallback order using `const`, `enum`, `items`, and `oneOf`
- reuse standard schemas through `$ref` and `allOf` without redefining common properties
- use `if`/`then`/`else`, dependencies, and variant-specific narrowing without creating overlapping or permissive branches
- choose correctly between `additionalProperties` and `unevaluatedProperties`
- validate fixed-name and patterned child nodes while preserving ownership of their properties
- write minimal examples that compile independently and exercise the intended contract
- run complete and targeted `dt_binding_check` and `dtbs_check` workflows
- separate YAML, meta-schema, reference-resolution, example, `dtc`, and DTB-validation failures
- detect when an invalid binding was skipped and therefore did not validate any DTS users
- turn validation output into the smallest responsible schema or DTS correction

## Prerequisites

Complete [Binding Design And Stable ABI](binding-design-and-stable-abi.md). You should already have a hardware-centered contract, property semantics, compatible strategy, absence behavior, and old/new compatibility analysis. Schema syntax cannot repair a confused ABI.

## Learning Path

1. [Schema Anatomy, Identity, And Node Selection](writing-and-validating-binding-schemas/schema-anatomy-identity-and-node-selection.md)
2. [Property Types, Encodings, And Cardinality](writing-and-validating-binding-schemas/property-types-encodings-and-cardinality.md)
3. [References, Composition, Conditionals, And Closure](writing-and-validating-binding-schemas/references-composition-conditionals-and-closure.md)
4. [Child Nodes, Name Patterns, And Common Schemas](writing-and-validating-binding-schemas/child-nodes-name-patterns-and-common-schemas.md)
5. [Examples, Vendor Bindings, And Authoring Workflow](writing-and-validating-binding-schemas/examples-vendor-bindings-and-authoring-workflow.md)
6. [Validation Toolchain And Targeted Checks](writing-and-validating-binding-schemas/validation-toolchain-and-targeted-checks.md)
7. [Diagnosing Schema, Example, And DTB Failures](writing-and-validating-binding-schemas/diagnosing-schema-example-and-dtb-failures.md)
8. [Binding Schema Authoring And Validation Lab](writing-and-validating-binding-schemas/binding-schema-authoring-and-validation-lab.md)

## Validation Has Multiple Gates

Treat validation as a pipeline:

```text
YAML text
  -> YAML parse
binding document
  -> DT meta-schema and JSON Schema checks
processed schema set
  -> references resolved and schemas selected
DTS example or platform source
  -> preprocessing and dtc
DTB data model
  -> matching binding schemas
validation diagnostics
```

Each gate answers a different question. A valid YAML document can be an invalid binding. A valid binding can contain an example that does not compile. A clean example does not prove that platform DTBs match. A clean `dtbs_check` run does not prove a broken schema participated.

## Think In Accepted And Rejected Sets

A useful schema defines two sets:

- every supported hardware instance must be accepted
- every unsupported or contradictory description must be rejected

For each rule, create at least one positive and one negative mental example. If AX200 requires two interrupts, validate a complete two-interrupt node and reason about a one-interrupt failure. If AX100 forbids a reset, ensure a reset-bearing AX100 node cannot slip through an unconstrained common property.

## Minimal Schema Skeleton

```yaml
# SPDX-License-Identifier: (GPL-2.0-only OR BSD-2-Clause)
%YAML 1.2
---
$id: http://devicetree.org/schemas/media/acme,ax-capture.yaml#
$schema: http://devicetree.org/meta-schemas/core.yaml#

title: Acme AX capture engine

maintainers:
  - Example Maintainer <maintainer@example.com>

description: |
  The AX capture engine transfers samples from an external interface to memory.

properties:
  compatible:
    enum:
      - acme,axc100
      - acme,axc200

  reg:
    maxItems: 1

required:
  - compatible
  - reg

additionalProperties: false

examples:
  - |
    capture@48000000 {
        compatible = "acme,axc100";
        reg = <0x48000000 0x1000>;
    };
```

This is only a structural starting point. Real schemas must constrain the full resource contract and variants.

## Core Review Loop

For each iteration:

1. state the hardware rule in plain language
2. encode the broad common shape under `properties`
3. narrow variants through compatible-specific conditions
4. close the schema at every object boundary
5. compile and validate examples
6. validate real DTBs that should match
7. introduce a deliberate invalid fixture or mutation
8. confirm the expected rule rejects it for the expected reason

This loop prevents a schema that is syntactically sophisticated but semantically inactive.

## Completion Check

You are ready for [Overlays In Depth](overlays-in-depth.md) when you can:

- explain how a node is selected for a binding and prove that representative DTBs actually match
- translate each DTS property into the processed validation shape and apply exact cardinality
- encode compatible fallback lists without accepting reversed or invented sequences
- compose common schemas while closing the final object correctly
- model child nodes with bounded names, properties, and recursion
- keep examples minimal while still compiling and covering meaningful branches
- run schema checks before interpreting `dtbs_check` as evidence
- target one binding or directory without mistaking a partial run for repository-wide validation
- classify a diagnostic by validation stage and trace its schema path and instance path
- distinguish an undocumented DTS property from a missing schema reference or a wrongly closed object
- demonstrate both positive and negative coverage for each variant contract

## Authoritative References

- [Linux: Writing Devicetree Bindings In JSON Schema](https://docs.kernel.org/devicetree/bindings/writing-schema.html)
- [Linux annotated example schema](https://github.com/torvalds/linux/blob/master/Documentation/devicetree/bindings/example-schema.yaml)
- [`dt-schema` project](https://github.com/devicetree-org/dt-schema)
- [Devicetree Specification](https://devicetree-specification.readthedocs.io/en/stable/)
- [Linux: Submitting Devicetree Binding Patches](https://docs.kernel.org/devicetree/bindings/submitting-patches.html)

## Related Topics

- [Binding Design And Stable ABI](binding-design-and-stable-abi.md)
- [Build And Diagnostic Tools](build-and-diagnostic-tools.md)
- [Device Tree Binding Validation](../build-systems/advanced/linux-kernel/device-tree-binding-validation.md)
- [Product-Scale Maintenance And Engineering](product-scale-maintenance-and-engineering.md)

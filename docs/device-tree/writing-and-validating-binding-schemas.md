---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# Writing And Validating Binding Schemas

This page scaffolds the YAML schema vocabulary and validation workflow used for modern Devicetree bindings.

## Topics Covered

- YAML bindings
- `dt-bindings`
- `$id`
- `$schema`
- `maintainers`
- `description`
- `select`
- `properties`
- `patternProperties`
- `required`
- `$ref`
- `allOf`
- `oneOf`
- conditional schemas
- `additionalProperties` vs `unevaluatedProperties`
- child-node schemas
- property types
- array cardinality
- binding examples
- vendor bindings
- `dt_binding_check`
- targeted validation with `DT_SCHEMA_FILES`
- `dtc` warnings
- `dtbs_check`
- why invalid schemas can cause `dtbs_check` to skip checks
- schema errors
- undocumented properties

## Related Topics

- [Binding Design And Stable ABI](binding-design-and-stable-abi.md)
- [Device Tree Binding Validation](../build-systems/advanced/linux-kernel/device-tree-binding-validation.md)

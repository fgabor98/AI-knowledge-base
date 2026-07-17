---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# Binding Design And Stable ABI

This page covers how to design maintainable bindings that describe hardware and remain compatible over time.

## Topics Covered

- describing hardware rather than Linux implementation details
- avoiding nodes created only to instantiate drivers
- complete hardware descriptions despite incomplete driver support
- binding backward compatibility
- compatible-string versioning
- property naming and standard unit suffixes
- standard property reuse
- avoiding policy in Device Tree
- board and product revision strategies
- Devicetree ABI versioning across product revisions
- binding review expectations
- upstream binding submission workflow
- submitting bindings before DTS users

## Related Topics

- [Driver Matching](driver-matching.md)
- [Writing And Validating Binding Schemas](writing-and-validating-binding-schemas.md)

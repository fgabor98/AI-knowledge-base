---
status: draft
reviewed: false
domain: device-tree
difficulty: beginner
last_reviewed: null
---

# Syntax, Values, And Source Composition

This page covers Device Tree source syntax, value encodings, include mechanisms, and source layering.

## Topics Covered

- 32-bit cells and cell arrays
- strings and string lists
- byte arrays
- empty and boolean properties
- 64-bit values represented by multiple cells
- property and node references
- label references and path references
- `/bits/`
- `/delete-node/`
- `/delete-property/`
- `/include/` directives
- C preprocessor includes and macros
- DTS includes vs C preprocessor includes
- overriding and extending nodes from included DTSI files
- board, SoC, and shared-family source layering
- source formatting and Linux DTS coding style

## Related Topics

- [Foundations](foundations.md)
- [Provider-Consumer Relationships](provider-consumer-relationships.md)

---
status: draft
reviewed: false
domain: device-tree
difficulty: beginner
last_reviewed: null
---

# Syntax, Values, And Source Composition

Readable DTS is not just valid punctuation. It preserves the hardware hierarchy, encodes values exactly as bindings require, and makes board differences obvious enough to review. This module moves from individual source constructs to maintainable multi-file descriptions.

## Learning Outcomes

After completing this module, you should be able to:

- write valid version 1 DTS nodes and properties
- distinguish strings, string lists, cells, byte arrays, and empty properties
- encode 64-bit values and explicitly sized integer arrays
- explain how the same label reference becomes a phandle or a path depending on context
- extend previously defined nodes and remove inherited properties or nodes
- distinguish native `/include/` processing from C preprocessing
- use `dt-bindings` constants without losing sight of the emitted numeric cells
- inspect preprocessed source when macros or includes behave unexpectedly
- layer SoC, module, carrier, and board sources according to physical ownership
- format a Linux DTS for efficient review and low-conflict maintenance

## Prerequisites

Complete [Device Tree Foundations](foundations.md), especially the distinctions among source labels, runtime paths, aliases, and phandles. You should also be comfortable running `dtc` and reading compiler diagnostics.

## Learning Path

1. [DTS Grammar And Value Encodings](syntax-values-and-source-composition/dts-grammar-and-value-encodings.md)
2. [References, Amendments, And Deletions](syntax-values-and-source-composition/references-amendments-and-deletions.md)
3. [Includes, Preprocessing, And Macros](syntax-values-and-source-composition/includes-preprocessing-and-macros.md)
4. [Hardware-Based Layering And Source Style](syntax-values-and-source-composition/hardware-based-layering-and-source-style.md)
5. [Source Composition Lab](syntax-values-and-source-composition/source-composition-lab.md)

The pages reuse a fictional Trainer SoC, system-on-module, and two carrier boards. The growing example shows both the final tree and which physical layer owns each fact.

## Composition Mental Model

```text
textual includes + preprocessing + node amendments + deletions
                              |
                              v
                   one effective source tree
                              |
                             dtc
                              |
                              v
                      one compiled DTB
```

The DTB does not retain a normal include stack, macro vocabulary, or object-oriented inheritance model. Source composition is a maintainability mechanism that must result in one coherent hardware description.

## The Three Questions Behind Every Value

When reading a property, ask:

1. What source notation produced these bytes?
2. Which binding defines their semantic type and cardinality?
3. Which parent bus or provider supplies the context needed to decode them?

For example, angle brackets prove only that the source contains cells. They do not tell you whether those cells are addresses, sizes, flags, phandles, or provider-specific specifiers.

## Scope Boundary

This module teaches source representation and composition. It introduces contextual values only far enough to prevent syntax-level mistakes. Detailed address translation, provider specifiers, schema design, and runtime overlays remain in their dedicated modules.

## Completion Check

You are ready to continue when you can:

- predict the byte representation of a string list, cell array, and byte array
- rewrite a 64-bit integer as two 32-bit cells
- explain `&label` inside and outside `<...>`
- show the difference between extending, disabling, and deleting a node
- identify whether `#include` or `/include/` processes a given file
- produce and inspect a preprocessed DTS
- justify whether a component belongs in the SoC, module, or board source
- review a multi-file change by reconstructing its final effective node

## Authoritative References

- [Devicetree Specification v0.4 release](https://github.com/devicetree-org/devicetree-specification/releases/tag/v0.4)
- [Devicetree Specification: source format](https://devicetree-specification.readthedocs.io/en/stable/source-language.html)
- [Devicetree Specification: values and standard properties](https://devicetree-specification.readthedocs.io/en/stable/devicetree-basics.html)
- [Linux DTS coding style](https://docs.kernel.org/devicetree/bindings/dts-coding-style.html)
- [Device Tree Compiler repository](https://github.com/dgibson/dtc)

## Related Topics

- [Device Tree Foundations](foundations.md)
- [Provider-Consumer Relationships](provider-consumer-relationships.md)
- [Addressing And Bus Modeling](addressing-and-bus-modeling.md)
- [Build And Diagnostic Tools](build-and-diagnostic-tools.md)

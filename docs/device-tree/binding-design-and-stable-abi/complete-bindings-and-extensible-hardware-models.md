---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# Complete Bindings And Extensible Hardware Models

A binding should describe the hardware contract, not the subset today's driver happens to use. Omitting a real required clock, reset, interrupt, or address relationship because support is not implemented makes the ABI depend on a temporary software limitation.

## Complete Does Not Mean Expose Everything

Completeness means representing every externally relevant fact needed for correct ownership and operation. It does not mean copying the data sheet into DT.

Include facts that are:

- non-discoverable and required to locate or operate the block
- instance-specific wiring, resources, limits, or topology
- required to coordinate ownership with another provider or execution domain
- necessary for safe reduced operation by an older consumer

Exclude facts that are:

- internal microarchitecture with no software-visible contract
- reliably discoverable at runtime
- fixed by the selected compatible and suitable for match data
- derivable without ambiguity from another binding value
- current driver defaults or allocation choices

## Inventory Before Schema

Create a resource table for every variant:

| Resource | v1 | v2 | Instance-specific? | Discoverable? | DT representation |
|---|---:|---:|---:|---:|---|
| control registers | yes | yes | address only | no | `reg`, `reg-names` |
| error IRQ | no | yes | routing | no | `interrupts`, names |
| bus clock | yes | yes | provider/specifier | no | `clocks` |
| functional clock | yes | yes | provider/specifier | no | `clocks` |
| FIFO depth | 32 | 64 | no | yes | hardware register |
| reset | shared | dedicated | provider/specifier | no | `resets` |

This table exposes whether one compatible can honestly describe both versions and whether list ordering is stable.

## Describe Unsupported Resources Now

Suppose hardware has completion and error interrupts, but the first driver only handles completion. The binding and DTS should still describe both if both are architected outputs and the intended binding makes them required:

```dts
interrupts = <42>, <43>;
interrupt-names = "completion", "error";
```

The initial driver may request only `completion`. A later driver can use `error` without changing deployed DTBs. The same principle applies to clocks, resets, register windows, and graph endpoints.

Do not describe a hazardous resource that firmware owns as though Linux can use it. Completeness includes ownership and access boundaries.

## Required, Optional, And Conditional

Use precise meanings:

- **required**: every instance covered by this compatible needs the property for an accurate description
- **optional**: hardware may physically omit the relationship, or software can discover its presence; absence has defined semantics
- **conditional**: requirement follows from a compatible, operating mode, or another hardware fact

“The current driver does not need it” is not a reason to make a property optional. Neither is “old DTS files omitted it” unless backward compatibility now constrains the design; document that legacy case separately.

Avoid vague absence semantics. State whether absence means physically not connected, fixed hardware behavior, discoverable behavior, or a legacy DTB for which software must retain a default.

## Stable List Ordering And Names

Ordered resource lists become ABI. Prefer names wherever the subsystem defines parallel `*-names` properties, especially when variants add resources.

Bad evolution:

```text
v1 clocks: bus, functional
v2 clocks: bus, sample, functional
```

Inserting in the middle changes indices. A driver using names is more robust, but schema and older consumers may still assume order. Define the semantic order deliberately and do not reorder existing entries for aesthetics.

Names should describe hardware roles, such as `bus`, `core`, `ref`, `tx`, and `rx`; they should not name the Linux call site that requests them.

## Model Modes Without Contradiction

Some hardware has mutually exclusive operating modes. Prefer a standard mode property or a mode-specific compatible when the distinction changes the programming contract. Conditional resources should form complete, non-overlapping configurations.

Write a truth table:

| Mode | required resources | forbidden resources | discoverable? |
|---|---|---|---|
| host | host IRQ, VBUS supply | device-only wake IRQ | no |
| peripheral | device IRQ | host-only VBUS control | no |
| dual-role | both sets plus role switch | none | partly |

If two properties can request incompatible modes simultaneously, the model is incomplete until the contradiction can be rejected.

## Extensibility Without Property Bags

Design for known hardware axes, not arbitrary future settings. Extensibility comes from:

- precise compatible identities and safe fallback chains
- standard provider-consumer references
- named resources with stable semantics
- well-defined child-node or graph structures
- optional properties whose absence behavior is explicit
- new compatibles when future hardware changes the contract

An open-ended `vendor,config = <...>` array, undocumented firmware blob, or string-to-value map evades review and cannot be validated meaningfully.

## Avoid Duplicate Truth

If the compatible identifies FIFO depth, do not also require `fifo-depth`. If hardware reports a reliable port count, do not repeat it in DT. If child nodes enumerate physically populated ports, do not add a separate count that can disagree.

When two representations appear necessary, define their distinct roles. For example, a controller may report maximum ports while DT child nodes describe which board connectors are actually routed.

## Completeness Review

Use three independent views:

1. **data-sheet view**: all external resources and modes
2. **schematic view**: actual board wiring, constraints, and omissions
3. **software view**: what the driver consumes and what future support is expected to consume

Any fact present only in the software view is suspicious. Any externally important fact present in the first two views but absent from the binding needs an explicit explanation.

## Authoritative References

- [Linux binding design guidelines](https://docs.kernel.org/devicetree/bindings/writing-bindings.html)
- [Linux schema-writing guide](https://docs.kernel.org/devicetree/bindings/writing-schema.html)
- [Devicetree Specification: properties](https://devicetree-specification.readthedocs.io/en/stable/devicetree-basics.html)

## Continue

Proceed to [Property Design, Naming, Units, And Standard Reuse](property-design-naming-units-and-standard-reuse.md).

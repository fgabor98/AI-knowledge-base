---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# Binding Design And Stable ABI

A binding is a long-lived contract between hardware descriptions and every program that consumes them. A useful binding exposes facts software needs, remains valid when driver internals change, and lets old DTBs and new software coexist. A convenient property that merely makes today's Linux driver probe can instead become permanent technical debt.

## Learning Outcomes

After completing this module, you should be able to:

- define a binding from observable hardware and integration constraints rather than a driver implementation
- decide whether a fact belongs in DT, hardware discovery, firmware, driver match data, or runtime policy
- describe complete hardware even when current software supports only part of it
- reuse standard properties, units, providers, and common schemas instead of inventing local vocabulary
- design `compatible` identities and fallback lists with explicit old-DTB/new-kernel and new-DTB/old-kernel behavior
- evolve required and optional properties without breaking deployed DTBs
- deprecate flawed properties and compatibles using an executable migration plan
- model board revisions and product variants without using DT as an uncontrolled configuration database
- build a compatibility matrix across kernels, bootloaders, firmware, DTBs, overlays, and released products
- structure an upstream series so bindings are reviewed before DTS users and drivers consume the contract
- review a proposed binding separately for hardware truth, ABI, schema, DTS use, and driver behavior
- write an ABI decision record that future maintainers can use without reconstructing old discussions

## Prerequisites

Complete [Boot-Time Mutation And Ownership](boot-time-mutation-and-ownership.md). You should understand compatible matching, provider-consumer relationships, final-tree provenance, and the difference between a compiled source tree and the DTB a kernel actually receives.

## Learning Path

1. [Hardware Description Contracts And Policy Boundaries](binding-design-and-stable-abi/hardware-description-contracts-and-policy-boundaries.md)
2. [Complete Bindings And Extensible Hardware Models](binding-design-and-stable-abi/complete-bindings-and-extensible-hardware-models.md)
3. [Property Design, Naming, Units, And Standard Reuse](binding-design-and-stable-abi/property-design-naming-units-and-standard-reuse.md)
4. [Compatible Identities, Fallbacks, And Variant Data](binding-design-and-stable-abi/compatible-identities-fallbacks-and-variant-data.md)
5. [Backward Compatibility, Deprecation, And Migration](binding-design-and-stable-abi/backward-compatibility-deprecation-and-migration.md)
6. [Board Revisions, Products, And Deployment Matrices](binding-design-and-stable-abi/board-revisions-products-and-deployment-matrices.md)
7. [Review Strategy And Upstream Submission Order](binding-design-and-stable-abi/review-strategy-and-upstream-submission-order.md)
8. [Binding Design And ABI Review Lab](binding-design-and-stable-abi/binding-design-and-abi-review-lab.md)

## The Binding Is The Interface

Keep four artifacts conceptually separate:

| Artifact | Responsibility |
|---|---|
| hardware | provides fixed registers, signals, limits, identities, and discoverable capabilities |
| binding | names the hardware facts and relationships software may rely on |
| DT instance | supplies the facts for one SoC, board, or assembled product |
| driver | interprets the binding and implements some or all supported behavior |

The driver's current parsing code is not the binding. A driver may lag the binding, support only one variant, or be replaced entirely. Once deployed, DTBs may outlive the source tree that created them.

## The Placement Test

For every proposed property, ask these questions in order:

1. Is it a stable fact about the hardware or its physical integration?
2. Can software discover it safely and reliably from hardware?
3. Is it already implied by a sufficiently specific `compatible`?
4. Does an existing standard property or provider relationship express it?
5. Does it vary per physical instance, rather than per software release or user preference?
6. Can its semantics be stated without naming a Linux function, subsystem implementation, or algorithm?

If the answer to the first question is no, the value usually belongs in firmware policy, kernel configuration, userspace, or a runtime protocol. If it is discoverable or implied by the compatible, duplicating it in DT creates two sources of truth.

## Compatibility Is A Matrix

Never evaluate an ABI change only with a newly built kernel and newly built DTB:

```text
                    old software       new software
old deployed DTB    released baseline  backward compatibility
new DTB             forward behavior   intended new behavior
```

The new-DTB/old-software cell may mean safe reduced functionality, deliberate non-binding, or an unsupported update order. State which. A fallback compatible is a claim that the old behavior is safe, not a promise that probe merely reaches completion.

For real products, expand “software” into boot firmware, bootloader, kernel, modules, and auxiliary firmware. Add every separately updated DTB or overlay producer.

## ABI Decision Record

For every nontrivial addition or change, preserve:

```text
hardware fact:
why software needs it:
why it is not discoverable:
chosen standard vocabulary:
compatible/property semantics:
valid values and units:
old-DTB/new-software behavior:
new-DTB/old-software behavior:
invalid combinations:
deployment and rollback order:
test evidence:
```

This record is not a substitute for a schema. It captures the reasoning that a schema cannot express, especially why a fallback is safe and which upgrade combinations are intentional.

## Completion Check

You are ready for [Writing And Validating Binding Schemas](writing-and-validating-binding-schemas.md) when you can:

- reject a driver-shaped property and place the underlying requirement in the correct interface
- show that a proposed binding describes the complete hardware contract, including resources unsupported by today's driver
- justify every custom property against standard alternatives and name its exact type, unit, range, and default behavior
- prove each fallback compatible through reduced-functionality and lifecycle testing
- state the behavior of all four old/new DTB and software combinations
- migrate a deployed mistake without silently reinterpreting an existing property
- represent product revisions through physical deltas, clear identities, and bounded update policy
- produce a patch dependency graph with the binding before DTS users and consumers
- distinguish schema-valid syntax from a semantically sound, stable hardware ABI
- turn a field failure into a binding rule, compatibility test, or review checklist item

## Authoritative References

- [Linux Devicetree binding design guidelines](https://docs.kernel.org/devicetree/bindings/writing-bindings.html)
- [Linux Devicetree schema-writing guide](https://docs.kernel.org/devicetree/bindings/writing-schema.html)
- [Devicetree Specification](https://devicetree-specification.readthedocs.io/en/stable/)
- [Linux submitting Devicetree patches](https://docs.kernel.org/devicetree/bindings/submitting-patches.html)
- [Linux SoC maintainer guidance](https://docs.kernel.org/process/maintainer-soc.html)

## Related Topics

- [Driver Matching](driver-matching.md)
- [Standard Nodes And Properties](standard-nodes-and-properties.md)
- [Boot-Time Mutation And Ownership](boot-time-mutation-and-ownership.md)
- [Writing And Validating Binding Schemas](writing-and-validating-binding-schemas.md)
- [Product-Scale Maintenance And Engineering](product-scale-maintenance-and-engineering.md)

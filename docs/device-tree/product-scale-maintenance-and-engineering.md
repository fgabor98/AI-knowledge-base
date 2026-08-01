---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# Product-Scale Maintenance And Engineering

A Device Tree that boots one board is a technical result. A Device Tree system that remains correct across silicon revisions, modules, carriers, products, kernels, bootloaders, teams, and release branches is an engineering capability. Product-scale work is mostly about controlling relationships: what is common, what varies, who owns each contract, which combinations are supported, and how failures become permanent checks.

This module turns a growing collection of DTS files into a maintained product architecture with explicit compatibility policy, review dimensions, upstream strategy, migration rules, CI matrices, hardware coverage, and learning feedback.

## Learning Outcomes

After completing this module, you should be able to:

- distinguish Devicetree Specification, binding, toolchain, and platform conventions and record a supported implementation baseline
- decide which hardware description belongs in SoC, package, system-on-module, carrier, board, product, or overlay layers
- minimize variant deltas without hiding real hardware differences behind conditionals or software-policy properties
- define ownership and review responsibility across silicon, module, carrier, product, boot firmware, kernel, and release teams
- review DT changes independently for hardware correctness, binding/ABI safety, style, maintainability, integration, and test coverage
- split binding, driver, DTS, and migration work into reviewable and bisectable upstream series
- maintain a measurable downstream patch stack while reducing long-lived divergence and duplicate sources of truth
- introduce, deprecate, and migrate compatibles and properties without breaking supported old DTB/new kernel combinations
- construct a CI matrix from products, variants, overlays, toolchains, release lines, and change impact instead of testing an accidental Cartesian product
- combine schema/build/static checks, artifact provenance, semantic diffs, boot tests, runtime assertions, and subsystem tests
- choose representative hardware coverage using risk and equivalence classes and state what simulation cannot prove
- write postmortems that trace technical, detection, and process causes and convert each escape into an owned prevention or detection control
- plan upstream convergence and product releases without making one block silently depend on the other

## Prerequisites

Complete [Binding Design And Stable ABI](binding-design-and-stable-abi.md), [Writing And Validating Binding Schemas](writing-and-validating-binding-schemas.md), [Build And Diagnostic Tools](build-and-diagnostic-tools.md), and [Security And Production Lifecycle](security-and-production-lifecycle.md). This module assumes you can diagnose an individual DT failure; it focuses on preventing recurrence across a portfolio.

## Learning Path

1. [Specification, Binding, And Toolchain Compatibility Baselines](product-scale-maintenance-and-engineering/specification-binding-and-toolchain-compatibility-baselines.md)
2. [Layering, Variants, Ownership, And Source Architecture](product-scale-maintenance-and-engineering/layering-variants-ownership-and-source-architecture.md)
3. [Multidimensional Review And Change Design](product-scale-maintenance-and-engineering/multidimensional-review-and-change-design.md)
4. [Upstream Development And Downstream Patch-Stack Discipline](product-scale-maintenance-and-engineering/upstream-development-and-downstream-patch-stack-discipline.md)
5. [Binding Evolution, Deprecation, And Migration](product-scale-maintenance-and-engineering/binding-evolution-deprecation-and-migration.md)
6. [Matrix CI, Artifact Validation, And Compatibility Testing](product-scale-maintenance-and-engineering/matrix-ci-artifact-validation-and-compatibility-testing.md)
7. [Hardware Coverage, Release Qualification, And Learning From Escapes](product-scale-maintenance-and-engineering/hardware-coverage-release-qualification-and-learning-from-escapes.md)
8. [Product-Family Maintenance And Regression Prevention Lab](product-scale-maintenance-and-engineering/product-family-maintenance-and-regression-prevention-lab.md)

## Product-Scale Control Loop

```text
hardware/product model
  -> layered source and binding contracts
  -> scoped review across independent dimensions
  -> reproducible build and provenance
  -> impact-derived validation matrix
  -> representative hardware qualification
  -> upstream/downstream release integration
  -> field evidence and incident learning
  -> stronger model, review rule, schema, test, or ownership
```

The loop is incomplete when a failure is fixed only in one board file. The lasting result is the new control that prevents or detects the same class across every affected product.

## Engineering Evidence Matrix

| Evidence | Establishes | Does not establish |
|---|---|---|
| `dt_binding_check` passes | binding schema and examples validate | schema describes the hardware correctly |
| `dtbs_check` passes | built nodes satisfy applicable schemas | all deployed variants were built |
| source diff is small | textual delta is small | semantic blast radius is small |
| common DTSI reuse | text is shared | shared hardware is truly identical |
| old DTB boots new kernel | one compatibility direction on tested board works | new DTB works with old supported kernel |
| QEMU/sandbox boot passes | selected software paths execute | real power, timing, signal, DMA, or thermal behavior works |
| one representative board passes | that sample and configuration work | every claimed equivalence class is covered |
| patch is upstream | community accepted that revision | downstream release integrated and qualified it |
| incident action item exists | remediation was proposed | an owned, verified control was deployed |

## Required Product Records

Maintain these as versioned, reviewable artifacts:

- supported board/revision/option inventory and stable identity source
- source-layer map and ownership table
- binding and compatible lifecycle register
- upstream/downstream patch inventory with status and rationale
- supported kernel/bootloader/DTB/DTBO compatibility windows
- generated artifact manifest and provenance
- CI change-impact rules and validation matrix
- hardware equivalence classes and lab inventory
- release qualification record and explicit exceptions
- incident/postmortem actions linked to tests, schemas, or review rules

## Completion Check

You are ready for [Board Porting Workflow](board-porting-workflow.md) when you can:

- explain the applicable specification and toolchain baseline for every supported release line
- place a change in the narrowest correct hardware layer and identify all inheriting products
- use an ownership/RACI map without splitting responsibility for a single contract ambiguously
- review one patch separately for correctness, ABI, style, maintainability, integration, and evidence
- produce an upstreamable series and a downstream patch ledger with no anonymous carry patches
- migrate a binding using a compatibility window and explicit removal criteria
- derive CI jobs from a change-impact graph and report unsupported combinations honestly
- justify hardware representatives and identify remaining physical risks
- turn a bring-up or field escape into schema, static, boot, runtime, and process controls
- complete the lab with an ordered containment, migration, upstream, test, and release plan

## Authoritative References

- [Devicetree Specification](https://devicetree-specification.readthedocs.io/en/stable/)
- [Linux Devicetree ABI guidance](https://docs.kernel.org/devicetree/bindings/ABI.html)
- [Linux DTS coding style](https://docs.kernel.org/devicetree/bindings/dts-coding-style.html)
- [Writing Devicetree bindings](https://docs.kernel.org/devicetree/bindings/writing-bindings.html)
- [Submitting Devicetree binding patches](https://docs.kernel.org/devicetree/bindings/submitting-patches.html)
- [Linux patch submission guide](https://docs.kernel.org/process/submitting-patches.html)
- [Linux SoC maintainer process](https://docs.kernel.org/process/maintainer-soc.html)

## Related Topics

- [Binding Design And Stable ABI](binding-design-and-stable-abi.md)
- [Writing And Validating Binding Schemas](writing-and-validating-binding-schemas.md)
- [Build And Diagnostic Tools](build-and-diagnostic-tools.md)
- [Security And Production Lifecycle](security-and-production-lifecycle.md)
- [Board Porting Workflow](board-porting-workflow.md)

---
status: active
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# 7. Binding ABI, Schema Authoring, Validation, And Upstreaming

Official section: [Linux Devicetree bindings documentation](https://docs.kernel.org/devicetree/bindings/index.html)

Knowledge-guide companion: [Stage 7](knowledge-guide-companion.md#stage-7-binding-abi-schema-authoring-validation-and-upstreaming)

## Binding Design And ABI

- [ ] **P0** [Devicetree ABI guidance](https://docs.kernel.org/devicetree/bindings/ABI.html).
- [ ] **P0** [DOs and DON'Ts for bindings](https://docs.kernel.org/devicetree/bindings/writing-bindings.html).
- [ ] **P0** Describe hardware, not Linux driver state or product software policy.
- [ ] **P0** Make bindings complete even when the current driver supports only a subset.
- [ ] **P0** Reuse common properties, node classes, and standard unit suffixes.
- [ ] **P0** Constrain type, count, ordering, units, allowed values, and relationships.
- [ ] **P0** Use specific compatible identities and only truthful fallback chains.
- [ ] **P0** Preserve old-property meaning and old-DTB/new-kernel behavior; use a new compatible for incompatible hardware semantics.
- [ ] **P1** Research consumers beyond Linux before tightening or removing an established contract.

## Schema Anatomy

Official guide: [Writing Devicetree Bindings in json-schema](https://docs.kernel.org/devicetree/bindings/writing-schema.html)

- [ ] **P0** YAML/JSON-compatible syntax, SPDX license, `%YAML`, and document markers.
- [ ] **P0** `$id`, `$schema`, title, maintainers, and description.
- [ ] **P0** Default compatible-based selection and explicit `select` only when required.
- [ ] **P0** `properties`, `patternProperties`, and `required` ordering.
- [ ] **P0** Types from `schemas/types.yaml`, standard suffix inference, and explicit descriptions.
- [ ] **P0** Scalar/list/matrix transformations between DT YAML encoding and schema authoring view.
- [ ] **P0** `minItems`, `maxItems`, fixed `items`, enums, consts, ranges, and uniqueness.
- [ ] **P0** `$ref`, `allOf`, `oneOf`, `anyOf`, `if`/`then`/`else`, and evaluation behavior.
- [ ] **P0** `additionalProperties: false` versus `unevaluatedProperties: false`.
- [ ] **P0** child nodes, common bus schemas, node-name patterns, and dependent constraints.
- [ ] **P1** `deprecated: true` as documentation/policy, not automatic permission to break deployed DTBs.

## Canonical Examples And Common Schemas

- [ ] **P0** [Annotated example schema](https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/tree/Documentation/devicetree/bindings/example-schema.yaml).
- [ ] **P0** `schemas/types.yaml`, `schemas/core.yaml`, and relevant common schemas in the exact `dt-schema` version.
- [ ] **P0** [dt-schema project](https://github.com/devicetree-org/dt-schema): README, release/version, dependencies, and changelog affecting the project.
- [ ] **P0** Compare at least three current upstream bindings in the same subsystem.
- [ ] **P0** Ensure examples compile as standalone schema examples and match every review revision.
- [ ] **P1** Vendor-prefix and property-unit schema sources.
- [ ] **P1** Meta-schema behavior and generated processed schema when diagnosing unexpected validation.

## Validation Toolchain

- [ ] **P0** `make dt_binding_check` for schema/meta-schema and examples.
- [ ] **P0** `make dtbs_check` for built DTBs against processed schemas.
- [ ] **P0** Understand that `dtbs_check` can skip invalid schemas; run binding checks too.
- [ ] **P0** Target `DT_SCHEMA_FILES` precisely while iterating.
- [ ] **P0** Run full relevant architecture/family checks before submission/release.
- [ ] **P0** Pin Python, `dt-schema`, dtc, kernel tree, configuration, and build command in evidence.
- [ ] **P1** `yamllint`, `checkpatch.pl`, and project warning policy as additional gates.
- [ ] **P1** Direct `dt-doc-validate`, `dt-mk-schema`, and `dt-validate` only when understanding/debugging the build wrapper.

## Diagnose Failures By Layer

- [ ] **P0** YAML parser failure versus JSON Schema/meta-schema failure.
- [ ] **P0** unresolved `$ref` or wrong `$id` path.
- [ ] **P0** example compile failure versus example schema failure.
- [ ] **P0** schema selection failure versus property/cardinality/closure failure.
- [ ] **P0** DTS defect versus incomplete/incorrect schema versus intentional historical ABI.
- [ ] **P0** new-tool detection of an old defect versus an introduced product regression.
- [ ] **P1** Reduce to a minimal schema/node reproducer before adding exceptions.

## DTS Style And Source Architecture

- [ ] **P0** [DTS coding style](https://docs.kernel.org/devicetree/bindings/dts-coding-style.html).
- [ ] **P0** Names, labels, unit addresses, hexadecimal case, property order, node order, indentation, and wrapping.
- [ ] **P0** SoC, package, module, carrier, and board source organization by reusable physical hardware.
- [ ] **P0** Mechanical source moves separately from semantic changes.
- [ ] **P1** Architecture/subarchitecture-specific conventions from maintainers and neighboring files.

## Upstream Submission

- [ ] **P0** [Submitting Devicetree binding patches](https://docs.kernel.org/devicetree/bindings/submitting-patches.html).
- [ ] **P0** [Linux patch submission guide](https://docs.kernel.org/process/submitting-patches.html).
- [ ] **P0** Document compatible strings before DTS users.
- [ ] **P0** Place binding before driver use; keep DTS changes separate because they normally travel through SoC/platform trees.
- [ ] **P0** Query `MAINTAINERS`, history, and `scripts/get_maintainer.pl` from the target tree.
- [ ] **P0** Keep each patch logical, reviewable, buildable, and bisectable.
- [ ] **P1** [SoC maintainer process](https://docs.kernel.org/process/maintainer-soc.html) for platform DTS routing.
- [ ] **P1** Track review links, versions, dependencies, downstream carry status, and drop conditions.

## Authoring Exercise

- [ ] Select a real undocumented or incomplete device contract.
- [ ] Write the hardware model and compatibility policy before YAML.
- [ ] Author a closed schema with meaningful examples and exact cardinality.
- [ ] Validate schema, examples, affected DTBs, and unrelated family DTBs.
- [ ] Test old/new compatible and property combinations.
- [ ] Review generated semantic DTB differences.
- [ ] Split binding, driver, SoC data, and board enablement into the appropriate series.

## Stage Completion

- [ ] I can design a binding that is hardware-based, complete, closed, and backward-compatible.
- [ ] I can author and debug YAML schemas using the exact dt-schema transformation model.
- [ ] I can classify validation failures rather than suppress them.
- [ ] I can prepare a correctly routed, bisectable upstream series and maintain its downstream lifecycle.


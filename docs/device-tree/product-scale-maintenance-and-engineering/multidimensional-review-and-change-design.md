---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# Multidimensional Review And Change Design

DT review fails when “it builds” is treated as a complete verdict. A change can be schema-valid but electrically wrong, functionally correct but ABI-breaking, or correct for one board but misplaced in a shared layer. Review each dimension explicitly and require evidence proportional to blast radius.

## Separate Review Dimensions

### Hardware correctness

- node represents real hardware, not desired driver behavior
- address, size, interrupt, polarity, clock, reset, DMA, and power data match schematics/reference manuals
- provider specifier cell counts and meanings are correct
- pin electrical configuration and voltage domains are safe
- parent bus and address translation are correct
- board revision and fitted-option assumptions are supported

### Binding and ABI

- compatible is documented before use
- properties are defined, typed, constrained, and complete
- existing property meaning is unchanged
- old valid DTBs retain defined behavior with new drivers
- fallback compatibles represent real programming compatibility
- required/optional transitions and defaults have a migration plan

### Style and source organization

- naming, unit addresses, property/node ordering, indentation, and comments follow current project rules
- facts live in the correct physical layer
- labels, overrides, and includes preserve navigation and reduce conflicts
- no unrelated formatting obscures semantic change

### Maintainability

- commonality is physical rather than coincidental
- variant strategy scales to expected revisions
- no magic numeric values when stable binding constants exist
- comments explain non-obvious hardware reasons, not restate syntax
- generated content has an owned reproducible source

### Integration and lifecycle

- bootloader fixups, overlays, firmware, kernel versions, and release packaging are accounted for
- downstream/upstream dependencies and merge order are explicit
- rollback and older supported artifacts remain valid where promised
- ownership and deprecation state are recorded

### Evidence

- binding/example validation
- affected DTB builds with warnings policy
- semantic artifact diff
- overlay composition tests
- boot and runtime assertions
- subsystem-level hardware result
- cross-version coverage based on impact

## Start With A Change Contract

Before editing, write:

```text
problem and user/hardware impact:
hardware evidence:
affected binding/ABI:
source layer and inheritors:
supported release lines:
bootloader/overlay interactions:
expected semantic artifact delta:
validation plan:
rollback/migration plan:
owners and reviewers:
```

This makes hidden assumptions reviewable and guides patch boundaries.

## Read The Built Tree, Not Only Source

Includes and overrides make source diffs incomplete. For every affected artifact:

```bash
make ARCH=arm64 dtbs_check DT_SCHEMA_FILES=/capture/

dtc -I dtb -O dts -o before.dts before.dtb
dtc -I dtb -O dts -o after.dts after.dtb
diff -u before.dts after.dts
```

Normalize with the same tool version and account for labels/phandles when interpreting differences. The review question is “what hardware description changed for every consumer?”

## Use A Review Evidence Table

| Claim | Source evidence | Automated evidence | Hardware evidence |
|---|---|---|---|
| interrupt is active-low | schematic/netlist + controller binding | schema/cell check | interrupt triggers once under test |
| property is backward-compatible | binding history + driver behavior | old-DTB/new-kernel test | representative old board boots |
| shared DTSI change affects only revC+ | include graph + inventory | generated artifact diff set | chosen equivalence representatives |
| overlay remains compatible | target/symbol ABI | compose every allowed pair | affected option operates |

Assertions without a named evidence source remain review debt.

## Design Reviewable Patches

Prefer an ordered series:

1. binding/schema addition or compatible extension
2. binding constants/header when required
3. driver support preserving old behavior
4. SoC/module shared description
5. board/product enablement
6. migration of existing users
7. cleanup only after behavior is stable

Exact routing can require binding/driver and DTS patches to travel through different maintainer trees. Keep dependencies explicit and each patch buildable/bisectable.

Separate:

- mechanical move from semantic edit
- warning cleanup from feature addition
- binding conversion from tightening constraints
- bug fix from refactoring
- one hardware reason from another

## Review Schemas As Executable ABI

Ask:

- Does `compatible` selection cover every intended node and only those nodes?
- Are list lengths, item ordering, and provider names constrained?
- Do conditionals express hardware differences rather than driver versions?
- Does `additionalProperties: false` or `unevaluatedProperties: false` close the intended object?
- Do examples exercise meaningful variants and still match the prose?
- Will tightening reject historically valid deployed trees?
- Is an apparent schema error a DTS defect, schema omission, or deliberate legacy allowance?

A schema change that causes many failures needs classification, not mass suppression.

## Prioritize Findings

Use categories rather than one undifferentiated comment stream:

```text
BLOCKER: safety, corruption, security, ABI break, unbootable intermediate patch
MAJOR: incorrect binding, wrong shared layer, unsupported compatibility assumption
MINOR: maintainability or localized organization problem
STYLE: convention/readability with no semantic effect
QUESTION: evidence or intent is unclear
```

Style matters, but mixing style churn into a functional fix increases review risk. Resolve blockers first and move unrelated cleanup.

## Review Comments That Scale

A useful comment states:

```text
observation: this regulator is defined in module-common.dtsi
impact: carrier B does not populate it but inherits an enabled consumer
contract: module layer must be true for every carrier inheritor
request: move the regulator and consumer supply link to carrier A layer
evidence/test: rebuild all module consumers and verify only carrier A artifact changes
```

This creates a reusable rule rather than a file-local preference.

## Handle Exceptions Explicitly

An accepted exception should contain:

- precise rule being waived
- technical reason
- affected products/releases
- risk and compensating evidence
- owner
- expiration or removal trigger
- tracking identifier

Avoid anonymous `W=1` suppressions, schema allowlists, or “temporary” downstream properties with no lifecycle.

## Change-Risk Heuristic

Increase review and test depth when a change:

- touches high-fan-out DTSI or common schema
- changes address, interrupt, DMA/IOMMU, power, thermal, memory, or boot storage
- alters compatible matching or required properties
- changes labels/symbols consumed by overlays
- depends on bootloader fixups or cross-tree merges
- affects multiple supported kernel/firmware lines
- modifies generated artifact bytes unexpectedly
- has no physical representative in CI

Line count is a weak risk measure.

## Reviewer Checklist

```text
[ ] change contract states problem, layer, ABI, blast radius, and evidence
[ ] hardware source is named
[ ] binding exists and schema behavior is understood
[ ] old/new compatibility direction is explicit
[ ] all transitive source consumers are identified
[ ] built semantic diffs match intended products only
[ ] patches separate mechanics, contracts, code, DTS, and cleanup
[ ] overlay and bootloader interactions are tested
[ ] exceptions are owned and time-bounded
[ ] hardware evidence proves subsystem function, not just probe
```

## Further Reading

- [Linux DTS coding style](https://docs.kernel.org/devicetree/bindings/dts-coding-style.html)
- [Writing Devicetree bindings](https://docs.kernel.org/devicetree/bindings/writing-bindings.html)
- [Submitting Devicetree binding patches](https://docs.kernel.org/devicetree/bindings/submitting-patches.html)
- [Upstream Development And Downstream Patch-Stack Discipline](upstream-development-and-downstream-patch-stack-discipline.md)

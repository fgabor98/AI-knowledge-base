---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# Review Strategy And Upstream Submission Order

A strong review proves five different things: the model matches hardware, the ABI can evolve, the schema encodes it, DTS instances use it correctly, and consumers behave correctly. Passing `dtbs_check` proves only part of that chain.

## Review In Independent Passes

### 1. Hardware Truth

Compare the proposal with manuals and schematics:

- Are all address regions, interrupts, clocks, resets, supplies, DMA paths, and modes represented?
- Which facts are discoverable or implied by compatible?
- Does each node correspond to a defensible entity?
- Are ownership and security boundaries accurate?

### 2. Binding Semantics

Read the binding without driver code:

- Can every property be defined in hardware terms?
- Are units, ranges, ordering, absence, and defaults exact?
- Are standard properties reused?
- Are required, optional, and conditional resources honest?
- Can invalid combinations be stated?

### 3. ABI Evolution

- Are compatible fallbacks safe?
- What happens for all old/new DTB and consumer pairings?
- Has any existing value been reinterpreted?
- Can future known variants be represented without a property bag?
- Is deprecation separate from consumer removal?

### 4. Schema And Examples

- Does the schema encode compatible sequences, cardinality, conditional requirements, and closed property sets?
- Do examples demonstrate the normal complete case?
- Do targeted binding checks and real `dtbs_check` pass?

Detailed YAML mechanics belong to the next module, [Writing And Validating Binding Schemas](../writing-and-validating-binding-schemas.md).

### 5. Consumer And Lifecycle

- Does the driver use names rather than fragile indices where appropriate?
- Are missing optional resources handled according to the binding?
- Are errors propagated rather than converted into guesses?
- Do probe, defer, suspend, resume, reset, and recovery honor providers and ownership?
- Does driver match data agree with compatible semantics?

## Patch Dependency Graph

A typical upstream change is logically ordered:

```text
binding
  +-> headers/constants, if required by binding
  +-> driver support
  +-> SoC DTSI node
  +-> board DTS enablement
  +-> overlays or product variants
```

Bindings should be submitted before or with their first DTS users, and normally appear first in a series. DTS must not introduce undocumented compatibles or properties. Driver code should not establish an ABI by merging before its binding is accepted.

Separate patches by maintainership and review domain, but explain dependencies in the cover letter. Do not assume all patches will merge through one tree or in one release.

## Why Binding Before DTS Matters

Reviewing the binding first forces agreement on the interface before examples harden into deployed ABI. It also enables automation to validate DTS users. A DTS-only property can spread to downstream products before anyone decides its stable meaning.

When a driver already exists for undocumented firmware data, document the binding without silently redesigning deployed behavior. Inventory real users and treat them as compatibility constraints.

## Cover Letter Content

State:

- hardware and integration problem
- affected SoCs, boards, and variants
- whether the ABI is new or already deployed
- reasons for new compatible and custom properties
- fallback safety argument
- patch dependency and merge strategy
- validation commands and results
- old/new compatibility testing
- links to previous discussion or versions

For revisions, include a concise change log that explains design changes, not only textual patch changes.

## Commit Structure

A reviewable series might be:

```text
1/5 dt-bindings: vendor: add AX200 capture engine
2/5 media: ax-capture: add AX200 compatible and resources
3/5 arm64: dts: acme: add AX200 capture node
4/5 arm64: dts: acme: enable capture on Falcon Rev B
5/5 MAINTAINERS: add binding and driver entries
```

Avoid combining binding, driver, and multiple board changes into one large patch. Each commit should build and validate at its intended dependency point when practical.

## Evidence Package

At minimum, collect:

- targeted schema validation
- example compilation
- affected architecture DT build
- affected DTB schema validation
- clean compiler warnings at the project's expected level
- normalized dump of representative final DTBs
- old-DTB/new-driver behavior
- new-DTB/old-driver behavior when claimed supported
- hardware results for normal, power-management, and error paths

Do not report only “boots.” State what was exercised.

## Common Review Failures

- deriving the schema from `of_property_read_*()` calls rather than hardware
- adding a vendor boolean for a driver quirk implied by compatible
- marking a resource optional only to preserve an incomplete example
- claiming fallback because register offset zero still responds
- mixing a board property into a reusable IP binding
- updating DTS and driver together without old/new cross-testing
- submitting DTS before the binding is accepted
- treating `dtbs_check` success as proof of electrical or lifecycle correctness
- ignoring bootloader or firmware consumers of the same node

## Reviewer Question Set

Ask the author to answer in one sentence each:

1. What physical fact does each new field represent?
2. Why can software not discover it?
3. Which standard property was considered?
4. Why is each fallback safe under error and power transitions?
5. What does absence mean for every compatible?
6. Which shipped artifact already uses the interface?
7. Which independently updated consumers parse it?
8. What sequence gets the binding, consumer, and DTS into users' hands?
9. What rollback pairing is possible?
10. Which validation and hardware tests demonstrate the claims?

## Upstream Versus Downstream

Downstream urgency does not remove ABI cost. If a temporary property ships, assume it may become permanent. Keep downstream patches close to upstream form, submit the binding early, record deviations, and prevent undocumented properties from multiplying across products.

When upstream feedback changes the interface, plan a downstream migration rather than silently rewriting old releases.

## Authoritative References

- [Linux submitting Devicetree patches](https://docs.kernel.org/devicetree/bindings/submitting-patches.html)
- [Linux binding design guidelines](https://docs.kernel.org/devicetree/bindings/writing-bindings.html)
- [Linux schema-writing guide](https://docs.kernel.org/devicetree/bindings/writing-schema.html)
- [Linux kernel patch submission guide](https://docs.kernel.org/process/submitting-patches.html)
- [Linux SoC maintainer guidance](https://docs.kernel.org/process/maintainer-soc.html)

## Continue

Proceed to the [Binding Design And ABI Review Lab](binding-design-and-abi-review-lab.md).

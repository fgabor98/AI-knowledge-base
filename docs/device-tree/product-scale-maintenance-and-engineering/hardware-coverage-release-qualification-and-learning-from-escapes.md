---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# Hardware Coverage, Release Qualification, And Learning From Escapes

Physical testing is finite, so choose it with an explicit risk model. A representative is valid only for the characteristics relevant to the change. Then use every escaped defect to refine equivalence classes, review rules, schemas, assertions, and ownership—not merely to patch the failed board.

## Define Equivalence By Risk

Boards can share one class for a CPU-node style change but require different classes for a regulator, memory, or peripheral change. Consider:

- SoC and silicon revision
- package and pin multiplexing
- module revision and PMIC/RAM design
- carrier power, reset, clock, and connector wiring
- board revision and fitted component alternatives
- memory size/layout and reserved regions
- boot storage and boot firmware path
- optional overlays and application order
- DMA/IOMMU topology
- interrupt controller/routing
- thermal sensors, trips, cooling, and enclosure
- recovery/update connectivity

Document the equivalence claim per test, not once for the whole product.

## Select Representatives

Use a coverage table:

| Class | Distinguishing risk | Representative | Alternate/backup | Evidence |
|---|---|---|---|---|
| revB base | old PMIC + 2 GiB | asset 0142 | asset 0161 | power/boot/storage |
| revC capture | new IOMMU stream + 4 GiB | asset 0481 | asset 0489 | DMA soak |
| revC radio | overlay + shared reset | asset 0502 | none | RF/reset/concurrency |
| recovery | SPI fallback path | asset 0204 | asset 0205 | corrupt-slot recovery |

If a class has no representative, release risk is explicit; a nearby board does not become equivalent by convenience.

## Match Test Depth To Change Risk

### Low-risk source-only reorganization

- exact/semantic artifact equivalence for every consumer
- overlay symbol/fixup comparison
- smoke boot if bytes changed unexpectedly

### Device enablement or resource correction

- boot and binding evidence
- real subsystem operation
- interrupt/DMA/error path
- suspend/resume or power-cycle where relevant

### Shared power, memory, thermal, storage, or update path

- every distinct physical class
- stress and fault injection
- cold/warm boot and repeated cycles
- rollback/recovery and power-loss tests
- longer soak with instrumentation

## Qualify Outcomes, Not Milestones

“Reached userspace” does not prove:

- storage is reliable under load
- interrupts are correctly triggered and acknowledged
- DMA respects intended isolation
- regulator sequencing is safe
- thermal control acts at the right point
- optional hardware coexists with base peripherals
- recovery remains reachable

Define functional, negative, stress, and recovery assertions for the affected subsystem.

## Create A Release Evidence Record

```text
release set and manifest digest
source/toolchain baseline
change list and impact analysis
schema/build/semantic-diff results
supported compatibility matrix
hardware classes and assets tested
fixture/environment identities
raw logs and measurements
failures, retries, waivers, and rationale
sign-off by accountable owners
known unsupported combinations
field monitoring and rollback thresholds
```

A waiver must have risk, owner, expiry, and follow-up. “Lab unavailable” is a status, not a technical justification.

## Separate Containment, Correction, And Prevention

After an escape:

1. **Contain:** stop rollout, preserve evidence, protect devices, establish recovery.
2. **Correct:** fix affected release/source/artifact and qualify it.
3. **Prevent/detect:** improve model, contract, schema, CI, review, or ownership.

Rushing directly to a DTS patch often destroys original evidence and leaves the systemic cause untouched.

## Build A Causal Timeline

Use artifact and decision checkpoints:

```text
hardware revision approved
  -> schematic/netlist changed
  -> source-layer decision
  -> binding/review
  -> generated DTBs
  -> CI selection
  -> hardware qualification
  -> release packaging
  -> rollout/field observation
```

At each point ask what information existed, who owned the decision, which control ran, and why the defect passed.

## Analyze Multiple Cause Layers

### Technical cause

The immediate incorrect property, provider, composition, or behavior.

### Introduction cause

Why the change was represented incorrectly: ambiguous schematic, wrong common layer, copied node, incompatible binding assumption, generator defect.

### Escape cause

Why review, schema, CI, hardware selection, or release qualification did not detect it.

### Amplification cause

Why impact spread: high-fan-out include, mutable selection, incomplete rollback, poor fleet visibility.

### Recovery cause

Why restoration was slow or unsafe: missing evidence, no representative, unsigned recovery, unclear owner.

Avoid stopping at “human error.” Humans operate inside the review and tooling system being improved.

## Convert Findings Into Controls

| Finding | Strong control |
|---|---|
| wrong cell count | schema constraint + `dtbs_check` |
| wrong board inherits node | inventory-driven artifact assertion + source-layer rule |
| wrong interrupt polarity | schematic-derived review evidence + hardware interrupt test |
| overlay/base mismatch | compatibility manifest + compose/reject matrix |
| old DTB breaks new driver | compatibility test fixture |
| CI skipped new revision | inventory completeness gate |
| lab representative mislabeled | hardware identity attestation/inventory audit |
| fix not backported | patch ledger and affected-release automation |

Prefer controls closest to introduction. Hardware tests remain essential for facts that static analysis cannot prove.

## Write Verifiable Action Items

Weak:

```text
Be more careful reviewing regulator changes.
```

Strong:

```text
Owner: carrier-platform
Due: product-44 branch cut
Action: add inventory rule mapping each carrier regulator node to populated BOM revisions
Verification: CI fixture with revB absent/revC present cases; change 174 cannot merge without both
Evidence: test report linked from release qualification
```

Every action needs owner, due/trigger, observable completion, and effectiveness check.

## Close The Feedback Loop

Update the appropriate durable artifact:

- binding or schema
- DTS layering convention
- compatible/deprecation register
- source/variant inventory
- patch review template
- semantic assertion
- compatibility matrix
- hardware equivalence classes
- lab fixture/instrumentation
- release gate
- field telemetry/support bundle
- ownership/maintainer entry

Review action effectiveness after later releases. A merged test that never runs on the release branch is not closure.

## Postmortem Template

```text
impact and affected population:
detection and containment:
exact release/artifact/runtime evidence:
causal timeline:
technical cause:
introduction cause:
escape cause:
amplification/recovery factors:
what worked:
corrective change and compatibility impact:
prevention/detection actions (owner, due, proof):
affected branches/products/upstream status:
release and field verification:
effectiveness review date:
```

## Further Reading

- [Runtime Device Tree And Probe Forensics Lab](../runtime-inspection/runtime-device-tree-and-probe-forensics-lab.md)
- [Field Updates, Recovery, And Key Rotation](../security-and-production-lifecycle/field-updates-recovery-and-key-rotation.md)
- [Product-Family Maintenance And Regression Prevention Lab](product-family-maintenance-and-regression-prevention-lab.md)

---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# Backward Compatibility, Deprecation, And Migration

DTBs can be stored in boot firmware, a filesystem, a signed image, field-replaceable modules, or a factory partition. They may be updated independently from kernels and bootloaders. Once shipped, a binding mistake cannot be repaired by changing every source file in one repository.

## Define The Compatibility Population

Before changing a binding, inventory all producers and consumers:

```text
producers:
  upstream and downstream DTS
  bootloader fixups
  overlays and module EEPROM-generated fragments
  firmware-generated nodes

consumers:
  supported kernels
  bootloaders using the node for early hardware
  secure/system firmware parsers
  manufacturing and diagnostics tools
  alternative operating systems
```

The ABI population is the set of artifacts that can meet in deployed systems, not just the current source tree.

## Additive Changes Are Not Automatically Safe

Adding an optional property is usually syntactically compatible, but its semantics may not be. Ask:

- What does a new consumer do when an old DTB lacks it?
- What does an old consumer do when a new DTB contains it?
- Does ignoring it leave hardware safe?
- Does its presence contradict an old compatible fallback?
- Does a bootloader copy, delete, or rewrite it?

A newly required property normally needs either a new compatible, a safe legacy default for old DTBs, or a deployment rule that prevents incompatible pairings.

## Never Reinterpret Existing Data

Do not change an existing property's:

- unit or scale
- signedness or cell layout
- list order
- meaning of zero or absence
- referenced provider type
- polarity or flag interpretation
- applicability to an existing compatible

Changing a description and driver together conceals the ABI break. Introduce a new property or compatible and retain legacy parsing as required by the support policy.

## Migration Patterns

### Misspelled Or Poorly Named Property

1. define the corrected property
2. mark the old property deprecated in the schema
3. accept both during a transition
4. define precedence and reject conflicting values when possible
5. update in-tree DTS producers
6. retain legacy consumer support for the deployed lifetime

Do not emit both indefinitely unless a specific older consumer requires it.

### Incorrect Compatible

1. add the new identity and its precise contract
2. keep matching the old string where safe
3. update DTS and overlay producers
4. test independent update directions
5. document whether old strings are accepted forever or only under a bounded product policy

### Property Becomes Mandatory On New Hardware

Use a new compatible whose schema requires it. Permit old DTBs under old compatibles with the original absence behavior. Do not silently require the property for already deployed identities.

### Unsafe Legacy Description

If no safe interpretation exists, fail explicitly for the affected combination. A clear probe error naming the incompatible legacy description is better than guessed resources. Recovery may require updating the DTB and software atomically.

## Dual-Property Precedence

During migration, define deterministic behavior:

| old | new | result |
|---|---|---|
| absent | absent | documented legacy/default behavior or error |
| present | absent | legacy interpretation |
| absent | present | new interpretation |
| present | present, equal | accept temporarily if required |
| present | present, conflicting | reject; do not silently choose |

The binding should discourage ambiguous double encoding, and tooling should catch it when schema expressiveness permits.

## Deprecation Is Not Removal

Schema deprecation tells new producers to stop using an interface. It does not prove deployed DTBs disappeared. Consumer removal needs evidence that no supported field population can supply the old data.

Track separately:

- producer deprecation date
- last release that emitted the old form
- consumer support commitment
- product support and rollback lifetime
- telemetry or inventory evidence, if available

Upstream kernels commonly preserve old-DTB compatibility longer than a tightly controlled product stack would.

## Rollback Matters

An A/B updater can pair a new DTB with a rolled-back kernel. Signed boot chains may prevent some combinations but permit others. Test actual rollback paths rather than assuming update order.

For each artifact, record:

```text
can update independently?
can roll back independently?
who verifies compatibility?
what version/identity is observable?
what is the recovery artifact?
```

## Compatibility Gates

A practical CI matrix should include:

- oldest supported DTB with newest software
- newest DTB with oldest supported software, when allowed
- deprecated property fixtures
- absent optional-property fixtures
- conflicting legacy/new property negative tests
- overlay combinations using both old and new bases
- bootloader parsing or mutation where applicable
- suspend, recovery, and error paths affected by the change

Store representative compiled DTBs as fixtures when source rebuilds would erase historical encoding details.

## ABI Change Review Questions

- Has this compatible or property shipped outside the repository?
- Which artifacts can be mixed in the field?
- Is absence behavior unchanged for every existing compatible?
- Can an old consumer safely ignore the new data?
- Are any list positions, units, or defaults being reinterpreted?
- What is the transition precedence?
- How is rollback tested?
- What evidence would permit eventual consumer cleanup?

## Authoritative References

- [Linux binding design guidelines](https://docs.kernel.org/devicetree/bindings/writing-bindings.html)
- [Linux stable API discussion](https://docs.kernel.org/process/stable-api-nonsense.html)
- [Linux submitting Devicetree patches](https://docs.kernel.org/devicetree/bindings/submitting-patches.html)

## Continue

Proceed to [Board Revisions, Products, And Deployment Matrices](board-revisions-products-and-deployment-matrices.md).

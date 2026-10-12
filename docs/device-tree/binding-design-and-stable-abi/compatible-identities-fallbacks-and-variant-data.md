---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# Compatible Identities, Fallbacks, And Variant Data

`compatible` identifies a programming contract, not a marketing label or driver name. A compatible list is ordered from most specific to more general implementations that can operate the hardware safely. Every fallback is a compatibility assertion that must be tested.

## When A New Compatible Is Needed

Create a distinct compatible when software must distinguish hardware to operate it correctly and cannot reliably discover the difference—not because a driver, file, product name, or kernel version changed. [Compatible Evolution And Stable ABI Decisions](../driver-matching/compatible-evolution-and-stable-abi-decisions.md) lists the typical triggers and a decision matrix. This page focuses on proving fallbacks safe and on separating compatible-implied facts from per-instance properties.

## Specific-To-General Ordering

```dts
compatible = "acme,ax200-uart", "acme,ax100-uart";
```

This claims all of the following:

1. new software may recognize AX200 specifically
2. software that knows only AX100 may bind using its AX100 path
3. that older path is safe on AX200 with the properties an old consumer understands
4. reduced functionality and lifecycle operations remain acceptable

List order is ABI. Do not sort strings alphabetically or use the first match merely as a preferred driver.

## The Fallback Safety Proof

Before adding a fallback, document:

| Question | Evidence required |
|---|---|
| register compatibility | offsets, widths, reset values, and field semantics used by old code agree |
| resource compatibility | old code can operate with the clocks, resets, IRQs, supplies, and DMA it knows |
| sequence compatibility | probe, suspend, resume, reset, and recovery sequences remain safe |
| default compatibility | unrecognized new properties do not hide a mandatory action |
| error compatibility | old handling cannot wedge hardware, corrupt data, or violate ownership |
| performance limitation | reduced performance is known and acceptable |

Successful probe is weak evidence. Test transfers, interrupts under load, runtime PM, system suspend, errors, reset, removal if supported, and warm boot.

## When No Fallback Is Correct

Do not list an older compatible when old software would:

- omit a newly mandatory supply or reset sequence
- program a field with changed semantics
- use unsafe DMA addressing or coherency assumptions
- miss an erratum that can corrupt data or lock the bus
- operate a secure or firmware-owned resource directly
- appear functional until a rare recovery or power transition

Deliberate failure to bind is safer than deceptive partial compatibility.

## Compatible Versus Properties

Use compatible-specific driver match data for facts invariant across every instance of an implementation, and DT properties for per-instance integration such as provider clocks, board supplies, routed interrupts, or carrier-imposed limits ([Match Data Versus DT Properties](../driver-matching/compatible-evolution-and-stable-abi-decisions.md#match-data-versus-dt-properties)). Do not make a property like `acme,is-ax200` duplicate the compatible.

## Feature Properties Need Hardware Semantics

A feature bit property can be valid when the feature varies independently per instance, cannot be discovered, and has clear hardware meaning. It is not valid merely to avoid adding match data.

Compare:

```dts
/* Driver-shaped. */
acme,enable-v2-code-path;

/* Potentially hardware-shaped, if the binding proves the board has this wiring. */
acme,external-sample-clock;
```

Even the second proposal should first be compared with standard `clocks` and assigned-clock mechanisms.

## Root And SoC Compatible Lists

Root lists follow the same rules ([Root Versus Device Compatible](../driver-matching/compatible-contracts-and-fallback-chains.md#root-versus-device-compatible)); every fallback level must have meaningful consumers and safe semantics. A revision-specific root compatible is warranted when firmware, early platform code, or software-visible board behavior must distinguish it. Peripheral compatibles need not change when existing properties completely describe the wiring delta.

## Unknown Compatible Behavior

Consumers match any string they know, generally respecting table behavior and list order. Unknown strings are expected during evolution. The contract must not require every consumer to understand the most specific string if a valid fallback is supplied.

Conversely, adding a new property beside an old fallback does not teach an old consumer to honor it. If ignoring the property is unsafe, the fallback is invalid.

## Four-Way Test Matrix

Run the old/new DTB × old/new consumer matrix from [Compatibility In Both Directions](../driver-matching/compatible-evolution-and-stable-abi-decisions.md#compatibility-in-both-directions). Record exact versions and tests, and repeat across any bootloader or firmware component that also parses the node.

## Authoritative References

- [Devicetree Specification: `compatible`](https://devicetree-specification.readthedocs.io/en/stable/devicetree-basics.html)
- [Linux binding design guidelines](https://docs.kernel.org/devicetree/bindings/writing-bindings.html)
- [Linux driver model and Devicetree](https://docs.kernel.org/devicetree/usage-model.html)

## Continue

Proceed to [Backward Compatibility, Deprecation, And Migration](backward-compatibility-deprecation-and-migration.md).

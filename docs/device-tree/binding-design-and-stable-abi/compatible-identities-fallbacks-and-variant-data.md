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

The AM62x GPU node in `k3-am62-main.dtsi` carries four levels:

```dts
gpu: gpu@fd00000 {
        compatible = "ti,am62-gpu", "img,img-axe-1-16m", "img,img-axe",
                     "img,img-rogue";
        clocks = <&k3_clks 187 0>;
        clock-names = "core";
        power-domains = <&k3_pds 187 TI_SCI_PD_EXCLUSIVE>;
        power-domain-names = "a";
        /* reg, interrupts */
};
```

Each string makes a separate claim:

1. `ti,am62-gpu` — TI's integration. The schema uses it to limit `clocks` to one, because the number of clocks is an integration choice.
2. `img,img-axe-1-16m` — the exact IP core. The schema uses it to require exactly one power domain, because that is a property of the core.
3. `img,img-axe` — the original, too-vague family string. It is deprecated but kept so kernels that match only it still bind.
4. `img,img-rogue` — the architecture. The current `pvr_drv.c` binds on this string.

No current driver matches the first two strings. They exist so that a future quirk can target the exact integration or core without another DT change, and so that schemas can apply the right constraints.

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

Use compatible-specific driver match data for facts invariant across every instance of an implementation, and DT properties for per-instance integration such as provider clocks, board supplies, routed interrupts, or carrier-imposed limits ([Match Data Versus DT Properties](../driver-matching/compatible-evolution-and-stable-abi-decisions.md#match-data-versus-dt-properties)). Do not add a property such as a hypothetical `ti,is-am62` that duplicates the compatible.

## Feature Properties Need Hardware Semantics

A feature bit property can be valid when the feature varies independently per instance, cannot be discovered, and has clear hardware meaning. It is not valid merely to avoid adding match data.

Compare a real board-wiring property with a hypothetical driver-shaped one:

```dts
/* Real (ti,am62-usb.yaml): VBUS reaches the SoC through a 1/3 divider on this board. */
ti,vbus-divider;

/* Hypothetical and driver-shaped: names a code path, not hardware. */
ti,use-new-vbus-detect;
```

`ti,vbus-divider` varies per board, cannot be discovered, and has a precise electrical meaning. The second would select software behavior and should be match data or a driver decision.

Before accepting any vendor feature property, first check whether a standard mechanism such as `clocks`, a supply, or a GPIO already expresses it.

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

---
status: draft
reviewed: false
domain: device-tree
difficulty: intermediate
last_reviewed: null
---

# Compatible Contracts And Fallback Chains

A compatible string names a hardware programming-model contract. An ordered list says that the node is best described by the first string and can also be driven as each later, less-specific implementation.

## Ordered From Specific To General

```dts
serial@4000 {
        compatible = "acme,ax200-uart", "acme,ax100-uart";
        reg = <0x4000 0x100>;
};
```

This asserts more than family resemblance. It says software implementing the `acme,ax100-uart` contract can safely operate the AX200 block, possibly without newer features.

The first entry should identify the actual implementation. Later entries are fallbacks in decreasing specificity. Order matters to clients selecting the best supported entry.

## What Compatibility Must Cover

A safe fallback normally preserves everything old software relies on:

- register layout, width, access semantics, and reset values used by the old driver
- interrupt behavior and acknowledgement sequence
- clock, reset, power, DMA, and PHY expectations
- required initialization and ordering constraints
- old binding properties and their meanings
- behavior when new features are ignored

Matching register offsets alone is insufficient. If the new device needs an additional reset sequence, has inverted interrupt semantics, or corrupts data unless a new quirk is applied, the old driver may not be safe.

## Exact Strings, Not Patterns

Compatible strings are literal values conventionally written as `vendor,device`. They do not support wildcards such as `vendor,uart-*`, semantic-version comparisons, or substring matching. The vendor prefix must follow the binding ecosystem's registered naming rules.

Use hardware identity, not deployment or software policy:

```text
good: vendor,ax200-uart
bad:  vendor,production-uart
bad:  vendor,linux-6-12-uart
bad:  vendor,uart-driver-v3
```

The contract must remain meaningful to other operating systems and future kernels.

## Root Versus Device Compatible

The root node identifies the complete machine:

```dts
/ {
        compatible = "acme,atlas-revc", "acme,atlas", "acme,ax200";
};
```

A peripheral node identifies one hardware block:

```dts
ethernet@5000 {
        compatible = "acme,ax200-ethernet", "acme,ax100-ethernet";
};
```

The same list principles apply, but consumers differ. Root compatibles can select early platform behavior and describe board/SoC compatibility; device compatibles normally match a bus driver and binding. Do not copy a SoC root compatible onto each peripheral or put a peripheral IP compatible in the root list.

A board fallback to a SoC string is appropriate only where the architecture ecosystem defines that relationship and generic SoC support can boot the board safely. Board identity and SoC identity are not automatically interchangeable.

## How Linux Uses A List

Linux OF matching compares a node's compatible strings with a driver's `struct of_device_id` table. The OF matching helpers prefer a table entry matching an earlier, more-specific string in the node's list.

That preference operates within a candidate match table. It is not a global arbitration system for competing drivers. The driver core checks registered drivers according to the bus model; two unrelated drivers should not claim overlapping compatible contracts and expect list specificity to choose the intended one.

## Fallbacks Enable Old Software

Suppose an older kernel knows only:

```c
{ .compatible = "acme,ax100-uart", .data = &ax100_data },
```

The AX200 node's fallback allows that driver to bind. A newer kernel can add:

```c
{ .compatible = "acme,ax200-uart", .data = &ax200_data },
```

and select AX200-specific behavior. This only works if AX100 behavior is a safe subset. A fallback must never be added merely to make an old kernel bind; an explicit non-match is safer than silent misprogramming.

## Generic Compatibles

Some standardized devices have legitimate generic contracts such as `fixed-clock`, `regulator-fixed`, or `gpio-keys`. These names are defined by their bindings. Do not invent a generic value such as `generic-uart` or use `syscon` alone to avoid documenting the real programming model.

Generic fallbacks can constrain future evolution. Once deployed, old software may bind on that promise. Review them as long-lived ABI, not as convenience labels.

## Review Method

For each list:

1. Find the schema that permits the exact sequence.
2. Identify the real hardware named by the first string.
3. For every fallback, name the old driver behavior expected to work.
4. Compare registers, required resources, errata, and initialization ordering.
5. Test the new DTB with the oldest supported kernel and the old DTB with the newest kernel.
6. Remove any fallback whose safety cannot be demonstrated.

## Common Failures

- Listing sibling devices that share a vendor but are not backward-compatible.
- Ordering a generic fallback before the actual implementation.
- Adding a fallback solely to suppress “no driver” symptoms.
- Treating marketing family names as programming contracts.
- Using a software version or board role in a compatible string.
- Allowing two drivers to claim the same compatible and relying on registration order.

## Authoritative References

- [Devicetree Specification: `compatible`](https://devicetree-specification.readthedocs.io/en/stable/devicetree-basics.html)
- [Linux binding design guidance](https://docs.kernel.org/devicetree/bindings/writing-bindings.html)
- [Linux DeviceTree matching APIs](https://docs.kernel.org/devicetree/kernel-api.html)
- [Linux driver-core binding model](https://docs.kernel.org/driver-api/driver-model/binding.html)

## Next Step

Continue with [Compatible Evolution And Stable ABI Decisions](compatible-evolution-and-stable-abi-decisions.md).

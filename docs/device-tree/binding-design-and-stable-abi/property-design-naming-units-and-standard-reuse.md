---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# Property Design, Naming, Units, And Standard Reuse

Every new property creates vocabulary that producers and consumers may need to support for years. Reusing a standard property preserves shared semantics, tooling, and cross-OS understanding. A custom property needs a stronger justification than convenience.

## Search Before Inventing

Search in this order:

1. Devicetree Specification standard properties
2. core and subsystem common schemas
3. bindings for the same hardware class
4. provider bindings for clocks, resets, GPIOs, DMA, IOMMUs, PHYs, power, and interconnects
5. vendor bindings only when the fact is genuinely vendor-specific

Search by meaning, not just the proposed spelling. `clock-frequency`, `max-frequency`, and a clock provider relationship have different semantics even though all involve hertz.

## Standard Relationship Before Scalar

Do not replace a resource relationship with a scalar or boolean:

```dts
/* Weak: loses provider, identity, and topology. */
vendor,clock-rate = <24000000>;
vendor,has-reset;

/* Strong: names actual hardware relationships. */
clocks = <&osc24m>, <&ccu 7>;
clock-names = "ref", "bus";
resets = <&resetc 12>;
reset-names = "core";
```

A consumer may constrain or request a clock rate, but the clock specifier still identifies the provider and line. Use the subsystem's established semantics.

## Names Describe Meaning

Property names should be lowercase and hyphen-separated. Vendor-specific names require the registered vendor prefix. Use a generic name when the semantics are generic and standardized; do not omit the prefix from an ad hoc property to make it look standard.

Prefer names such as:

- `startup-delay-us` for a time duration when that standard semantic applies
- `foo-supply` for a regulator supply named by the binding
- `reset-gpios` for GPIO-based reset when defined by the device binding
- `phys` with `phy-names` for PHY relationships

Avoid names such as:

- `enable-feature` when it really describes presence or wiring
- `use-fast-mode` when the value is policy
- `driver-buffer-size`
- `vendor,magic-values`
- names containing a Linux function or configuration symbol

## Standard Unit Suffixes

Use defined unit suffixes and state the physical meaning. Common suffixes include:

| Suffix | Meaning | Example question to settle |
|---|---|---|
| `-hz` | frequency in hertz | input, output, maximum, or exact? |
| `-microvolt` | electric potential | nominal, minimum, or maximum? |
| `-microamp` | electric current | operating limit or measured value? |
| `-ohms` | resistance | effective or component value? |
| `-celsius` | temperature | signed? absolute or offset? |
| `-ms`, `-us`, `-ns` | duration | minimum delay, maximum latency, or exact time? |
| `-bits` | bit count | width, mask size, or payload size? |

Do not invent implicit scaling. A value of `25` must not silently mean 2.5 V, 25 MHz, or 25 microseconds. The property description must define range, inclusivity, rounding, and whether zero has special meaning.

## Type And Cardinality Are Semantics

Choose a type that matches the fact:

- boolean: the physical condition is either present or absent, and absence has clear meaning
- scalar: one numeric value
- array: an ordered, fixed-semantic tuple or repeated homogeneous values
- string: a defined symbolic value, not free-form driver input
- string array: an ordered set with defined vocabulary
- phandle or phandle-array: a relationship to another described entity
- child nodes: repeated entities with their own properties and identity

Do not encode multiple flags into an undocumented integer. Do not use a string where the consumer performs arbitrary parsing. Do not make a one-element array merely because the driver API returns an array.

## Defaults Are ABI

If a property is optional, absence needs one stable interpretation. A default must be:

- safe for every compatible to which it applies
- derived from hardware behavior or an established legacy contract
- documented in the binding and implemented consistently
- covered by old-DTB/new-driver tests

Driver defaults that change after a refactor expose that the property was underspecified. If no universal safe default exists, make the property required or split the compatible.

## Boolean Traps

A boolean is appropriate when presence asserts a fixed fact, such as a standard coherency property on hardware whose integration is coherent. It is poor when designers later need three states or a numeric limit.

Ask:

- Is false equivalent to absence, or is “unknown” distinct?
- Can future hardware need `auto`, `required`, and `forbidden`?
- Is the fact already implied by compatible?
- Does the property describe capability or request behavior?

Avoid paired booleans that can contradict each other.

## Vendor Prefix Decision

Use a vendor prefix when the property represents a vendor-specific hardware feature with no common abstraction. Before doing so, write why a generic property is not accurate and whether other vendors have the same concept.

A vendor prefix does not make policy or driver internals acceptable. It only namespaces a legitimate nonstandard hardware contract.

## Property Proposal Template

For each new property, record:

```text
name and vendor prefix:
hardware fact represented:
why existing standard properties do not apply:
type and exact cardinality:
unit and numeric range:
meaning of each value:
meaning when absent:
which compatibles permit/require it:
invalid combinations:
old consumer behavior:
example from a real board:
```

If any line is vague, the schema will only encode the vagueness more precisely.

## Authoritative References

- [Linux property unit suffix definitions](https://github.com/devicetree-org/dt-schema/blob/main/dtschema/schemas/property-units.yaml)
- [Linux common DT properties](https://github.com/devicetree-org/dt-schema/blob/main/dtschema/schemas/dt-core.yaml)
- [Linux vendor prefix registry](https://github.com/devicetree-org/dt-schema/blob/main/dtschema/schemas/vendor-prefixes.yaml)
- [Linux binding design guidelines](https://docs.kernel.org/devicetree/bindings/writing-bindings.html)

## Continue

Proceed to [Compatible Identities, Fallbacks, And Variant Data](compatible-identities-fallbacks-and-variant-data.md).

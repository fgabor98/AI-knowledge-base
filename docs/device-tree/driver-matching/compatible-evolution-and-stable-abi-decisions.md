---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# Compatible Evolution And Stable ABI Decisions

Device Trees and kernels are often updated independently. Compatible-string design must let a new kernel interpret old DTBs correctly and prevent an old kernel from driving new hardware unsafely.

## Start From Observable Hardware Behavior

Introduce a new compatible when software must distinguish an implementation to operate it correctly or expose its hardware accurately. Typical reasons include:

- changed register layout or field semantics
- new mandatory clock, reset, regulator, PHY, or power sequence
- an erratum requiring a distinct quirk
- changed interrupt, DMA, coherency, or endianness behavior
- feature/capability differences not safely discoverable at runtime
- incompatible binding property requirements

Do not introduce one solely because:

- the Linux driver was refactored
- the board DTS moved files
- the product acquired a new marketing name
- firmware or kernel versions changed
- a feature already discoverable from hardware is now supported by software

## Decision Matrix

| Hardware change | Compatible strategy | Fallback? |
|---|---|---|
| no software-visible difference | reuse existing compatible | not applicable |
| strict superset; old programming path remains safe | new specific compatible | old compatible may follow |
| new optional feature, safe when ignored | often new specific compatible or existing feature property, per binding | only if old behavior remains safe |
| mandatory sequence or semantic change | new compatible | generally no old fallback |
| new erratum old driver cannot avoid | new compatible | no unsafe fallback |
| board revision changes only wiring described by existing properties | update board/root identity as needed; peripheral compatible may stay | depends on whole-board compatibility |

The binding maintainer decides whether a feature belongs in compatible-specific match data, a discoverable capability, or a hardware property. Avoid properties that merely select driver code paths such as `vendor,use-new-driver`.

## Compatibility In Both Directions

Test four combinations:

| DTB | Kernel | Question |
|---|---|---|
| old | old | Does the released baseline still work? |
| old | new | Did the new driver preserve the old binding contract? |
| new | new | Are new features and requirements represented correctly? |
| new | old | Does a fallback provide safe reduced functionality, or correctly avoid binding? |

The last case determines whether a fallback is valid. “Probe runs” is not a sufficient result; exercise interrupt load, DMA, suspend/resume, error recovery, and reset paths affected by the change.

## Schema-Compatible Lists

A schema should constrain valid sequences explicitly:

```yaml
properties:
  compatible:
    oneOf:
      - const: acme,ax100-uart
      - items:
          - const: acme,ax200-uart
          - const: acme,ax100-uart
```

This documents that AX200 may fall back to AX100 and prevents reversed or invented lists. When variants have different requirements, conditional schema rules should make those differences testable.

```yaml
allOf:
  - if:
      properties:
        compatible:
          contains:
            const: acme,ax200-uart
    then:
      required:
        - resets
```

Only use this pattern if the fallback remains safe without the new kernel understanding the new property. If AX100 software cannot operate an AX200 that requires the reset, then the old fallback itself is false.

## Match Data Versus DT Properties

Driver match data is appropriate for facts implied by the compatible:

```c
struct acme_uart_data {
        u32 fifo_depth;
        bool needs_status_readback;
};
```

A DT property is appropriate for board- or instance-specific hardware not implied by the IP version, such as a routed clock source, GPIO connection, or actual FIFO size when it varies independently and the binding defines it.

Do not duplicate the same truth in both match data and an unconstrained property. Conflicting sources lead to ambiguous ownership and unsafe defaults.

## Root And Board Evolution

Board revisions need a new root compatible when software may need to distinguish them, even if most peripheral nodes are unchanged. A revision-specific first string can fall back to the earlier board only when old board-level behavior remains safe.

Examples requiring care include:

- power sequencing changed outside a peripheral's own binding
- boot storage or console moved
- GPIO polarity or regulator topology changed
- a secure-world interface or reserved-memory layout changed
- an expansion connector changed electrical capability

Existing properties may fully describe some changes, allowing common drivers without a new peripheral compatible. Root identity can still change for inventory, firmware policy, or early platform behavior.

## Deprecation And Migration

Once deployed, a misspelled or poorly designed compatible may still be ABI. Migration normally requires:

1. document the corrected/new compatible
2. teach new kernels both old and new strings where safe
3. update DTS producers to emit the new string
4. preserve old-DTB support for the product's lifetime policy
5. remove support only with explicit ecosystem agreement

Silently changing a string in DTS and driver together passes same-tree testing but breaks independent upgrades.

## Senior Review Questions

- What software-visible fact makes the new identity necessary?
- Is that fact per-implementation or per-board instance?
- Can hardware report it reliably without firmware data?
- What exact reduced behavior will an old fallback driver use?
- Which old-DTB/new-kernel and new-DTB/old-kernel combinations are supported?
- Does schema encode valid list order and variant requirements?
- Are bootloader-owned DTBs and field-upgrade sequencing included in testing?
- What is the lifetime commitment for already shipped strings?

## Authoritative References

- [Linux binding design DOs and DON'Ts](https://docs.kernel.org/devicetree/bindings/writing-bindings.html)
- [Linux schema-writing guide](https://docs.kernel.org/devicetree/bindings/writing-schema.html)
- [Devicetree Specification: compatible-list semantics](https://devicetree-specification.readthedocs.io/en/stable/devicetree-basics.html)
- [Linux SoC maintainer guidance](https://docs.kernel.org/process/maintainer-soc.html)

## Next Step

Continue with [From Device Tree Nodes To Linux Devices](from-device-tree-nodes-to-linux-devices.md).

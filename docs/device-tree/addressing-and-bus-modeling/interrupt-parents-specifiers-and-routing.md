---
status: draft
reviewed: false
domain: device-tree
difficulty: intermediate
last_reviewed: null
---

# Interrupt Parents, Specifiers, And Routing

The interrupt tree is a routing graph overlaid on the device hierarchy. `interrupt-parent` selects a controller or nexus; `interrupts` contains specifiers interpreted by that parent. The numbers are local to an interrupt domain, not global Linux IRQ numbers.

## `interrupts` Depends On A Parent

[GPIO And Interrupt Relationships](../provider-consumer-relationships/gpio-and-interrupt-relationships.md) introduced `interrupt-parent`, `interrupts`, and `interrupts-extended`. The decoding rule is:

1. Resolve the effective `interrupt-parent`.
2. Read that node's `#interrupt-cells`.
3. Split the `interrupts` stream into entries of exactly that width.
4. Apply the interrupt-controller binding to each cell; no generic rule says the cells are "number plus flags".

## Inheritance

`interrupt-parent` is inherited through the Device Tree hierarchy. A bus can define it once for descendants:

```dts
soc {
        interrupt-parent = <&intc>;

        uart@1000 {
                interrupts = <17 4>;
        };
};
```

Inheritance reduces repetition, but it can hide mistakes in layered sources. A board include that moves a node, adds a nearer parent, or changes an interrupt nexus can change the effective controller without touching `interrupts`.

During review, walk upward from the consumer in the **final** tree until the first `interrupt-parent` is found. Do not assume the root interrupt controller from architecture conventions.

## `interrupts-extended`

`interrupts-extended` bypasses inheritance by embedding a controller phandle in every entry. If both forms are present, the specification gives the extended form precedence, but a binding should normally require one unambiguous representation. `interrupt-names` maps to completed entries, not raw cells.

## Interrupt Nexuses

An interrupt nexus translates a child interrupt key through `interrupt-map`. The key can include both the child's unit address and interrupt specifier, allowing two devices using local interrupt `1` to map differently.

```text
child unit address + child interrupt specifier
        ↓ interrupt-map-mask and interrupt-map
parent phandle + parent unit address + parent interrupt specifier
```

Field widths come from the nexus and selected parent controller. See [Cross-Cutting Standard Relationships](../standard-nodes-and-properties/cross-cutting-standard-relationships.md#interrupt-nexus-mapping) for row mechanics.

Address translation and interrupt translation are independent. The child unit-address portion used for interrupt matching follows the interrupt binding's rules; it must not be replaced casually with an already translated CPU physical address.

## Cascaded Controllers

A GPIO controller can also be an interrupt controller whose own parent interrupt connects it to an upstream controller, forming a chain of IRQ domains. Linux assigns virtual IRQ numbers dynamically, so a number in `/proc/interrupts` need not match the hardware number stored in Device Tree. [GPIO Interrupts, Cascades, And Shared Lines](../pinctrl-gpios-and-interrupts/gpio-interrupts-cascades-and-shared-lines.md) traces a full cascade.

## INTx Versus MSI

PCI legacy INTx is pin-based and commonly routed through `interrupt-map`. MSI and MSI-X are memory-write-based messages and use MSI-controller relationships such as `msi-parent` or bus mappings such as `msi-map`. A correct INTx map does not configure MSI, and MSI success can hide a broken fallback INTx path.

## Runtime Diagnosis

For a missing or storming interrupt:

1. Confirm the device generated the signal electrically and internally.
2. Resolve the effective parent and decode the specifier binding.
3. Follow every nexus and cascaded controller.
4. Check pin configuration, polarity, trigger type, masks, and wake routing.
5. Compare controller-local hardware IDs with Linux IRQ-domain mappings.
6. Inspect `/proc/interrupts`, debugfs IRQ data when enabled, and controller registers.
7. Test inactive, active, clear, mask, and repeated-event behavior.

Polarity and trigger-type errors often produce one successful event followed by silence, or a continuous interrupt storm. Do not “fix” them by changing driver flags when the hardware wiring belongs in Device Tree.

## Senior Review Checklist

- Does every specifier use the correct controller binding and revision?
- Is inherited `interrupt-parent` still correct after DTS composition?
- Are named interrupts ordered exactly as the consumer binding requires?
- Are cascaded-controller parent interrupts described independently from child lines?
- Do suspend/wake paths use the same or a separate always-on controller?
- Are PCI INTx and MSI fallback paths both validated?
- Can diagnostic tooling map Linux IRQs back to domain and hardware IRQ without assuming equality?

## Authoritative References

- [Devicetree Specification: interrupt properties and mappings](https://devicetree-specification.readthedocs.io/en/stable/devicetree-basics.html)
- [Linux IRQ-domain documentation](https://docs.kernel.org/core-api/irq/irq-domain.html)
- [Linux DeviceTree interrupt APIs](https://docs.kernel.org/devicetree/kernel-api.html)
- [Linux generic IRQ handling](https://docs.kernel.org/core-api/genericirq.html)

## Next Step

Continue with [Bus-Specific Child Addressing](bus-specific-child-addressing.md).

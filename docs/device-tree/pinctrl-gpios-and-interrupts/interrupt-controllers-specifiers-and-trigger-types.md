---
status: draft
reviewed: false
domain: device-tree
difficulty: intermediate
last_reviewed: null
---

# Interrupt Controllers, Specifiers, And Trigger Types

An interrupt-controller node defines a local hardware interrupt namespace. A consumer's specifier is meaningful only through that controller's binding; the final Linux IRQ number is allocated separately by the IRQ-domain subsystem.

## Controller Contract

```dts
intc: interrupt-controller@2000 {
        compatible = "example,trainer-intc";
        reg = <0x2000 0x100>;
        interrupt-controller;
        #interrupt-cells = <2>;
};
```

`interrupt-controller` marks a provider and `#interrupt-cells` determines each child specifier's width. A two-cell binding might encode hardware line plus trigger type; another controller may need an interrupt class, CPU affinity, bank, or other fields.

Never infer cell meaning from count alone. Read the controller schema and any referenced common binding.

## Consumer Forms

With an inherited or explicit parent:

```dts
device@3000 {
        interrupt-parent = <&intc>;
        interrupts = <17 IRQ_TYPE_LEVEL_HIGH>;
};
```

With the parent embedded per entry:

```dts
interrupts-extended = <&intc 17 IRQ_TYPE_LEVEL_HIGH>;
```

`interrupts-extended` is useful for mixed parents. `interrupt-names` labels completed entries positionally. See [Interrupt Parents, Specifiers, And Routing](../addressing-and-bus-modeling/interrupt-parents-specifiers-and-routing.md) for inheritance and nexus mechanics.

## Trigger Types

Common flags are:

| Flag | Hardware condition |
|---|---|
| `IRQ_TYPE_EDGE_RISING` | low-to-high transition |
| `IRQ_TYPE_EDGE_FALLING` | high-to-low transition |
| `IRQ_TYPE_EDGE_BOTH` | either transition |
| `IRQ_TYPE_LEVEL_HIGH` | high level remains asserted |
| `IRQ_TYPE_LEVEL_LOW` | low level remains asserted |

These constants apply where the controller binding uses the common encoding. Some bindings add controller-specific cells or restrictions.

Trigger type is about signal behavior at the interrupt controller input after any inversion. It is not derived mechanically from whether a device pin is named `IRQ#` or a GPIO consumer is active-low.

## Choose Edge Or Level From The Device Protocol

Use level triggering when the device holds the line active until software removes the cause. A correct handler generally:

1. reads status
2. services every active source
3. clears or masks the device condition
4. returns only after the line can deassert

If the condition remains active, the controller will present the interrupt again. This is desirable for lossless shared or latched conditions but becomes a storm when polarity or acknowledgement is wrong.

Use edge triggering when the event is a pulse or transition and the hardware/controller can capture it reliably. Edge events can be lost while masked or suspended unless the source or controller latches them.

Both-edge support may be implemented in hardware or emulated and can race with rapidly changing signals. Validate the controller binding and rate limits.

## Physical Polarity Versus Trigger Condition

An active-low level interrupt normally uses `IRQ_TYPE_LEVEL_LOW`; an active-low pulse normally produces a falling edge on assertion and may use `IRQ_TYPE_EDGE_FALLING`. But intermediate inverters, GPIO controller configuration, and device mode can change what arrives at the controller.

Document:

- inactive voltage
- assertion transition or level
- whether the source latches the event
- how software clears it
- whether multiple sources share the net
- minimum pulse width and debounce/filtering

Then choose the controller-visible trigger.

## Linux IRQ Numbers Are Not DT IDs

Linux IRQ domains translate controller-local hardware IRQs into Linux virtual IRQ numbers. `/proc/interrupts` may show a number unrelated to the DT cell.

Keep these columns separate during diagnosis:

| Identity | Example |
|---|---:|
| device status bit | bit 3 |
| GPIO offset | 7 |
| GPIO parent hardware IRQ | 42 |
| root-controller hardware IRQ | 89 |
| Linux virtual IRQ | 126 |

Comparing `interrupts = <42 ...>` directly with line 42 of `/proc/interrupts` is generally invalid.

## Interrupt Names And Driver Lookup

```dts
interrupts = <17 IRQ_TYPE_LEVEL_HIGH>,
             <18 IRQ_TYPE_EDGE_RISING>;
interrupt-names = "fault", "complete";
```

The binding defines the names and order. Drivers can request by name, avoiding fragile numeric indices. Reordering either list independently silently swaps semantics.

## Failure Signatures

| Symptom | Likely category |
|---|---|
| no counter increment | wrong parent/line, masked source, mux/input problem |
| one interrupt then silence | edge/level mismatch, uncleared status, disabled source |
| immediate storm | wrong polarity, level stuck active, bad acknowledge order |
| events only under polling | source works but route/controller does not |
| wrong device handler fires | specifier or nexus mapping error |
| counter increases but driver sees no work | stale/shared interrupt or status decode issue |

## Authoritative References

- [Devicetree Specification: interrupt properties](https://devicetree-specification.readthedocs.io/en/stable/devicetree-basics.html)
- [Linux common IRQ trigger constants](https://github.com/torvalds/linux/blob/master/include/dt-bindings/interrupt-controller/irq.h)
- [Linux generic IRQ handling](https://docs.kernel.org/core-api/genericirq.html)
- [Linux IRQ-domain documentation](https://docs.kernel.org/core-api/irq/irq-domain.html)

## Next Step

Continue with [GPIO Interrupts, Cascades, And Shared Lines](gpio-interrupts-cascades-and-shared-lines.md).

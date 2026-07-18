---
status: draft
reviewed: false
domain: device-tree
difficulty: intermediate
last_reviewed: null
---

# GPIO And Interrupt Relationships

GPIO and interrupt specifiers often refer to the same physical controller, but they represent different hardware functions and are decoded through different domains.

## GPIO Providers And Consumers

A GPIO controller identifies itself and declares its specifier size:

```dts
gpio0: gpio-controller@b000 {
        compatible = "example,trainer-gpio";
        gpio-controller;
        #gpio-cells = <2>;
};
```

A consumer property uses the conventional `<function>-gpios` form:

```dts
ethernet@2000 {
        reset-gpios = <&gpio0 12 GPIO_ACTIVE_LOW>;
};
```

For a conventional two-cell GPIO binding, the provider binding may define:

```text
cell 0: line offset within this controller
cell 1: electrical/logical flags
```

That layout is common, not universal. Read the GPIO controller binding. Some providers add a bank or instance cell.

## Function Names And GPIO Lookup

Linux GPIO lookup derives the property name from the driver's connection ID:

```c
reset = devm_gpiod_get(dev, "reset", GPIOD_OUT_LOW);
```

For Device Tree this normally finds `reset-gpios`. An indexed getter selects entries from a multi-GPIO property. The connection ID describes the signal's function at the consumer; it should not be a global GPIO number or provider pin name.

GPIO flags describe wiring semantics such as active-low. The driver works with logical active/inactive values through the descriptor API; it should not compensate by manually inverting a correctly described line.

## GPIO Is Not Pinctrl

These relationships answer different questions:

| Relationship | Question |
|---|---|
| GPIO specifier | Which controller line performs this consumer function, with what flags? |
| pinctrl state | How should SoC pins be muxed and electrically configured? |

A reset GPIO can be correct while its pad remains muxed to another peripheral. Conversely, a pin can be configured as GPIO without being assigned to this consumer.

## Interrupt Controllers

An interrupt controller declares:

```dts
intc: interrupt-controller@c000 {
        compatible = "example,trainer-intc";
        interrupt-controller;
        #interrupt-cells = <2>;
};
```

The provider binding defines the interrupt specifier. A common two-cell form is hardware interrupt number plus trigger flags, but other controllers use three or more cells for type, number, flags, CPU routing, or hierarchy data.

## `interrupt-parent` And `interrupts`

The traditional form separates provider selection from provider arguments:

```dts
device@3000 {
        interrupt-parent = <&intc>;
        interrupts = <17 IRQ_TYPE_LEVEL_HIGH>;
};
```

`interrupts` does not begin with a phandle. Its entry width comes from the selected interrupt parent's `#interrupt-cells`. `interrupt-parent` can be inherited through the Device Tree interrupt topology, so the property may be absent on the immediate consumer.

Do not decode `interrupts` using the structural parent unless the interrupt-parent rules actually select it.

## `interrupts-extended`

The extended form places a provider phandle in every entry:

```dts
interrupts-extended = <&intc 17 IRQ_TYPE_LEVEL_HIGH>,
                      <&gpio0 6 IRQ_TYPE_EDGE_FALLING>;
interrupt-names = "core", "wakeup";
```

This supports entries from different interrupt domains. Each tuple uses the referenced provider's `#interrupt-cells`, so entry widths can differ.

Do not normally combine `interrupts` and `interrupts-extended` for the same resource list unless the binding explicitly defines compatibility behavior.

## A GPIO Controller Can Also Be An Interrupt Controller

One hardware block can advertise both roles:

```dts
gpio0: gpio-controller@b000 {
        gpio-controller;
        #gpio-cells = <2>;
        interrupt-controller;
        #interrupt-cells = <2>;
};
```

These consumer properties use separate namespaces:

```dts
enable-gpios = <&gpio0 6 GPIO_ACTIVE_HIGH>;
interrupts-extended = <&gpio0 6 IRQ_TYPE_EDGE_RISING>;
```

The same line can sometimes serve both roles, but the hardware and bindings decide whether that is valid. `#gpio-cells` never decodes an interrupt specifier.

## Trigger Flags Describe Hardware Signaling

Interrupt trigger values specify level/edge and polarity semantics defined by the interrupt binding. They must match the electrical signal and controller behavior. Guessing a trigger can cause:

- an interrupt storm
- one event followed by permanent silence
- missed short pulses
- a handler running continuously because the source remains asserted
- wake failures

Use binding constants from the relevant `dt-bindings/interrupt-controller/...` header and verify them against the schematic and device datasheet.

## Interrupt Names And Driver Lookup

Multiple interrupts should have stable semantic names:

```dts
interrupts = <17 IRQ_TYPE_LEVEL_HIGH>,
             <18 IRQ_TYPE_EDGE_RISING>;
interrupt-names = "core", "wakeup";
```

Drivers can request a named IRQ through platform helpers. As with clocks and resets, the names list maps positionally and forms part of the binding ABI.

## Debugging Checklist

1. Determine whether the consumer uses `interrupts` or `interrupts-extended`.
2. Resolve the effective interrupt parent for each entry.
3. Read the provider's `#interrupt-cells` and binding.
4. Decode hardware number and flags without assuming a universal layout.
5. For GPIO-backed IRQs, verify both GPIO-controller and irqchip registration.
6. Confirm pinmux, direction, pull configuration, and electrical polarity.
7. Inspect runtime DT, `/proc/interrupts`, and driver logs.
8. Distinguish “IRQ mapped” from “hardware is producing the expected signal.”

## Common Errors

- Parsing `interrupts` as though its first cell were a phandle.
- Using `#gpio-cells` to decode `interrupts-extended` from a combined GPIO/IRQ provider.
- Confusing `GPIO_ACTIVE_LOW` with an interrupt trigger flag.
- Fixing active-low behavior by driver inversion instead of describing wiring.
- Assuming a GPIO assignment configures pinmux.
- Omitting `interrupt-names` while a driver requests a named IRQ.
- Copying a GIC specifier shape to a GPIO interrupt controller.

## Exercises

1. Decode two `interrupts-extended` entries whose providers use two and three cells.
2. Explain why `reset-gpios` maps to connection ID `reset`.
3. Separate polarity semantics of GPIO output and IRQ trigger configuration.
4. List the additional checks for an interrupt supplied by an I2C GPIO expander.
5. Explain why the same numeric line can appear in both GPIO and interrupt specifiers without making the specifiers interchangeable.

## References And Next Step

- [Devicetree interrupt model](https://devicetree-specification.readthedocs.io/en/stable/devicetree-basics.html#interrupts-and-interrupt-mapping)
- [Linux GPIO mappings](https://docs.kernel.org/driver-api/gpio/board.html#device-tree)
- [Linux GPIO consumer interface](https://docs.kernel.org/driver-api/gpio/consumer.html)
- [Linux IRQ domain documentation](https://docs.kernel.org/core-api/irq/irq-domain.html)

Continue with [DMA, IOMMU, PHY, And Regulator Dependencies](dma-iommu-phy-and-regulator-dependencies.md).

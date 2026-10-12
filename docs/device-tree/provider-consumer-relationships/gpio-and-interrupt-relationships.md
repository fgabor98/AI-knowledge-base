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

A GPIO controller identifies itself and declares its specifier size. AM64x has SoC GPIO banks and, on the EVM, an I2C GPIO expander:

```dts
/* k3-am64-main.dtsi */
main_gpio0: gpio@600000 {
        compatible = "ti,am64-gpio", "ti,keystone-gpio";
        gpio-controller;
        #gpio-cells = <2>;
        /* ... */
};

/* k3-am642-evm.dts, on main_i2c1 */
exp1: gpio@22 {
        compatible = "ti,tca6424";
        reg = <0x22>;
        gpio-controller;
        #gpio-cells = <2>;
        gpio-line-names = /* ... */ "GPIO_PCIe_RST_OUT", /* ... */ "GPIO_OLED_RESETn", /* ... */;
};
```

A consumer property uses the conventional `<function>-gpios` form. Two EVM consumers of the same expander:

```dts
display@3c {
        compatible = "solomon,ssd1306fb-i2c";
        reset-gpios = <&exp1 14 GPIO_ACTIVE_LOW>;
};

&pcie0_rc {
        reset-gpios = <&exp1 5 GPIO_ACTIVE_HIGH>;
};
```

Both providers use the conventional two-cell layout:

```text
cell 0: line offset within this controller
cell 1: flags from dt-bindings/gpio/gpio.h
```

That layout is common, not universal. Read the GPIO controller binding. Some providers add a bank or instance cell.

## Function Names And GPIO Lookup

Linux GPIO lookup derives the property name from the driver's connection ID. Both drivers above use the same call:

```c
/* drivers/gpu/drm/solomon/ssd130x.c and drivers/pci/controller/cadence/pci-j721e.c */
reset = devm_gpiod_get_optional(dev, "reset", GPIOD_OUT_LOW);
```

For Device Tree this finds `reset-gpios`. The connection ID describes the signal's function at the consumer; it is not a global GPIO number or provider pin name.

GPIO flags describe wiring semantics. The OLED reset is active-low and the PCIe reset output active-high, yet both drivers request `GPIOD_OUT_LOW` (logically deasserted) and work with logical values; neither inverts the line in code.

## GPIO Is Not Pinctrl

These relationships answer different questions:

| Relationship | Question |
|---|---|
| GPIO specifier | Which controller line performs this consumer function, with what flags? |
| pinctrl state | How should SoC pins be muxed and electrically configured? |

A reset GPIO can be correct while its pad remains muxed to another peripheral. Conversely, a pin can be configured as GPIO without being assigned to this consumer.

## Interrupt Controllers

An interrupt controller declares `interrupt-controller` and `#interrupt-cells`. AM64x chains three kinds (`k3-am64-main.dtsi`):

```dts
gic500: interrupt-controller@1800000 {
        compatible = "arm,gic-v3";
        interrupt-controller;
        #interrupt-cells = <3>;           /* type, number, trigger flags */
};

main_gpio_intr: interrupt-controller@a00000 {
        compatible = "ti,sci-intr";       /* interrupt router */
        interrupt-controller;
        interrupt-parent = <&gic500>;
        #interrupt-cells = <1>;           /* router input */
};

main_gpio0: gpio@600000 {
        interrupt-parent = <&main_gpio_intr>;
        interrupts = <190>, <191>, <192>, <193>, <194>, <195>;
        interrupt-controller;
        #interrupt-cells = <2>;           /* GPIO line, trigger flags */
};
```

The provider binding defines the specifier. Three, one, and two cells appear on one SoC.

## `interrupt-parent` And `interrupts`

The traditional form separates provider selection from provider arguments. The AM642 SK Wi-Fi module's interrupt (`k3-am642-sk.dts`):

```dts
wlcore: wlcore@2 {
        compatible = "ti,wl1837";
        interrupt-parent = <&main_gpio0>;
        interrupts = <46 IRQ_TYPE_EDGE_FALLING>;
};
```

`interrupts` does not begin with a phandle. Its entry width comes from the selected interrupt parent's `#interrupt-cells`. `interrupt-parent` can also be inherited: the AM64x root node sets `interrupt-parent = <&gic500>;`, so `main_uart0` simply writes `interrupts = <GIC_SPI 178 IRQ_TYPE_LEVEL_HIGH>;` with three cells.

Do not decode `interrupts` using the structural parent unless the interrupt-parent rules actually select it.

## `interrupts-extended`

The extended form places a provider phandle in every entry, so entries can come from different interrupt domains with different widths. The BeagleBoard-X15 UART3 has a normal interrupt and a pad wake-up interrupt (`arch/arm/boot/dts/ti/omap/am57xx-beagle-x15-common.dtsi`):

```dts
&uart3 {
        interrupts-extended = <&crossbar_mpu GIC_SPI 69 IRQ_TYPE_LEVEL_HIGH>,
                              <&dra7_pmx_core 0x3f8>;
};
```

`crossbar_mpu` (`ti,irq-crossbar`) uses three cells; `dra7_pmx_core` (`pinctrl-single` acting as a wake-up interrupt controller) uses one, the pad register offset. The `8250_omap.yaml` binding documents the second entry as the optional system wake-up IRQ.

Do not normally combine `interrupts` and `interrupts-extended` for the same resource list unless the binding explicitly defines compatibility behavior.

## A GPIO Controller Can Also Be An Interrupt Controller

`main_gpio0` above advertises both roles. Consumers use separate namespaces on the same block. On the AM642 SK:

```dts
wlan_en: regulator-4 {
        gpio = <&main_gpio0 48 GPIO_ACTIVE_HIGH>;      /* GPIO role */
};

wlcore: wlcore@2 {
        interrupt-parent = <&main_gpio0>;
        interrupts = <46 IRQ_TYPE_EDGE_FALLING>;      /* interrupt role */
};
```

Both happen to use two cells with a line number first, but the second cells come from different headers (`gpio.h` versus `irq.h`). `#gpio-cells` never decodes an interrupt specifier.

## Trigger Flags Describe Hardware Signaling

Interrupt trigger values specify level/edge and polarity semantics defined by the interrupt binding. They must match the electrical signal and controller behavior. Guessing a trigger can cause:

- an interrupt storm
- one event followed by permanent silence
- missed short pulses
- a handler running continuously because the source remains asserted
- wake failures

Use binding constants from the relevant `dt-bindings/interrupt-controller/...` header and verify them against the schematic and device datasheet.

## Interrupt Names And Driver Lookup

Multiple interrupts should have stable semantic names. The AM64x MCAN node lists `interrupt-names = "int0", "int1";` and `drivers/net/can/m_can/m_can_platform.c` requests `platform_get_irq_byname(pdev, "int0")`. As with clocks and resets, the names map positionally and form part of the binding ABI.

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

1. Decode both `interrupts-extended` entries of the BeagleBoard-X15 `uart3`, whose providers use three and one cells.
2. Explain why `reset-gpios` maps to connection ID `reset`.
3. Separate polarity semantics of GPIO output and IRQ trigger configuration.
4. List the additional checks for an interrupt supplied by an I2C GPIO expander, such as the AM62x SK HDMI bridge whose `interrupt-parent` is the `exp1` expander.
5. Explain why the same numeric line can appear in both GPIO and interrupt specifiers without making the specifiers interchangeable.

## References And Next Step

- [Devicetree interrupt model](https://devicetree-specification.readthedocs.io/en/stable/devicetree-basics.html#interrupts-and-interrupt-mapping)
- [Linux GPIO mappings](https://docs.kernel.org/driver-api/gpio/board.html#device-tree)
- [Linux GPIO consumer interface](https://docs.kernel.org/driver-api/gpio/consumer.html)
- [Linux IRQ domain documentation](https://docs.kernel.org/core-api/irq/irq-domain.html)

Continue with [DMA, IOMMU, PHY, And Regulator Dependencies](dma-iommu-phy-and-regulator-dependencies.md).

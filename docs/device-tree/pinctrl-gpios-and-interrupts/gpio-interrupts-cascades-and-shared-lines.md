---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# GPIO Interrupts, Cascades, And Shared Lines

A GPIO controller can be both an interrupt consumer and an interrupt provider. A peripheral interrupt first enters the GPIO controller's local domain, then travels through the GPIO block's parent interrupt to an upstream controller.

## Two Roles In One Node

The AM625 SK HDMI bridge interrupt passes through two GPIO controllers before reaching the GIC (`k3-am625-sk-common.dtsi`, `k3-am62x-sk-common.dtsi`, `k3-am62-main.dtsi`, trimmed):

```dts
/* SoC GPIO bank: consumer of the interrupt router, provider of GPIO IRQs */
main_gpio1: gpio@601000 {
        gpio-controller;
        #gpio-cells = <2>;
        interrupt-parent = <&main_gpio_intr>;
        interrupts = <180>, <181>, <182>, <183>, <184>, <185>;
        interrupt-controller;
        #interrupt-cells = <2>;
};

/* I2C expander: consumer of main_gpio1 line 23, provider of 24 IRQ lines */
exp1: gpio@22 {
        compatible = "ti,tca6424";
        reg = <0x22>;
        pinctrl-names = "default";
        pinctrl-0 = <&main_gpio1_ioexp_intr_pins_default>;
        interrupt-parent = <&main_gpio1>;
        interrupts = <23 IRQ_TYPE_EDGE_FALLING>;
        interrupt-controller;
        #interrupt-cells = <2>;
        gpio-controller;
        #gpio-cells = <2>;
};

/* HDMI bridge: consumer of expander line 16 */
sii9022: bridge-hdmi@3b {
        compatible = "sil,sii9022";
        reg = <0x3b>;
        interrupt-parent = <&exp1>;
        interrupts = <16 IRQ_TYPE_EDGE_FALLING>;
};
```

Read the directions carefully:

- Expander line 16 is the bridge's interrupt input to `exp1`.
- Main GPIO1 line 23 is `exp1`'s interrupt output toward `main_gpio1`, which also needs a pinctrl state selecting GPIO input on that pad.
- `main_gpio1` banks reach the GIC through the TI-SCI interrupt router `main_gpio_intr` (router inputs 180–185).
- Each controller consumes its parent interrupt while providing child interrupts.

Some GPIO controllers have one parent per bank, one per line, or a hierarchical mapping. Follow that controller binding instead of assuming one shared parent.

## Cascaded IRQ-Domain Flow

```text
SiI9022 hot-plug/status event
  ↓ HDMI_INTn falls
TCA6424 input 16 latches; expander INT output falls
  ↓ main_gpio1 line 23 falling-edge detect (pad muxed as GPIO input)
main_gpio1 bank interrupt -> main_gpio_intr router -> GIC SPI
  ↓ gpio-davinci irqchip dispatches line 23
threaded pca953x handler reads expander status over I2C
  ↓ dispatches child line 16
sii902x handler clears the bridge status
```

A failure at either GPIO provider affects every child: an expander that loses I2C access silences all 24 of its interrupt lines. If the parent counter never increases, inspect GPIO bank masks, parent wiring, power, and the parent specifier. If the parent increments but no child dispatch occurs, inspect line-to-bank mapping and GPIO irqchip status.

## Pinctrl Still Matters

An interrupt described through `gpio0` does not automatically mux the package pad to GPIO input on every SoC. The consumer or board may need a pinctrl state selecting GPIO function, input enable, and a stable bias.

For a falling-edge source with no external pull-up, an internal pull-up may be necessary to establish the inactive level. For a push-pull source, an unwanted internal pull can increase current or distort edges. Use the schematic and electrical specification.

## Do Not Duplicate The Same Line Casually

A peripheral binding may describe an interrupt with `interrupts` and separately expose a GPIO property for a different function. Do not add an invented `irq-gpios` property alongside `interrupts` for the same line unless the binding explicitly requires it.

The IRQ subsystem can often map a GPIO-backed interrupt without the consumer requesting the line as a GPIO descriptor. Simultaneous GPIO and IRQ ownership depends on controller design and driver APIs. Double-requesting can fail or disturb the irqchip configuration.

## Shared Physical Interrupt Lines

Several devices may share a wired-OR/open-drain active-low interrupt net. The board needs a pull-up and every participant must release the line when inactive. The upstream controller typically sees a level-low interrupt.

Every handler on a shared Linux IRQ must determine whether its device asserted the line and return the correct result. More importantly, each source must clear its condition; otherwise the shared line remains low and all handlers repeat.

Device Tree representation is binding- and topology-specific. Multiple consumers may reference the same controller specifier, but hardware and drivers must support sharing. Do not infer shareability merely because the numeric interrupt is identical.

## Edge Loss And Level Storms

### Edge loss

An edge can occur while its parent bank is masked, during suspend, or before the child handler is installed. Determine whether the GPIO controller latches pending edges and whether the driver replays them after unmask.

### Level storm

A level source stays pending until the line deasserts. Typical causes of a storm include:

- wrong active level
- device status not cleared
- clear operation requires a readback or ordering barrier
- another device still asserts a shared line
- pad floats to the active level
- GPIO irqchip acknowledgement order conflicts with hardware

Masking the IRQ permanently is not a fix; it discards the notification path.

## GPIO Expanders As IRQ Controllers

An I2C/SPI GPIO expander may provide child interrupts through one host GPIO interrupt. The path then includes a slow bus transaction in the threaded handler:

```text
peripheral → expander GPIO → expander INT pin
→ SoC GPIO irqchip → root IRQ controller → threaded expander handler
→ I2C/SPI status read → peripheral child IRQ
```

Probe order, bus availability, runtime PM, and sleepability become part of interrupt delivery. The expander's parent line must be configured before its children can receive IRQs.

## Diagnostic Ladder

1. Verify the device's status bit or interrupt output with datasheet-safe access.
2. Observe the physical net and inactive/asserted voltages.
3. Check pinmux, input enable, and bias.
4. Inspect GPIO input and pending/mask state.
5. Check the GPIO block's parent interrupt counter.
6. Inspect child IRQ-domain mapping and `/proc/interrupts`.
7. Confirm handler entry, status acknowledgement, and line deassertion.
8. Repeat under load, suspend/resume, and shared-source conditions.

## Senior Review Checklist

- Are provider and consumer sides of the GPIO irqchip described separately?
- Does every parent bank interrupt cover the intended line offsets?
- Is the pad configured as an input with an electrically valid inactive state?
- Does the trigger match pulse/level behavior after board inversion?
- Can pending events survive masking and suspend?
- Are shared lines electrically open-drain/open-collector and software-shareable?
- Do expander IRQ handlers use sleepable/threaded paths correctly?
- Can one failed child leave an entire parent bank storming or masked?

## Authoritative References

- [Linux GPIO irqchip provider interface](https://docs.kernel.org/driver-api/gpio/driver.html)
- [Linux IRQ-domain hierarchy](https://docs.kernel.org/core-api/irq/irq-domain.html)
- [Linux generic IRQ handling](https://docs.kernel.org/core-api/genericirq.html)
- [Linux pinctrl and GPIO coordination](https://docs.kernel.org/driver-api/pin-control.html)

## Next Step

Continue with [Sleep States, Wakeup, And Ownership](sleep-states-wakeup-and-ownership.md).

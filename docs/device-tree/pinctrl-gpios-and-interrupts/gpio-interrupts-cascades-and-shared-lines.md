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

```dts
gpio0: gpio-controller@1000 {
        compatible = "example,trainer-gpio";
        reg = <0x1000 0x100>;

        gpio-controller;
        #gpio-cells = <2>;

        interrupt-controller;
        #interrupt-cells = <2>;
        interrupt-parent = <&gic>;
        interrupts = <42 IRQ_TYPE_LEVEL_HIGH>;
};

sensor@48 {
        interrupt-parent = <&gpio0>;
        interrupts = <7 IRQ_TYPE_EDGE_FALLING>;
};
```

Read the directions carefully:

- GPIO offset 7 is the sensor's interrupt input to `gpio0`.
- Hardware interrupt 42 is `gpio0`'s output toward `gic`.
- `gpio0` consumes its parent interrupt while providing child interrupt 7.

Some GPIO controllers have one parent per bank, one per line, or a hierarchical mapping. Follow that controller binding instead of assuming one shared parent.

## Cascaded IRQ-Domain Flow

```text
sensor condition
  ↓ board trace and pad input configuration
GPIO line 7 pending bit
  ↓ GPIO irqchip masks/acks/dispatches
GPIO parent output 42
  ↓ root interrupt controller domain
Linux virtual IRQ
  ↓ sensor handler
sensor status clear
```

A failure at the GPIO provider can affect every child. If the parent counter never increases, inspect GPIO bank masks, parent wiring, power, and the parent specifier. If the parent increments but no child dispatch occurs, inspect line-to-bank mapping and GPIO irqchip status.

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

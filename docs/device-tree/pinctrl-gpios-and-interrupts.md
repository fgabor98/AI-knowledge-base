---
status: draft
reviewed: false
domain: device-tree
difficulty: intermediate
last_reviewed: null
---

# Pinctrl, GPIOs, And Interrupts

One package pin can participate in three different models. Pinctrl selects its hardware function and electrical configuration, GPIO gives software logical control of a line, and an interrupt controller turns signal conditions into interrupt-domain events. Correct descriptions keep those roles distinct while proving that they converge on the same physical wiring.

## Learning Outcomes

After completing this module, you should be able to:

- distinguish pin multiplexing from pin configuration and GPIO direction/value
- construct and review `default`, `init`, `idle`, and `sleep` pinctrl states
- decode GPIO specifiers through provider-specific `#gpio-cells`
- use active-low semantics as logical assertion rather than raw voltage
- model GPIO controllers, line names, ranges, reserved lines, and hogs
- review reset GPIO acquisition and sequencing without output glitches
- decode interrupt-controller specifiers and trigger flags
- trace an interrupt through a GPIO irqchip and its parent domain
- verify that sleep pinctrl, power domains, and wake IRQ policy agree
- diagnose ownership conflicts from DT through pinctrl, gpiolib, IRQ domains, and the physical signal

## Prerequisites

Complete [Driver Matching](driver-matching.md). This module assumes you can resolve providers, decode specifier widths, distinguish Linux devices from DT nodes, and determine whether probe succeeded.

## Learning Path

1. [Pinmux, Pin Configuration, And States](pinctrl-gpios-and-interrupts/pinmux-pin-configuration-and-states.md)
2. [GPIO Controllers, Ranges, Line Names, And Hogs](pinctrl-gpios-and-interrupts/gpio-controllers-ranges-line-names-and-hogs.md)
3. [GPIO Consumers, Polarity, And Reset Sequencing](pinctrl-gpios-and-interrupts/gpio-consumers-polarity-and-reset-sequencing.md)
4. [Interrupt Controllers, Specifiers, And Trigger Types](pinctrl-gpios-and-interrupts/interrupt-controllers-specifiers-and-trigger-types.md)
5. [GPIO Interrupts, Cascades, And Shared Lines](pinctrl-gpios-and-interrupts/gpio-interrupts-cascades-and-shared-lines.md)
6. [Sleep States, Wakeup, And Ownership](pinctrl-gpios-and-interrupts/sleep-states-wakeup-and-ownership.md)
7. [Pin, GPIO, And Interrupt Bring-Up Lab](pinctrl-gpios-and-interrupts/pin-gpio-and-interrupt-bring-up-lab.md)

## Four Independent Questions

For one schematic net, answer:

| Layer | Question | Typical DT construct |
|---|---|---|
| mux | Which internal hardware function reaches the pad? | pinctrl provider state |
| electrical | What bias, drive, slew, input, and output mode apply? | pin configuration properties |
| logical GPIO | Which controller/offset owns it, and what means asserted? | `*-gpios` specifier |
| interrupt | Which condition generates which controller-local event? | `interrupts` and trigger flags |

An active-low reset says that low voltage means asserted. It does not by itself select GPIO mux, configure a pull-up, or specify an interrupt trigger. Likewise, `IRQ_TYPE_EDGE_FALLING` says which transition is detected, not whether the signal is logically active-low.

## End-To-End Signal Path

```text
device pin / board net
        ↕ external pull and voltage domain
SoC package pad
        ↓ pinctrl mux + electrical configuration
GPIO input/output logic
        ↓ optional GPIO irqchip
parent interrupt controller
        ↓ Linux IRQ domain
consumer driver
```

Debug from both ends. The runtime tree and subsystem state show software intent; a scope, logic analyzer, schematic, and datasheet show what actually happens electrically.

## Scope Boundary

This module concentrates on pin-level integration and routing. Detailed provider tuple mechanics are covered in [Provider-Consumer Relationships](provider-consumer-relationships.md); general interrupt-map mechanics are in [Addressing And Bus Modeling](addressing-and-bus-modeling.md).

## Completion Check

You are ready for [Clocks, Resets, Regulators, And Power](clocks-resets-regulators-and-power.md) when you can:

- identify mux, pinconf, GPIO, and IRQ ownership for one net
- derive physical output levels from logical GPIO values and polarity flags
- distinguish a GPIO line offset from a global Linux number
- explain why a requested GPIO can remain electrically unchanged
- trace a GPIO interrupt through two IRQ domains
- select a trigger type from signal behavior rather than its name
- show why a valid `wakeup-source` property may still fail to wake the system

## Authoritative References

- [Linux pin control subsystem](https://docs.kernel.org/driver-api/pin-control.html)
- [Linux GPIO consumer interface](https://docs.kernel.org/driver-api/gpio/consumer.html)
- [Linux GPIO provider interface](https://docs.kernel.org/driver-api/gpio/driver.html)
- [Devicetree Specification: phandles and interrupts](https://devicetree-specification.readthedocs.io/en/stable/devicetree-basics.html)

## Related Topics

- [Provider-Consumer Relationships](provider-consumer-relationships.md)
- [Addressing And Bus Modeling](addressing-and-bus-modeling.md)
- [Common Peripheral Nodes](common-peripheral-nodes.md)

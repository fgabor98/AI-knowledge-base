---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# Pin, GPIO, And Interrupt Bring-Up Lab

## Goal

Trace reset and interrupt nets through pinctrl, GPIO, and IRQ domains; predict physical levels; inspect runtime ownership; and diagnose deliberate polarity, trigger, hog, and sleep-state failures.

The controller compatibles and pin syntax are fictional. The tree teaches relationships and is not a schema-valid description of real hardware.

## Lab Tree

Create `pin-gpio-irq-lab.dts`:

```dts
/dts-v1/;

#include <dt-bindings/gpio/gpio.h>
#include <dt-bindings/interrupt-controller/irq.h>

/ {
        compatible = "example,pin-gpio-irq-lab";
        #address-cells = <1>;
        #size-cells = <1>;

        root_intc: interrupt-controller@2000 {
                compatible = "example,lab-root-intc";
                reg = <0x2000 0x100>;
                interrupt-controller;
                #address-cells = <0>;
                #interrupt-cells = <2>;
        };

        pinctrl: pinctrl@1000 {
                compatible = "example,lab-pinctrl-gpio";
                reg = <0x1000 0x100>;

                gpio-controller;
                #gpio-cells = <2>;
                ngpios = <16>;
                gpio-ranges = <&pinctrl 0 0 16>;
                gpio-line-names =
                        "", "", "", "",
                        "", "sensor-reset", "sensor-irq", "",
                        "", "", "", "",
                        "expansion-enable", "", "", "safe-mode";

                interrupt-controller;
                #interrupt-cells = <2>;
                interrupt-parent = <&root_intc>;
                interrupts = <42 IRQ_TYPE_LEVEL_HIGH>;

                sensor_reset_default_pin: sensor-reset-default-pin {
                        pins = "PIN5";
                        function = "gpio";
                        bias-pull-up;
                };

                sensor_irq_default_pin: sensor-irq-default-pin {
                        pins = "PIN6";
                        function = "gpio";
                        bias-pull-up;
                        input-enable;
                };

                sensor_reset_sleep_pin: sensor-reset-sleep-pin {
                        pins = "PIN5";
                        function = "gpio";
                        bias-pull-up;
                };

                sensor_irq_sleep_pin: sensor-irq-sleep-pin {
                        pins = "PIN6";
                        function = "gpio";
                        bias-pull-up;
                        input-enable;
                };

                safe_mode_hog: safe-mode-hog {
                        gpio-hog;
                        gpios = <15 GPIO_ACTIVE_HIGH>;
                        input;
                        line-name = "safe-mode";
                };
        };

        sensor@3000 {
                compatible = "example,lab-sensor";
                reg = <0x3000 0x100>;

                pinctrl-names = "default", "sleep";
                pinctrl-0 = <&sensor_reset_default_pin
                             &sensor_irq_default_pin>;
                pinctrl-1 = <&sensor_reset_sleep_pin
                             &sensor_irq_sleep_pin>;

                reset-gpios = <&pinctrl 5 GPIO_ACTIVE_LOW>;
                interrupt-parent = <&pinctrl>;
                interrupts = <6 IRQ_TYPE_EDGE_FALLING>;
                wakeup-source;
        };
};
```

Because C-preprocessor includes are used, build through the kernel DT build or preprocess using the same include paths and flags as the target kernel. A direct `dtc` command without preprocessing will not resolve the headers.

## Step 1: Capture The Artifact

Record:

- kernel tree/revision providing the binding headers
- preprocessor and `dtc` versions
- complete build command and warnings
- DTB hash
- decompiled final DTS

Compilation checks syntax and structural rules, not the fictional hardware contract or schematic correctness.

## Step 2: Build A Signal Ownership Table

Fill this table before booting:

| Net | Package pin | Pinctrl state | GPIO provider/offset | Logical polarity | IRQ provider/line | Trigger |
|---|---|---|---|---|---|---|
| sensor reset | | | | | n/a | n/a |
| sensor interrupt | | | | n/a | | |
| safe mode | | hog/default | | | n/a | n/a |

Explain why GPIO offset 6 and interrupt line 6 refer to the same controller-local hardware line in this fictional binding but use different flag namespaces.

## Step 3: Predict Reset Levels

For `reset-gpios = <&pinctrl 5 GPIO_ACTIVE_LOW>`, fill:

| Driver logical request | Physical level | Sensor state |
|---:|---|---|
| 1 active | | |
| 0 inactive | | |

Expected: logical 1 drives low and asserts reset; logical 0 drives high and deasserts it. Then identify what the external pull-up does before the GPIO controller and consumer driver take ownership.

## Step 4: Trace The Interrupt Cascade

Write the complete route:

```text
sensor IRQ pin
→ PIN6 input
→ GPIO local interrupt 6, falling-edge
→ GPIO block parent output
→ root controller hardware interrupt 42, level-high
→ Linux virtual IRQ
→ sensor handler
```

There are two trigger descriptions because there are two different controller inputs. Explain why the GPIO child can be falling-edge while the GPIO block's parent output is level-high.

## Step 5: Inspect Runtime State

On analogous real hardware, collect read-only evidence:

```sh
gpioinfo
cat /proc/interrupts
cat /sys/kernel/debug/pinctrl/*/pinmux-pins
cat /sys/kernel/debug/pinctrl/*/pinconf-pins
cat /sys/kernel/debug/pinctrl/*/gpio-ranges
```

Also resolve the consumer's `of_node`, inspect its runtime properties, and capture probe/IRQ logs. Map line names, consumer labels, GPIO offsets, hardware IRQs, and Linux IRQs into separate columns.

Do not use `gpioset` on kernel-owned reset or interrupt lines.

## Step 6: Break Reset Polarity

Change:

```dts
reset-gpios = <&pinctrl 5 GPIO_ACTIVE_HIGH>;
```

Predict the physical level for each logical driver request and likely symptom. A normal probe sequence may now release reset when it intends to assert it and assert reset before register access. Restore active-low.

Prove the error with a scope or controller readback where safe; a successful GPIO request alone cannot establish physical polarity.

## Step 7: Substitute The Wrong Flag Namespace

Change:

```dts
interrupts = <6 GPIO_ACTIVE_LOW>;
```

The numeric value compiles after preprocessing, but in the common IRQ namespace value `1` means rising edge, not “active low.” Predict the resulting missed or inverted event behavior. Restore `IRQ_TYPE_EDGE_FALLING`.

This demonstrates why symbolic constants must come from the property's provider binding, not merely have a plausible name.

## Step 8: Create A Hog Conflict

Change the hog to claim GPIO offset 5:

```dts
gpios = <5 GPIO_ACTIVE_HIGH>;
output-low;
```

The hog and sensor reset consumer now compete for one line. Record compile/schema diagnostics, then predict runtime ownership and the consumer's GPIO acquisition error. Explain why a static compiler cannot generally prove all runtime line conflicts. Restore offset 15 and `input`.

## Step 9: Break The Sleep Wake Path

Change `sensor_irq_sleep_pin` to disable its input and bias. Suspend is now permitted to remove the path or let it float even though `wakeup-source` remains present.

For an analogous real platform, test:

- interrupt immediately before suspend
- interrupt during the transition
- interrupt after deep sleep entry
- line already asserted at suspend

Capture the physical waveform, wake reason, GPIO state, and IRQ counters. Restore input and the valid inactive bias.

## Step 10: Break The Cascade Parent

Remove the GPIO controller's parent `interrupts` property while keeping it as a child interrupt provider. The sensor's local specifier is still structurally understandable, but the GPIO block has no described route upstream.

Predict which stages can still succeed:

- pinctrl selection
- GPIO controller registration
- reset GPIO request
- child IRQ-domain creation
- parent IRQ acquisition
- sensor IRQ delivery

Restore the parent relationship.

## Step 11: Test Trigger/Acknowledge Behavior

On real hardware, run repeated events and record:

1. inactive voltage
2. assertion edge/level
3. GPIO pending bit
4. root parent count
5. child Linux IRQ count
6. device status before and after handler
7. deassertion timing

Classify no-event, one-shot, duplicate, and storm failures without changing flags by trial and error.

## Completion Criteria

The lab is complete when you can provide:

- reproducible DTB identity and final source
- a complete signal ownership table
- logical-to-physical reset truth table
- two-domain interrupt route and trigger explanation
- runtime pinctrl/GPIO/IRQ evidence
- predicted and observed outcomes for each deliberate failure
- suspend/wake boundary tests
- proof of safe restoration and cleanup

## Authoritative References

- [Linux pin control subsystem](https://docs.kernel.org/driver-api/pin-control.html)
- [Linux GPIO consumer interface](https://docs.kernel.org/driver-api/gpio/consumer.html)
- [Linux GPIO irqchip provider interface](https://docs.kernel.org/driver-api/gpio/driver.html)
- [Linux IRQ-domain documentation](https://docs.kernel.org/core-api/irq/irq-domain.html)

## Continue

Proceed to [Clocks, Resets, Regulators, And Power](../clocks-resets-regulators-and-power.md).

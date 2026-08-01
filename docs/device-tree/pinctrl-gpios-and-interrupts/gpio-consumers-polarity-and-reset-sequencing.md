---
status: draft
reviewed: false
domain: device-tree
difficulty: intermediate
last_reviewed: null
---

# GPIO Consumers, Polarity, And Reset Sequencing

Consumer GPIO properties describe named logical signals. The provider binding defines the numeric cells; the consumer binding defines the property name, cardinality, and what assertion means for the device.

## Named Consumer Relationship

```dts
sensor@48 {
        compatible = "vendor,sensor";
        reset-gpios = <&gpio2 5 GPIO_ACTIVE_LOW>;
        enable-gpios = <&gpio1 3 GPIO_ACTIVE_HIGH>;
};
```

Linux descriptor lookup uses the consumer prefix:

```c
reset = devm_gpiod_get(dev, "reset", GPIOD_OUT_HIGH);
enable = devm_gpiod_get(dev, "enable", GPIOD_OUT_LOW);
```

`"reset"` maps to `reset-gpios`; `"enable"` maps to `enable-gpios`. The binding, not driver preference, decides whether those properties are required, optional, singular, or arrays.

Legacy singular `*-gpio` spellings exist in old bindings, but new bindings normally use `*-gpios`. Do not rename deployed properties without preserving ABI support.

## Logical Versus Physical Values

Descriptor APIs use logical assertion:

| DT polarity | Logical value | Physical level |
|---|---:|---|
| `GPIO_ACTIVE_HIGH` | 0 inactive | low |
| `GPIO_ACTIVE_HIGH` | 1 active | high |
| `GPIO_ACTIVE_LOW` | 0 inactive | high |
| `GPIO_ACTIVE_LOW` | 1 active | low |

Thus `GPIOD_OUT_HIGH` means initialize the signal **active**, not necessarily drive voltage high. For an active-low reset, it drives low and asserts reset.

Prefer logical `gpiod_set_value*()` calls. Raw APIs bypass polarity and should be rare in reusable drivers.

## Name The Device-Side Meaning

The GPIO flag should describe assertion at the consumer, including board inversion:

```text
SoC output high → inverter → device RESET# low → reset asserted
```

From the consumer's perspective this path is active-high at the SoC output: a logical active request produces the high level needed to assert the device after inversion. Review the entire schematic path, not only the `#` suffix at the device pin.

Avoid property names that encode polarity such as `reset-low-gpios`. Polarity belongs in the specifier flags; the property names the function.

## Open-Drain And Bias

Active-low and open-drain describe different things:

- active-low maps logical active to physical low
- open-drain drives low but releases high
- pull-up establishes the released high level

GPIO flags can encode single-ended/open-drain semantics where the provider and binding support them. Pinctrl may also configure pad drive and bias. These descriptions must agree with each other and with external pull resistors.

A shared active-low line often needs open-drain behavior so any participant can assert it without contention. Marking it only `GPIO_ACTIVE_LOW` still permits push-pull drive high.

## Reset Sequencing

A robust reset sequence considers power, clocks, pins, and timing:

```text
acquire reset descriptor with a safe initial logical state
→ enable supplies and required reference clocks
→ wait power-stabilization interval
→ select safe/active pinctrl state
→ deassert reset
→ wait reset-recovery interval
→ access device
```

Exact ordering comes from the component datasheet and board design. Some devices require reset asserted before power; others forbid driven inputs while unpowered because of back-powering.

Use a reset-controller binding instead of a GPIO when the line belongs to a reset controller. Use a GPIO only when the board physically wires reset to a general-purpose line and the consumer binding defines it.

## Avoid Output Glitches

Request output GPIOs with their initial logical value in the acquisition call. Configuring direction first and value second can briefly assert an enable or deassert reset.

Also inspect earlier phases:

- reset/power-on default of the pad
- bootloader mux and output value
- transition from pinctrl `init` to `default`
- GPIO controller probe and hog behavior
- suspend/resume restoration
- driver unbind and failed-probe cleanup

Software cannot eliminate a hardware glitch that occurs before it controls the pin. Safety-critical defaults may require external pulls or gating.

## Optional GPIOs And Errors

Use optional descriptor getters only when the binding makes absence valid. An optional getter can return no descriptor for absence, but malformed specifiers, unavailable providers, and deferral remain errors.

Do not convert every error to “GPIO not fitted.” That hides disabled controllers and broken phandles. Use `dev_err_probe()` and preserve `-EPROBE_DEFER`.

## Reset Cleanup And Ownership

Define the state after probe failure, remove, shutdown, and suspend:

- Should reset be asserted?
- Should enable be inactive?
- Can another firmware component own the device afterward?
- Does assertion while unpowered cause current leakage?
- Is the GPIO controller still powered when cleanup runs?

Managed descriptor release does not automatically choose the safe physical state. Add explicit cleanup actions where the hardware contract requires them.

## Review Traps

- Reading `GPIOD_OUT_HIGH` as physical high rather than logical active.
- Copying `GPIO_ACTIVE_LOW` from a signal name without tracing inversion.
- Confusing active-low with falling-edge interrupt triggering.
- Treating open-drain and pull-up as synonyms.
- Using raw GPIO APIs to compensate for a wrong DT polarity.
- Claiming the same reset through a hog and a consumer.
- Deasserting reset before supplies, clocks, or pins are valid.
- Assuming descriptor release restores a safe board state.

## Authoritative References

- [Linux GPIO descriptor consumer interface](https://docs.kernel.org/driver-api/gpio/consumer.html)
- [Linux standard GPIO binding flags](https://github.com/torvalds/linux/blob/master/include/dt-bindings/gpio/gpio.h)
- [Linux GPIO provider electrical behavior](https://docs.kernel.org/driver-api/gpio/driver.html)
- [Linux Device Tree binding guidance](https://docs.kernel.org/devicetree/bindings/writing-bindings.html)

## Next Step

Continue with [Interrupt Controllers, Specifiers, And Trigger Types](interrupt-controllers-specifiers-and-trigger-types.md).

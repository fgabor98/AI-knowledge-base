---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# Pinctrl

## What Problem Does This Solve?

Pinctrl configures pin multiplexing and electrical settings so SoC pins are connected to the intended hardware functions.

Modern SoC pins often have many possible functions:

```text
pin A:
  GPIO
  UART_TX
  SPI_MOSI
  PWM_OUT
```

They also have electrical configuration:

- pull-up or pull-down
- drive strength
- slew rate
- input enable
- open drain

The pinctrl subsystem lets board data define pin states and drivers select the appropriate state.

## Core Concepts

- pinctrl provider
- pin consumer
- pinmux
- pin configuration
- pinctrl state
- `default` state
- `sleep` state
- `idle` state
- bias
- drive strength
- slew rate
- pin ownership
- GPIO ranges
- bootloader pin state

## Mental Model

Drivers select named pinctrl states. Board descriptions define what each state means for package pins.

```dts
pinctrl-names = "default", "sleep";
pinctrl-0 = <&uart3_default_pins>;
pinctrl-1 = <&uart3_sleep_pins>;
```

Driver:

```c
pinctrl_pm_select_default_state(dev);
pinctrl_pm_select_sleep_state(dev);
```

The driver should not know register-level mux details.

## Device Tree Shape

Consumer node:

```dts
&uart3 {
    pinctrl-names = "default", "sleep";
    pinctrl-0 = <&uart3_default_pins>;
    pinctrl-1 = <&uart3_sleep_pins>;
    status = "okay";
};
```

Provider-specific pin group:

```dts
uart3_default_pins: uart3-default-pins {
    pins = "uart3-tx", "uart3-rx";
    function = "uart3";
    bias-disable;
};
```

The exact syntax of pin groups is provider-specific. Always use the SoC binding.

## Default State

The device core often selects the `default` state for devices during probe, depending on bus and configuration.

Many drivers do not need to manually select default state. Still, a driver may explicitly request/select states when it has runtime transitions:

```c
priv->pinctrl = devm_pinctrl_get(dev);
if (IS_ERR(priv->pinctrl))
    return dev_err_probe(dev, PTR_ERR(priv->pinctrl),
                         "failed to get pinctrl\n");

priv->pins_default = pinctrl_lookup_state(priv->pinctrl, "default");
if (IS_ERR(priv->pins_default))
    return PTR_ERR(priv->pins_default);

ret = pinctrl_select_state(priv->pinctrl, priv->pins_default);
```

Use subsystem PM helpers when appropriate:

```c
pinctrl_pm_select_default_state(dev);
pinctrl_pm_select_sleep_state(dev);
```

## Sleep And Idle States

Sleep state:

```dts
pinctrl-names = "default", "sleep";
pinctrl-0 = <&spi1_default_pins>;
pinctrl-1 = <&spi1_sleep_pins>;
```

Runtime/system suspend:

```c
static int demo_suspend(struct device *dev)
{
    return pinctrl_pm_select_sleep_state(dev);
}

static int demo_resume(struct device *dev)
{
    return pinctrl_pm_select_default_state(dev);
}
```

Sleep state may reduce leakage, avoid driving external devices, or allow wake-capable pins to remain configured.

## Pinctrl And GPIO

GPIO use may require pinmux to GPIO function.

Example consumer:

```dts
reset-gpios = <&gpio2 5 GPIO_ACTIVE_LOW>;
pinctrl-names = "default";
pinctrl-0 = <&sensor_reset_pin>;
```

Symptoms of missing pinctrl:

- GPIO request succeeds but physical pin does not move
- input always reads one value
- IRQ never fires
- another peripheral still owns the pinmux

Debug:

```sh
cat /sys/kernel/debug/pinctrl/*/pinmux-pins
cat /sys/kernel/debug/gpio
```

## Electrical Configuration

Common pin config properties include:

```dts
bias-pull-up;
bias-pull-down;
bias-disable;
drive-strength = <8>;
slew-rate = <1>;
input-enable;
output-high;
output-low;
```

Exact names and semantics vary by pinctrl provider binding.

Electrical settings matter. A muxed pin can still fail if pull, drive strength, or voltage domain is wrong.

## Bootloader Versus Linux Pin State

Boot firmware may configure pins before Linux boots.

Linux should still define the required pin state for the runtime driver. Depending on bootloader state alone is fragile because:

- warm boot and cold boot can differ
- firmware updates can change pin setup
- suspend/resume may restore Linux states
- overlays may change selected pins
- another driver may claim the pin

## Pin Ownership Conflicts

A pin cannot normally be owned by incompatible functions at the same time.

Conflict examples:

```text
UART3 TX and GPIO reset on same pin
SPI1 MOSI and PWM output on same pin
bootloader left pinmux in debug UART mode
```

Debug:

```sh
cat /sys/kernel/debug/pinctrl/*/pinmux-pins
cat /sys/kernel/debug/pinctrl/*/pins
```

Resolve in board design and Device Tree, not by fighting from driver code.

## Debugging Pinctrl

Debugfs:

```sh
ls /sys/kernel/debug/pinctrl
cat /sys/kernel/debug/pinctrl/*/pinmux-pins
cat /sys/kernel/debug/pinctrl/*/pinconf-pins
```

Runtime Device Tree:

```sh
dtc -I fs -O dts /proc/device-tree > /tmp/running.dts
rg 'pinctrl-names|pinctrl-0|bias-|drive-strength' /tmp/running.dts
```

Electrical validation:

- scope
- logic analyzer
- board schematic
- SoC pinmux tool
- hardware manual

## Common Failure Modes

| Symptom | Likely Cause | First Checks |
| --- | --- | --- |
| driver probes but bus silent | pinmux not selected | pinctrl debugfs |
| GPIO does not toggle | pin not muxed as GPIO | pinmux-pins |
| IRQ never fires | input disabled or wrong pull | pinconf, scope |
| high-speed signal unreliable | drive/slew wrong | electrical config |
| suspend wake fails | sleep state disables wake pin | sleep pinctrl |
| conflict warning | two functions claim pin | board DT |

## Common Mistakes

- Assuming bootloader pinmux is enough.
- Checking mux but not electrical config.
- Reusing pin groups across boards with different wiring.
- Selecting states manually when subsystem/device core already handles default without understanding interactions.
- Ignoring sleep state for suspend/resume.
- Debugging I2C/SPI/UART protocol before checking pinmux.

## Practice Exercises

### Exercise 1: Trace A Pin State

Pick one enabled UART/I2C/SPI node. Find its `pinctrl-0` group and inspect the active pinmux in debugfs.

### Exercise 2: Add Sleep State

Add a `sleep` state for a simple device and select it during suspend.

### Exercise 3: Debug A GPIO

Request a GPIO and verify both gpiolib ownership and pinctrl mux state.

## Debugging Checklist

- Does runtime Device Tree contain `pinctrl-names` and state references?
- Does the provider binding match the SoC?
- Is the default state selected?
- Are pins muxed to the intended function?
- Are electrical settings correct?
- Is another driver claiming the same pin?
- Does suspend/resume use the right state?
- Did the bootloader leave surprising state behind?

## Related Topics

- [GPIO Consumer API](gpio-consumer-api.md)
- [Clocks](clocks.md)
- [Suspend And Resume](../power-management/suspend-resume.md)
- [Device Tree Hardware Description](../fundamentals/device-tree-hardware-description.md)

## Official References

- [PINCTRL subsystem](https://docs.kernel.org/driver-api/pin-control.html)
- [Linux and the Devicetree](https://docs.kernel.org/devicetree/usage-model.html)

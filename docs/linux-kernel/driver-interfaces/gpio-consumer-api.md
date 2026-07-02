---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# GPIO Consumer API

## What Problem Does This Solve?

The GPIO consumer API lets drivers request and control GPIO lines without depending on board-specific GPIO numbers.

Instead of saying:

```text
use GPIO 37
```

a driver says:

```text
give me the GPIO used as this device's reset line
```

Device Tree, ACPI, or board data maps that role to the physical line and polarity.

## Core Concepts

- GPIO descriptor
- `struct gpio_desc`
- `devm_gpiod_get()`
- `devm_gpiod_get_optional()`
- `devm_gpiod_get_index()`
- GPIO arrays
- active-low handling
- logical value
- physical level
- direction flags
- output initial value
- `gpiod_set_value()`
- `gpiod_set_value_cansleep()`
- `gpiod_get_value()`
- `gpiod_get_value_cansleep()`
- `gpiod_to_irq()`
- Device Tree `*-gpios` properties

## Mental Model

A driver asks for a named GPIO role. The board description maps that role to a physical GPIO line and encodes polarity.

```text
driver asks for "reset"
Device Tree has reset-gpios = <&gpio2 5 GPIO_ACTIVE_LOW>
gpiolib returns a descriptor
driver sets logical 1 to assert reset
gpiolib drives physical low because the line is active-low
```

The driver should use logical values. Do not manually invert active-low lines.

## Device Tree Mapping

Example node:

```dts
sensor@48 {
    compatible = "example,tmp102";
    reg = <0x48>;
    reset-gpios = <&gpio2 5 GPIO_ACTIVE_LOW>;
    enable-gpios = <&gpio1 3 GPIO_ACTIVE_HIGH>;
};
```

Driver:

```c
priv->reset = devm_gpiod_get(dev, "reset", GPIOD_OUT_HIGH);
if (IS_ERR(priv->reset))
    return dev_err_probe(dev, PTR_ERR(priv->reset),
                         "failed to get reset gpio\n");

priv->enable = devm_gpiod_get_optional(dev, "enable", GPIOD_OUT_LOW);
if (IS_ERR(priv->enable))
    return dev_err_probe(dev, PTR_ERR(priv->enable),
                         "failed to get enable gpio\n");
```

The consumer name `"reset"` maps to the Device Tree property `reset-gpios`.

## Required Versus Optional GPIOs

Required:

```c
desc = devm_gpiod_get(dev, "reset", GPIOD_OUT_HIGH);
if (IS_ERR(desc))
    return dev_err_probe(dev, PTR_ERR(desc),
                         "failed to get reset gpio\n");
```

Optional:

```c
desc = devm_gpiod_get_optional(dev, "enable", GPIOD_OUT_LOW);
if (IS_ERR(desc))
    return dev_err_probe(dev, PTR_ERR(desc),
                         "failed to get enable gpio\n");

if (desc)
    gpiod_set_value_cansleep(desc, 1);
```

Use optional GPIOs only when the hardware can truly work without the line on some boards.

## Direction And Initial Value

Request with direction:

```c
reset = devm_gpiod_get(dev, "reset", GPIOD_OUT_HIGH);
irq_gpio = devm_gpiod_get(dev, "irq", GPIOD_IN);
```

Common flags:

| Flag | Meaning |
| --- | --- |
| `GPIOD_ASIS` | Do not configure direction yet. |
| `GPIOD_IN` | Configure as input. |
| `GPIOD_OUT_LOW` | Configure as output with logical 0. |
| `GPIOD_OUT_HIGH` | Configure as output with logical 1. |
| `GPIOD_OUT_LOW_OPEN_DRAIN` | Output low, enforce open-drain use. |
| `GPIOD_OUT_HIGH_OPEN_DRAIN` | Output high, enforce open-drain use. |

Initial output values are logical values. With active-low lines, logical high may drive the physical line low.

## Setting And Reading Values

Fast/non-sleeping provider:

```c
gpiod_set_value(desc, 1);
val = gpiod_get_value(desc);
```

Provider may sleep:

```c
gpiod_set_value_cansleep(desc, 1);
val = gpiod_get_value_cansleep(desc);
```

Use `*_cansleep()` unless you know the GPIO provider is safe in atomic context. GPIO expanders on I2C or SPI can sleep because they perform bus transactions.

Check:

```c
if (gpiod_cansleep(desc))
    dev_dbg(dev, "gpio access may sleep\n");
```

Do not call sleeping GPIO accessors from hard IRQ context.

## Active-Low Semantics

Device Tree:

```dts
reset-gpios = <&gpio2 5 GPIO_ACTIVE_LOW>;
```

Driver:

```c
gpiod_set_value_cansleep(reset, 1); /* assert reset */
gpiod_set_value_cansleep(reset, 0); /* deassert reset */
```

The driver expresses intent. GPIOLIB handles polarity.

Bad:

```c
gpiod_set_value_cansleep(reset, 0); /* manually inverted for active-low */
```

That breaks when the same driver runs on a board wired active-high.

## Indexed GPIOs And Arrays

Device Tree:

```dts
data-gpios = <&gpio1 0 GPIO_ACTIVE_HIGH>,
             <&gpio1 1 GPIO_ACTIVE_HIGH>,
             <&gpio1 2 GPIO_ACTIVE_HIGH>,
             <&gpio1 3 GPIO_ACTIVE_HIGH>;
```

Single indexed line:

```c
desc = devm_gpiod_get_index(dev, "data", 2, GPIOD_IN);
```

Array:

```c
struct gpio_descs *data;

data = devm_gpiod_get_array(dev, "data", GPIOD_IN);
if (IS_ERR(data))
    return PTR_ERR(data);
```

GPIO arrays are useful for simple parallel control, but do not turn complex buses into GPIO bit banging unless the hardware truly requires it.

## GPIO To IRQ

If a GPIO line is also an interrupt source:

```c
irq_gpio = devm_gpiod_get(dev, "irq", GPIOD_IN);
if (IS_ERR(irq_gpio))
    return dev_err_probe(dev, PTR_ERR(irq_gpio),
                         "failed to get irq gpio\n");

irq = gpiod_to_irq(irq_gpio);
if (irq < 0)
    return dev_err_probe(dev, irq, "failed to map gpio irq\n");
```

Then request the IRQ:

```c
ret = devm_request_threaded_irq(dev, irq, NULL, demo_irq_thread,
                                IRQF_TRIGGER_FALLING | IRQF_ONESHOT,
                                dev_name(dev), priv);
```

The trigger type may also come from Device Tree `interrupts` properties. Avoid encoding trigger policy twice.

## Open Drain And Electrical Behavior

Open-drain lines are often used for reset, interrupt, or wire-OR behavior.

Device Tree should describe electrical characteristics where bindings support it. The consumer may request open-drain output flags when the use case requires it:

```c
line = devm_gpiod_get(dev, "alert", GPIOD_OUT_HIGH_OPEN_DRAIN);
```

If the board description disagrees, the kernel may warn. Treat that warning as a board-description bug to fix.

## GPIO Consumer In Probe

```c
struct demo_priv {
    struct device *dev;
    struct gpio_desc *reset;
    struct gpio_desc *enable;
};

static int demo_probe(struct platform_device *pdev)
{
    struct device *dev = &pdev->dev;
    struct demo_priv *priv;

    priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
    if (!priv)
        return -ENOMEM;

    priv->dev = dev;

    priv->reset = devm_gpiod_get(dev, "reset", GPIOD_OUT_HIGH);
    if (IS_ERR(priv->reset))
        return dev_err_probe(dev, PTR_ERR(priv->reset),
                             "failed to get reset gpio\n");

    priv->enable = devm_gpiod_get_optional(dev, "enable", GPIOD_OUT_LOW);
    if (IS_ERR(priv->enable))
        return dev_err_probe(dev, PTR_ERR(priv->enable),
                             "failed to get enable gpio\n");

    if (priv->enable)
        gpiod_set_value_cansleep(priv->enable, 1);

    usleep_range(10000, 12000);
    gpiod_set_value_cansleep(priv->reset, 0);

    return 0;
}
```

## Debugging GPIO Consumers

Inspect GPIO chips and line ownership:

```sh
gpioinfo
```

Kernel debugfs, if enabled:

```sh
cat /sys/kernel/debug/gpio
```

Runtime Device Tree:

```sh
dtc -I fs -O dts /proc/device-tree > /tmp/running.dts
rg 'reset-gpios|enable-gpios' /tmp/running.dts
```

Driver logs:

```sh
dmesg | grep -i gpio
```

## Common Failure Modes

| Symptom | Likely Cause | First Checks |
| --- | --- | --- |
| `-ENOENT` for required GPIO | property missing or wrong consumer name | `*-gpios` property |
| `-EPROBE_DEFER` | GPIO controller not ready | provider driver and node |
| line is inverted | driver manually inverted active-low | remove manual inversion |
| sleep warning | used non-cansleep accessor on sleeping provider | `gpiod_cansleep()` |
| line busy | another driver requested the GPIO | `gpioinfo`, debugfs |
| no visible change on pin | pinmux not set or output enable wrong | pinctrl, scope |

## Common Mistakes

- Hard-coding global GPIO numbers.
- Using legacy `gpio_*()` APIs in new code.
- Manually inverting active-low lines.
- Calling sleeping GPIO access from hard IRQ context.
- Treating optional GPIO absence as an error.
- Treating a required GPIO as optional and hiding board bugs.
- Forgetting pinctrl when the line never toggles physically.

## Practice Exercises

### Exercise 1: Reset GPIO

Add a `reset-gpios` property to a dummy device and request it with `devm_gpiod_get()`. Toggle it with logical values and verify the physical polarity.

### Exercise 2: Optional Enable GPIO

Make `enable-gpios` optional. Test both with and without the property.

### Exercise 3: GPIO IRQ

Use `gpiod_to_irq()` on an input GPIO and request a threaded IRQ. Confirm the interrupt count in `/proc/interrupts`.

## Debugging Checklist

- Does the runtime Device Tree contain the expected `*-gpios` property?
- Does the consumer name match the property prefix?
- Did the GPIO provider probe?
- Is active-low encoded in firmware data?
- Are you using logical values?
- Can the GPIO provider sleep?
- Is pinctrl selecting the correct mux and electrical state?
- Does `gpioinfo` show the line requested by the expected consumer?

## Related Topics

- [Device Tree Hardware Description](../fundamentals/device-tree-hardware-description.md)
- [Pinctrl](pinctrl.md)
- [GPIO Controller Drivers](gpio-controller-drivers.md)
- [GPIO Expanders](gpio-expanders.md)
- [Resource Lookup And Managed Allocation](../fundamentals/resource-lookup-and-devm.md)

## Official References

- [GPIO Descriptor Consumer Interface](https://docs.kernel.org/driver-api/gpio/consumer.html)
- [GPIO Mappings](https://docs.kernel.org/driver-api/gpio/board.html)
- [GPIO Character Device Userspace API](https://docs.kernel.org/userspace-api/gpio/chardev.html)

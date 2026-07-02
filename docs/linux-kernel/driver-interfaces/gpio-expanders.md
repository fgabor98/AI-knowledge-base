---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# GPIO Expanders

## What Problem Does This Solve?

GPIO expanders provide additional GPIO lines through buses such as I2C or SPI.

They are common when an SoC does not have enough native GPIOs or when board designers need remote low-speed control lines.

A GPIO expander driver is both:

```text
I2C/SPI client
  talks to the expander chip

GPIO provider
  exposes expander pins through gpiolib
```

## Core Concepts

- I2C GPIO expander
- SPI GPIO expander
- `gpio_chip`
- `gpio-controller`
- `#gpio-cells`
- `can_sleep`
- register cache
- regmap
- interrupt-capable expander
- cascaded IRQ
- nested threaded IRQ
- reset GPIO
- regulator supply
- line names

## Mental Model

Expander GPIO operations are bus transactions. They can sleep.

```text
consumer calls gpiod_set_value_cansleep()
-> gpiolib calls expander .set()
-> expander driver writes an I2C/SPI register
-> physical expander pin changes
```

Do not use expander GPIOs from hard IRQ context.

## Device Tree Example

```dts
&i2c1 {
    status = "okay";

    gpio_expander: gpio@20 {
        compatible = "example,gpio-expander";
        reg = <0x20>;
        gpio-controller;
        #gpio-cells = <2>;
        reset-gpios = <&gpio2 4 GPIO_ACTIVE_LOW>;
        vcc-supply = <&vdd_3v3>;
        gpio-line-names =
            "exp-reset", "exp-enable", "exp-fault", "exp-user";
    };
};
```

Consumer:

```dts
sensor@48 {
    compatible = "example,tmp102";
    reg = <0x48>;
    reset-gpios = <&gpio_expander 0 GPIO_ACTIVE_LOW>;
};
```

The consumer does not care that the GPIO comes from an I2C expander. It requests a descriptor normally.

## Driver Shape

```c
struct demo_expander {
    struct device *dev;
    struct regmap *regmap;
    struct gpio_chip gc;
    struct mutex lock;
};
```

Register map:

```c
static const struct regmap_config demo_regmap_config = {
    .reg_bits = 8,
    .val_bits = 8,
    .max_register = DEMO_MAX_REG,
};
```

Probe:

```c
static int demo_probe(struct i2c_client *client)
{
    struct demo_expander *exp;
    int ret;

    exp = devm_kzalloc(&client->dev, sizeof(*exp), GFP_KERNEL);
    if (!exp)
        return -ENOMEM;

    exp->dev = &client->dev;
    mutex_init(&exp->lock);

    exp->regmap = devm_regmap_init_i2c(client, &demo_regmap_config);
    if (IS_ERR(exp->regmap))
        return dev_err_probe(&client->dev, PTR_ERR(exp->regmap),
                             "failed to init regmap\n");

    exp->gc.label = dev_name(&client->dev);
    exp->gc.parent = &client->dev;
    exp->gc.owner = THIS_MODULE;
    exp->gc.ngpio = 8;
    exp->gc.can_sleep = true;
    exp->gc.get = demo_gpio_get;
    exp->gc.set = demo_gpio_set;
    exp->gc.direction_input = demo_gpio_direction_input;
    exp->gc.direction_output = demo_gpio_direction_output;

    ret = devm_gpiochip_add_data(&client->dev, &exp->gc, exp);
    if (ret)
        return dev_err_probe(&client->dev, ret,
                             "failed to register gpiochip\n");

    return 0;
}
```

## Accessors Must Respect Bus Context

Example get:

```c
static int demo_gpio_get(struct gpio_chip *gc, unsigned int offset)
{
    struct demo_expander *exp = gpiochip_get_data(gc);
    unsigned int val;
    int ret;

    ret = regmap_read(exp->regmap, DEMO_INPUT_REG, &val);
    if (ret)
        return ret;

    return !!(val & BIT(offset));
}
```

Because `regmap_read()` over I2C can sleep, `gc.can_sleep` must be true.

## Direction And Output State

Many expanders have separate direction, output, and input registers.

Output pattern:

```c
static int demo_gpio_direction_output(struct gpio_chip *gc,
                                      unsigned int offset, int value)
{
    struct demo_expander *exp = gpiochip_get_data(gc);
    int ret;

    ret = regmap_update_bits(exp->regmap, DEMO_OUT_REG,
                             BIT(offset), value ? BIT(offset) : 0);
    if (ret)
        return ret;

    return regmap_update_bits(exp->regmap, DEMO_DIR_REG,
                              BIT(offset), 0);
}
```

Set output value before changing direction when the chip could glitch.

## Register Caching

Regmap caching is often useful for expanders because:

- output state can be cached
- direction state can be cached
- repeated reads/writes can be reduced
- volatile input registers can be marked separately

Example:

```c
static bool demo_volatile_reg(struct device *dev, unsigned int reg)
{
    return reg == DEMO_INPUT_REG || reg == DEMO_IRQ_STATUS_REG;
}

static const struct regmap_config demo_regmap_config = {
    .reg_bits = 8,
    .val_bits = 8,
    .max_register = DEMO_MAX_REG,
    .cache_type = REGCACHE_RBTREE,
    .volatile_reg = demo_volatile_reg,
};
```

Do not cache status or interrupt-clear registers incorrectly.

## Interrupt-Capable Expanders

Some expanders can signal changes through one parent IRQ line.

High-level flow:

```text
expander pin changes
-> expander asserts INT pin
-> SoC GPIO/IRQ sees parent IRQ
-> expander driver reads status register
-> gpiolib irqchip dispatches child GPIO IRQs
-> consumer driver's IRQ handler runs
```

This requires irqchip integration in the expander driver, not just a `gpio_chip`.

Debug separately:

- GPIO provider registration
- parent IRQ firing
- expander status register
- child IRQ mapping
- consumer handler

## Reset And Power

Expanders often need:

```dts
reset-gpios = <&gpio2 4 GPIO_ACTIVE_LOW>;
vcc-supply = <&vdd_3v3>;
```

Probe sequence:

```text
enable regulator
deassert reset
wait startup delay
read chip ID if available
register gpiochip
```

Use managed cleanup actions for enabled supplies or deasserted resets if needed.

## Debugging Expanders

I2C presence:

```sh
i2cdetect -y 1
```

Use cautiously. Some devices do not tolerate probing.

GPIO provider:

```sh
gpiodetect
gpioinfo
cat /sys/kernel/debug/gpio
```

Interrupts:

```sh
cat /proc/interrupts
dmesg | grep -i irq
```

Regmap debugfs, if enabled:

```sh
find /sys/kernel/debug/regmap -maxdepth 2 -type f
```

## Common Failure Modes

| Symptom | Likely Cause | First Checks |
| --- | --- | --- |
| expander does not probe | wrong bus address, reset, regulator, compatible | dmesg, DT, I2C/SPI bus |
| gpiochip missing | driver failed before `gpiochip_add` | probe logs |
| consumer sleeps warning | consumer used non-cansleep API | use `*_cansleep()` |
| output value stale | cache or output register bug | regmap debugfs |
| interrupt never fires | parent IRQ, expander IRQ config, polarity | `/proc/interrupts`, status reg |
| wrong pin toggles | offset mapping or `#gpio-cells` issue | binding, gpioinfo |

## Common Mistakes

- Forgetting `can_sleep = true`.
- Treating expander GPIOs like SoC MMIO GPIOs in IRQ context.
- Registering GPIO lines before the chip is powered and initialized.
- Caching volatile input or IRQ status registers.
- Debugging consumer failure without first confirming the expander gpiochip exists.
- Ignoring reset and regulator sequencing.
- Assuming GPIO support implies interrupt support.

## Practice Exercises

### Exercise 1: Inspect An Existing Expander

Find an expander node in Device Tree, then confirm:

```sh
gpiodetect
gpioinfo
```

shows the expected gpiochip and line count.

### Exercise 2: Consume An Expander Line

Reference one expander GPIO from another device's `reset-gpios` property. Confirm the consumer line owner in `gpioinfo`.

### Exercise 3: Separate GPIO And IRQ Debugging

For an interrupt-capable expander, prove the line can be read as GPIO first. Then debug parent IRQ and child IRQ handling separately.

## Debugging Checklist

- Is the expander bus device present?
- Did reset and power sequencing complete?
- Does a gpiochip appear?
- Is `can_sleep` set?
- Do consumer drivers use cansleep accessors?
- Are volatile registers marked correctly?
- If interrupt-capable, does the parent IRQ fire?
- Are line offsets and names correct?

## Related Topics

- [I2C Client Drivers](i2c-client-drivers.md)
- [SPI Device Drivers](spi-device-drivers.md)
- [GPIO Controller Drivers](gpio-controller-drivers.md)
- [Regmap](regmap.md)
- [Threaded Interrupts](threaded-interrupts.md)

## Official References

- [GPIO Driver Interface](https://docs.kernel.org/driver-api/gpio/driver.html)
- [GPIO Descriptor Consumer Interface](https://docs.kernel.org/driver-api/gpio/consumer.html)
- [Implementing I2C device drivers](https://docs.kernel.org/i2c/writing-clients.html)

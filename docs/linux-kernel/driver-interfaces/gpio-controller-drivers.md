---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# GPIO Controller Drivers

## What Problem Does This Solve?

GPIO controller drivers expose hardware GPIO lines to the kernel's GPIO subsystem so other drivers can consume them by descriptor.

A GPIO controller driver is a provider:

```text
hardware register block or expander
-> gpio_chip
-> gpiolib
-> consumer drivers request named lines
```

Do not mix provider and consumer roles accidentally. A GPIO controller driver exposes lines; a normal device driver consumes lines.

## Core Concepts

- `struct gpio_chip`
- GPIO provider
- GPIO consumer
- hardware line offset
- `ngpio`
- `.direction_input`
- `.direction_output`
- `.get`
- `.set`
- `.get_direction`
- `.set_config`
- `can_sleep`
- `devm_gpiochip_add_data()`
- `gpiochip_get_data()`
- line names
- GPIO ranges
- pinctrl integration
- irqchip integration
- `gpio-controller`
- `#gpio-cells`

## Mental Model

Inside a GPIO controller driver, lines are addressed by hardware offsets:

```text
offset 0 -> bit 0 in GPIO_DATA register
offset 1 -> bit 1 in GPIO_DATA register
...
offset n-1 -> bit n-1
```

Outside the controller driver, consumers use descriptors and role names. They should not know global GPIO numbers or hardware offsets.

## Minimal MMIO GPIO Chip Shape

Private state:

```c
struct demo_gpio {
    struct device *dev;
    void __iomem *base;
    struct gpio_chip gc;
    raw_spinlock_t lock;
};
```

Callbacks:

```c
static int demo_gpio_get(struct gpio_chip *gc, unsigned int offset)
{
    struct demo_gpio *dg = gpiochip_get_data(gc);
    u32 val = readl(dg->base + DEMO_GPIO_DATA);

    return !!(val & BIT(offset));
}

static void demo_gpio_set(struct gpio_chip *gc, unsigned int offset, int value)
{
    struct demo_gpio *dg = gpiochip_get_data(gc);
    unsigned long flags;
    u32 val;

    raw_spin_lock_irqsave(&dg->lock, flags);
    val = readl(dg->base + DEMO_GPIO_DATA);
    if (value)
        val |= BIT(offset);
    else
        val &= ~BIT(offset);
    writel(val, dg->base + DEMO_GPIO_DATA);
    raw_spin_unlock_irqrestore(&dg->lock, flags);
}
```

Registration:

```c
dg->gc.label = dev_name(dev);
dg->gc.parent = dev;
dg->gc.owner = THIS_MODULE;
dg->gc.ngpio = 32;
dg->gc.get = demo_gpio_get;
dg->gc.set = demo_gpio_set;
dg->gc.direction_input = demo_gpio_direction_input;
dg->gc.direction_output = demo_gpio_direction_output;
dg->gc.can_sleep = false;

ret = devm_gpiochip_add_data(dev, &dg->gc, dg);
if (ret)
    return dev_err_probe(dev, ret, "failed to register gpiochip\n");
```

This is a teaching skeleton. Real hardware needs direction registers, interrupt handling, pin configuration, power management, and locking carefully matched to the register model.

## `gpio_chip` Fields

Important fields:

| Field | Purpose |
| --- | --- |
| `label` | Diagnostic name. |
| `parent` | Device that owns the controller. |
| `owner` | Module owner. |
| `ngpio` | Number of lines. |
| `base` | Legacy global base; prefer dynamic assignment. |
| `can_sleep` | True if callbacks may sleep. |
| `names` | Optional per-line names. |
| callbacks | Direction, get, set, config, IRQ mapping. |

For new code, avoid fixed global bases unless a legacy platform requires them.

## Direction Callbacks

Direction input:

```c
static int demo_gpio_direction_input(struct gpio_chip *gc,
                                     unsigned int offset)
{
    struct demo_gpio *dg = gpiochip_get_data(gc);
    unsigned long flags;
    u32 val;

    raw_spin_lock_irqsave(&dg->lock, flags);
    val = readl(dg->base + DEMO_GPIO_DIR);
    val &= ~BIT(offset);
    writel(val, dg->base + DEMO_GPIO_DIR);
    raw_spin_unlock_irqrestore(&dg->lock, flags);

    return 0;
}
```

Direction output:

```c
static int demo_gpio_direction_output(struct gpio_chip *gc,
                                      unsigned int offset, int value)
{
    demo_gpio_set(gc, offset, value);
    /* then set direction output in hardware */
    return 0;
}
```

Hardware often needs output value programmed before switching direction to avoid glitches.

## `can_sleep`

Set:

```c
gc->can_sleep = true;
```

when callbacks perform operations that can sleep, such as I2C or SPI transfers.

Examples:

| Controller Type | `can_sleep` |
| --- | --- |
| MMIO GPIO controller | usually false |
| I2C GPIO expander | true |
| SPI GPIO expander | true |

Consumers must use `gpiod_*_cansleep()` for sleeping providers.

## Device Tree Provider Node

Example:

```dts
gpio2: gpio@2000000 {
    compatible = "example,demo-gpio";
    reg = <0x0 0x02000000 0x0 0x1000>;
    gpio-controller;
    #gpio-cells = <2>;
};
```

Consumer:

```dts
reset-gpios = <&gpio2 5 GPIO_ACTIVE_LOW>;
```

The meaning of `#gpio-cells` is defined by the binding. Commonly:

```text
cell 0: line offset
cell 1: flags such as active-low
```

## Line Names

Line names improve diagnostics:

```dts
gpio-line-names =
    "reset", "enable", "irq", "fault";
```

Inspect:

```sh
gpioinfo
cat /sys/kernel/debug/gpio
```

Line names are not a substitute for consumer properties. Consumers should still request named roles such as `reset-gpios`.

## Pinctrl Interaction

Some SoCs have separate pinctrl and GPIO blocks. GPIO control may require pinmux selection.

The GPIO provider may need GPIO ranges so gpiolib and pinctrl can map GPIO offsets to pins.

Common symptoms of missing pinctrl integration:

- consumer requests the line successfully but pin does not move
- pinmux remains assigned to another function
- GPIO IRQ never fires
- electrical settings are wrong

Debug:

```sh
cat /sys/kernel/debug/pinctrl/*/pinmux-pins
cat /sys/kernel/debug/gpio
```

## IRQ-Capable GPIO Controllers

Many GPIO controllers also provide interrupts. This requires irqchip integration.

High-level responsibilities:

- map GPIO line offsets to IRQs
- configure trigger type
- mask/unmask line interrupts
- acknowledge/clear interrupt status
- handle parent interrupt if cascaded
- avoid sleeping in irqchip callbacks unless using the appropriate nested/threaded model

Do not add IRQ support by making consumers poll GPIO values from hard IRQ context. Use the generic IRQ and gpiolib irqchip integration patterns.

## Locking And Register Updates

GPIO callbacks may run in atomic context for non-sleeping controllers. Use spinlocks/raw spinlocks where appropriate.

Protect read-modify-write sequences:

```c
raw_spin_lock_irqsave(&dg->lock, flags);
val = readl(reg);
val |= BIT(offset);
writel(val, reg);
raw_spin_unlock_irqrestore(&dg->lock, flags);
```

For sleeping controllers, use mutexes and mark `can_sleep = true`.

## Debugging GPIO Providers

List chips:

```sh
gpiodetect
gpioinfo
```

Kernel debugfs:

```sh
cat /sys/kernel/debug/gpio
```

Runtime Device Tree:

```sh
dtc -I fs -O dts /proc/device-tree > /tmp/running.dts
rg 'gpio-controller|gpio-line-names' /tmp/running.dts
```

Logs:

```sh
dmesg | grep -i gpio
```

## Common Failure Modes

| Symptom | Likely Cause | First Checks |
| --- | --- | --- |
| no gpiochip appears | provider probe failed | dmesg, compatible, resources |
| wrong number of lines | bad `ngpio` or binding | driver and DT |
| line toggles wrong bit | offset mapping bug | hardware manual, debugfs |
| consumer sleeps warning | provider marked non-sleeping but sleeps | `can_sleep` |
| line requested but pin unchanged | pinmux/range issue | pinctrl debugfs |
| GPIO IRQ does not fire | irqchip integration or trigger type wrong | `/proc/interrupts`, irqchip |

## Common Mistakes

- Exposing non-general-purpose hardware as GPIO.
- Using fixed global GPIO bases in new code.
- Forgetting `can_sleep = true` for I2C/SPI expanders.
- Returning physical active-low behavior from provider callbacks instead of raw physical line values.
- Mixing consumer policy into provider code.
- Failing to lock read-modify-write register updates.
- Ignoring pinctrl and GPIO ranges.

## Practice Exercises

### Exercise 1: Inspect Existing Providers

On a board:

```sh
gpiodetect
gpioinfo
cat /sys/kernel/debug/gpio
```

Map one gpiochip back to a Device Tree node and driver.

### Exercise 2: Trace A Consumer

Find a `reset-gpios` property, identify the provider chip, and confirm `gpioinfo` shows the line requested by the consumer.

### Exercise 3: Review A GPIO Expander Driver

Read an I2C GPIO expander driver and identify where it sets `can_sleep`, registers `gpio_chip`, and accesses registers.

## Debugging Checklist

- Did the provider bind and register a gpiochip?
- Is `ngpio` correct?
- Are offsets mapped to hardware bits correctly?
- Are callbacks allowed to sleep?
- Are pinctrl ranges and mux states correct?
- Are line names useful for diagnostics?
- If IRQ-capable, does `/proc/interrupts` show expected events?
- Are read-modify-write paths locked?

## Related Topics

- [GPIO Consumer API](gpio-consumer-api.md)
- [GPIO Expanders](gpio-expanders.md)
- [Legacy GPIO Interfaces](legacy-gpio-interfaces.md)
- [Pinctrl](pinctrl.md)
- [Interrupt Processing Model](interrupt-processing-model.md)

## Official References

- [GPIO Driver Interface](https://docs.kernel.org/driver-api/gpio/driver.html)
- [GPIO Descriptor Consumer Interface](https://docs.kernel.org/driver-api/gpio/consumer.html)
- [PINCTRL subsystem](https://docs.kernel.org/driver-api/pin-control.html)

---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# Legacy GPIO Interfaces

## What Problem Does This Solve?

Legacy GPIO APIs and sysfs interfaces appear in older drivers, vendor BSPs, and tutorials, but new kernel driver code should use descriptor-based GPIO APIs.

The old model:

```text
global integer GPIO number
-> gpio_request(37)
-> gpio_direction_output(37, 1)
```

The modern model:

```text
named role in firmware data
-> devm_gpiod_get(dev, "reset", GPIOD_OUT_HIGH)
-> gpiod_set_value_cansleep(reset, 1)
```

The modern model is portable across boards and handles polarity in firmware data.

## Core Concepts

- global GPIO numbers
- integer GPIO API
- `gpio_request()`
- `gpio_direction_input()`
- `gpio_direction_output()`
- `gpio_get_value()`
- `gpio_set_value()`
- `/sys/class/gpio`
- GPIO export
- GPIO descriptor API
- `gpiod_*`
- active-low semantics
- migration
- userspace GPIO character device ABI

## Mental Model

Legacy GPIO interfaces identify lines by global numbers. Descriptor APIs identify lines by role.

```text
legacy:
  "GPIO 37"

descriptor:
  "reset GPIO for this device"
```

The descriptor form lets the same driver work on boards where the reset line is connected to different providers, offsets, or polarities.

## Legacy Kernel API Example

Old-style code:

```c
#include <linux/gpio.h>

#define DEMO_RESET_GPIO 37

ret = gpio_request(DEMO_RESET_GPIO, "demo-reset");
if (ret)
    return ret;

ret = gpio_direction_output(DEMO_RESET_GPIO, 1);
if (ret)
    goto err_free_gpio;

gpio_set_value(DEMO_RESET_GPIO, 0);

err_free_gpio:
gpio_free(DEMO_RESET_GPIO);
```

Problems:

- global number may differ between boards
- polarity is hard-coded in the driver
- ownership is disconnected from firmware resource description
- cleanup is manual
- it does not scale well to multiple instances

## Descriptor Replacement

Device Tree:

```dts
reset-gpios = <&gpio2 5 GPIO_ACTIVE_LOW>;
```

Driver:

```c
#include <linux/gpio/consumer.h>

reset = devm_gpiod_get(dev, "reset", GPIOD_OUT_HIGH);
if (IS_ERR(reset))
    return dev_err_probe(dev, PTR_ERR(reset),
                         "failed to get reset gpio\n");

gpiod_set_value_cansleep(reset, 0);
```

The driver no longer knows the global number or physical polarity.

## Migration Table

| Legacy | Descriptor-Based |
| --- | --- |
| `gpio_request(gpio, label)` | `devm_gpiod_get(dev, name, flags)` |
| `gpio_free(gpio)` | managed cleanup or `gpiod_put()` |
| `gpio_direction_input(gpio)` | `gpiod_direction_input(desc)` |
| `gpio_direction_output(gpio, value)` | `gpiod_direction_output(desc, value)` |
| `gpio_get_value(gpio)` | `gpiod_get_value(desc)` |
| `gpio_set_value(gpio, value)` | `gpiod_set_value(desc, value)` |
| `gpio_to_irq(gpio)` | `gpiod_to_irq(desc)` |
| hard-coded polarity | firmware `GPIO_ACTIVE_LOW/HIGH` |

For providers that can sleep, use:

```c
gpiod_get_value_cansleep(desc);
gpiod_set_value_cansleep(desc, value);
```

## Legacy `/sys/class/gpio`

Old userspace flow:

```sh
echo 37 > /sys/class/gpio/export
echo out > /sys/class/gpio/gpio37/direction
echo 1 > /sys/class/gpio/gpio37/value
```

This interface is deprecated for new designs. Modern userspace should use the GPIO character device ABI through tools such as `libgpiod`:

```sh
gpiodetect
gpioinfo
gpioset gpiochip0 5=1
gpioget gpiochip0 5
```

For product drivers, userspace raw GPIO access should usually be a bring-up or manufacturing tool, not the final application interface.

## Why Global GPIO Numbers Are Fragile

Global numbers can change when:

- gpiochip probe order changes
- a new controller is added
- Device Tree changes
- kernel version changes
- a GPIO expander is added or removed
- a vendor BSP assigns different bases

Hard-coded numbers make drivers board-specific and brittle.

Descriptor APIs avoid this by using device-local mappings.

## Active-Low Migration

Legacy code often does:

```c
gpio_set_value(reset_gpio, 0); /* assert active-low reset */
```

Descriptor code should do:

```c
gpiod_set_value_cansleep(reset, 1); /* assert reset logically */
```

Device Tree carries:

```dts
reset-gpios = <&gpio2 5 GPIO_ACTIVE_LOW>;
```

This makes the driver portable to active-high designs.

## Handling Old BSP Code

When reading older code:

1. Identify the GPIO role.
2. Find where the integer number came from.
3. Move the mapping into Device Tree or firmware data.
4. Replace integer calls with descriptor calls.
5. Preserve logical behavior, not physical levels.
6. Test active-high and active-low reasoning.

Example:

```c
board_reset_gpio = 37;
```

becomes:

```dts
reset-gpios = <&gpio2 5 GPIO_ACTIVE_LOW>;
```

and:

```c
reset = devm_gpiod_get(dev, "reset", GPIOD_OUT_HIGH);
```

## When Legacy APIs Still Appear

You may still see legacy APIs in:

- old vendor BSPs
- platform code
- staging drivers
- tutorials
- board files without Device Tree
- transitional compatibility paths

Do not copy them into new code without a strong reason.

## Debugging Legacy Systems

Check whether sysfs GPIO exists:

```sh
ls /sys/class/gpio
```

Check modern line ownership:

```sh
gpioinfo
cat /sys/kernel/debug/gpio
```

Map old global number to chip/offset:

```text
global number = chip base + offset
```

This is exactly the mapping new code should avoid relying on.

## Common Failure Modes

| Symptom | Likely Cause | First Checks |
| --- | --- | --- |
| old GPIO number now controls wrong pin | gpiochip base changed | debugfs gpio, gpioinfo |
| active-low behavior wrong after migration | physical value copied instead of logical intent | DT flags, gpiod values |
| sysfs GPIO missing | kernel disabled deprecated interface | config, use libgpiod |
| line busy | kernel driver owns it | gpioinfo |
| driver supports only one board | hard-coded number/polarity | move to firmware data |

## Common Mistakes

- Translating `gpio_set_value(gpio, 0)` directly to `gpiod_set_value(desc, 0)` without checking polarity semantics.
- Keeping global GPIO numbers in module parameters.
- Exporting product GPIOs to userspace instead of writing a proper driver.
- Assuming `/sys/class/gpio` exists on modern systems.
- Using legacy APIs in new drivers.
- Ignoring `gpiod_cansleep()`.

## Practice Exercises

### Exercise 1: Convert A Reset Line

Convert:

```c
gpio_direction_output(reset_gpio, 1);
gpio_set_value(reset_gpio, 0);
```

to descriptor APIs using `reset-gpios`.

### Exercise 2: Replace A Module Parameter

Replace:

```c
static int reset_gpio = 37;
module_param(reset_gpio, int, 0444);
```

with a Device Tree property and `devm_gpiod_get()`.

### Exercise 3: Inspect sysfs Versus chardev

Compare:

```sh
ls /sys/class/gpio
gpioinfo
```

on your system.

## Debugging Checklist

- Is the code using integer GPIO APIs?
- What physical role does each integer GPIO represent?
- Can that role be represented as `*-gpios`?
- Is polarity represented in firmware data?
- Does the converted code use logical values?
- Does the provider sleep?
- Is raw userspace GPIO access still needed?

## Related Topics

- [GPIO Consumer API](gpio-consumer-api.md)
- [GPIO Controller Drivers](gpio-controller-drivers.md)
- [Device Tree Hardware Description](../fundamentals/device-tree-hardware-description.md)
- [User-Space Hardware Access Vs Kernel Drivers](../fundamentals/userspace-hardware-access-vs-kernel-drivers.md)

## Official References

- [GPIO Descriptor Consumer Interface](https://docs.kernel.org/driver-api/gpio/consumer.html)
- [GPIO Driver Interface](https://docs.kernel.org/driver-api/gpio/driver.html)
- [GPIO Character Device Userspace API](https://docs.kernel.org/userspace-api/gpio/chardev.html)

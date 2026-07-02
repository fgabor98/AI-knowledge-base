---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# User-Space Hardware Access Vs Kernel Drivers

## What Problem Does This Solve?

Embedded prototypes often access GPIO, I2C, or SPI from userspace. That is useful for bring-up, register exploration, and quick experiments. Product drivers often need kernel integration for interrupts, power management, resource sharing, subsystem APIs, security policy, and stable userspace behavior.

This page helps decide when userspace hardware access is acceptable and when it should become a kernel driver.

## Core Concepts

- userspace GPIO tools
- GPIO character device ABI
- `i2c-dev`
- `spidev`
- userspace register poking
- kernel subsystem drivers
- ownership
- resource arbitration
- interrupt handling
- runtime PM
- suspend/resume
- stable ABI
- security policy
- product supportability

## Mental Model

Userspace access is good for experiments. Kernel drivers are for integrated ownership.

```text
prototype:
  "Can I talk to this chip?"
  userspace tools may be enough

product:
  "Can the system safely own this device for years?"
  kernel driver or subsystem integration is often required
```

The moment the device needs interrupts, power sequencing, shared resources, suspend/resume, standard Linux APIs, or a stable ABI, raw userspace access starts to break down.

## Common Userspace Access Paths

### GPIO

Modern userspace GPIO uses the GPIO character device ABI through tools/libraries such as `libgpiod`.

Examples:

```sh
gpiodetect
gpioinfo
gpioset gpiochip0 12=1
gpioget gpiochip0 12
```

Good for:

- board bring-up
- checking pin wiring
- toggling reset lines in a lab
- proving polarity
- simple manufacturing tests

Limitations:

- no natural integration with LED/input/regulator/reset subsystems
- coordination with kernel drivers is critical
- userspace timing is not deterministic
- security policy must expose the GPIO chip/line
- suspend/resume and runtime PM interactions are limited

### I2C

Userspace I2C commonly uses `/dev/i2c-*` through `i2c-dev`.

Examples:

```sh
i2cdetect -y 1
i2cget -y 1 0x48 0x00
i2cset -y 1 0x48 0x01 0x80
```

Good for:

- checking whether a device ACKs
- reading simple registers
- comparing against datasheet examples
- early board validation

Limitations:

- unsafe probing can disturb devices
- no interrupt handling
- no automatic regulator/clock/runtime PM integration for the client
- conflicts if a kernel driver owns the same address
- no standard subsystem ABI for the device's function

### SPI

Userspace SPI often uses `spidev`.

Example:

```sh
spidev_test -D /dev/spidev1.0 -s 1000000 -p "\x9f"
```

Good for:

- verifying chip select wiring
- checking SPI mode and speed
- reading an ID register
- quick protocol experiments

Limitations:

- no IRQ integration
- no standard subsystem representation
- generic raw access may be inappropriate in production
- chip-select, power, reset, and locking policy can become fragile

## When Userspace Access Is Appropriate

Userspace access can be reasonable when:

- you are doing board bring-up
- the hardware operation is simple and low risk
- no kernel driver owns the resource
- timing requirements are loose
- there are no interrupts
- power management is irrelevant or externally handled
- security policy explicitly allows it
- the interface is for lab/manufacturing use, not product runtime

Examples:

- reading an I2C temperature register during bring-up
- toggling a reset GPIO on a lab bench
- sending a single SPI command to verify wiring
- scanning a bus on an unpopulated prototype

Even then, document what was touched and restore the system to a known state.

## When A Kernel Driver Is Needed

Write or use a kernel driver when the device needs:

- interrupt handling
- standard subsystem integration
- runtime power management
- suspend/resume behavior
- shared resource arbitration
- regulator, clock, reset, pinctrl sequencing
- DMA
- memory-mapped I/O
- low-latency response
- stable product ABI
- safe multi-process access
- hotplug/remove handling
- kernel-managed buffering
- module autoloading
- security boundaries stronger than raw bus access

Examples:

| Device | Better Kernel Integration |
| --- | --- |
| ADC or sensor | IIO |
| button or keypad | input subsystem |
| GPIO-controlled LED | LED subsystem |
| watchdog | watchdog subsystem |
| raw flash/EEPROM | MTD, nvmem, EEPROM drivers |
| network controller | netdev |
| audio codec | ASoC |
| display panel | DRM/panel |
| power rail | regulator framework |

## Ownership Rules

Only one owner should control a hardware resource at a time.

Bad:

```text
kernel driver owns I2C address 0x48
userspace i2cset writes registers at 0x48 anyway
```

Possible consequences:

- driver state becomes stale
- hardware mode changes unexpectedly
- interrupt status is cleared behind the driver's back
- power state assumptions break
- data corruption

Check I2C ownership:

```sh
i2cdetect -y 1
ls /sys/bus/i2c/devices
```

Check GPIO ownership:

```sh
gpioinfo
```

Check SPI devices:

```sh
ls /sys/bus/spi/devices
ls /dev/spidev*
```

Do not bypass a kernel driver to "just tweak one register" on a running product unless you fully understand the driver's state model.

## Power Management Boundary

Userspace raw access often does not know that a device depends on:

- regulators
- clocks
- resets
- pinctrl states
- power domains
- runtime PM state
- autosuspend delays

A kernel driver can do:

```text
enable regulator
prepare/enable clock
deassert reset
select pinctrl state
perform transfer
mark device idle
runtime suspend later
```

A userspace script usually cannot coordinate all of that safely without duplicating kernel policy in fragile ways.

## Interrupt Boundary

If a device signals events with an interrupt, userspace polling is often the wrong model.

Kernel driver advantages:

- request IRQ
- acknowledge hardware correctly
- use threaded handlers
- wake wait queues
- report events through subsystem ABI
- coordinate with suspend/resume wakeup

Userspace GPIO edge polling can be useful for simple experiments, but it does not replace a subsystem driver for product event handling.

## ABI Boundary

Raw bus access exposes implementation details:

```text
userspace knows register addresses
userspace knows bit fields
userspace sequences power/reset manually
userspace must handle hardware quirks
```

A product ABI should expose intent:

```text
read temperature
set output voltage
read button event
enable capture
get samples
```

Kernel subsystems often already provide that intent-based ABI. Use them where possible.

## Security Boundary

Giving userspace raw access to buses can be broad authority.

Risks:

- reprogramming unrelated devices on the same bus
- disabling regulators or clocks indirectly
- modifying secure peripherals
- corrupting EEPROM/flash
- bypassing driver validation
- creating denial-of-service paths

Product policy should answer:

- which users can access the bus?
- which devices can they affect?
- can they write or only read?
- is access available in production?
- is it logged or audited?
- is it disabled after manufacturing?

## Prototype-To-Driver Migration

Typical migration:

```text
userspace experiment
-> identify bus address, protocol, polarity, timing
-> write Device Tree binding/node
-> write kernel driver using proper subsystem
-> expose standard userspace ABI
-> remove raw bus access from production image
```

Example I2C migration:

Prototype:

```sh
i2cget -y 1 0x48 0x00
```

Device Tree:

```dts
temperature-sensor@48 {
    compatible = "example,tmp102";
    reg = <0x48>;
    vdd-supply = <&vdd_3v3>;
    interrupt-parent = <&gpio1>;
    interrupts = <7 IRQ_TYPE_LEVEL_LOW>;
};
```

Kernel driver:

```text
i2c_driver
-> regmap
-> optional IRQ
-> IIO or hwmon subsystem
```

Userspace product interface:

```text
/sys/bus/iio/devices/iio:deviceX/in_temp_input
```

or the hwmon ABI, depending on device purpose.

## Example: GPIO Reset Line

Prototype:

```sh
gpioset gpiochip0 12=0
sleep 0.01
gpioset gpiochip0 12=1
```

Driver-oriented Device Tree:

```dts
reset-gpios = <&gpio0 12 GPIO_ACTIVE_LOW>;
```

Driver:

```c
priv->reset = devm_gpiod_get(dev, "reset", GPIOD_OUT_HIGH);
if (IS_ERR(priv->reset))
    return dev_err_probe(dev, PTR_ERR(priv->reset),
                         "failed to get reset gpio\n");

gpiod_set_value_cansleep(priv->reset, 1);
usleep_range(10000, 12000);
gpiod_set_value_cansleep(priv->reset, 0);
```

This keeps reset ownership with the driver that understands the device state.

## Example: SPI Register Read

Prototype:

```sh
spidev_test -D /dev/spidev1.0 -s 1000000 -p "\x9f"
```

Kernel direction:

- define a real `compatible`
- write a SPI driver
- use `spi_sync_transfer()`
- use regmap if the device has register-like access
- expose the function through a subsystem

Avoid shipping a product daemon that knows every SPI register unless the product deliberately chooses raw SPI as its ABI and accepts the support/security cost.

## Decision Table

| Question | If Yes |
| --- | --- |
| Is this only board bring-up or manufacturing? | Userspace access may be acceptable. |
| Does the device have interrupts? | Prefer kernel driver. |
| Does it need regulators/clocks/resets/pinctrl? | Prefer kernel driver. |
| Does a standard subsystem exist? | Use the subsystem. |
| Must multiple processes share it safely? | Prefer kernel driver/subsystem. |
| Is timing important? | Prefer kernel driver. |
| Is the register protocol product ABI? | Reconsider; expose intent instead. |
| Is raw bus access allowed in production security policy? | If no, remove/disable userspace access. |
| Does another kernel driver own the device? | Do not bypass it. |

## Product Policy Recommendations

For production images:

- disable unused raw bus access
- avoid exposing `spidev` for devices with real drivers
- restrict `/dev/i2c-*` and GPIO chip permissions
- document any manufacturing-only access
- remove broad debug tools from locked-down images where required
- prefer subsystem ABIs
- keep Device Tree ownership clear
- test suspend/resume and runtime PM with the final driver

For development images:

- keep tools available
- label them as bring-up tools
- document safe addresses and lines
- avoid scripts that fight kernel drivers

## Common Failure Modes

| Symptom | Likely Cause | First Checks |
| --- | --- | --- |
| Kernel driver behaves strangely after `i2cset` | userspace changed registers behind driver | ownership, logs |
| GPIO tool says line busy | kernel driver already requested line | `gpioinfo` |
| SPI works in script but not in product | missing PM/IRQ/subsystem integration | driver design |
| Device fails after suspend | userspace access lacks PM handling | suspend logs |
| Security review rejects design | raw bus access too broad | permissions and ABI |
| Product app depends on register layout | raw hardware protocol leaked into ABI | subsystem/interface design |

## Common Mistakes

- Treating successful `i2cget` as proof a product driver is unnecessary.
- Shipping `spidev` because the prototype used it.
- Bypassing an existing kernel driver for convenience.
- Giving broad bus write permissions to product applications.
- Encoding board GPIO numbers in scripts instead of Device Tree.
- Ignoring interrupts and power management until late.
- Creating a private ABI when a subsystem ABI exists.
- Leaving manufacturing debug access enabled in production by accident.

## Practice Exercises

### Exercise 1: Classify A Prototype

Pick a userspace hardware script and answer:

- which resources does it touch?
- does a kernel driver already own them?
- does it need interrupts?
- does it need power sequencing?
- what product ABI should userspace really see?

### Exercise 2: Move A GPIO Into Device Tree

Replace a hard-coded userspace GPIO number with a Device Tree `reset-gpios` property and a driver-side `devm_gpiod_get()`.

### Exercise 3: Identify The Right Subsystem

For three devices on your board, choose the expected subsystem:

- sensor
- button
- LED

Then find the matching kernel docs or existing driver examples.

## Debugging Checklist

- Is userspace access for bring-up, manufacturing, debug, or production?
- Does a kernel driver already own the device/resource?
- Does a standard subsystem already exist?
- Are power, reset, clock, pinctrl, and regulator dependencies handled?
- Are interrupts needed?
- Is raw access safe under suspend/resume?
- Are permissions intentionally restricted?
- Is the product ABI stable and intent-based?
- Can the design be supported without exposing hardware internals?

## Related Topics

- [I2C Client Drivers](../driver-interfaces/i2c-client-drivers.md)
- [SPI Device Drivers](../driver-interfaces/spi-device-drivers.md)
- [GPIO Consumer API](../driver-interfaces/gpio-consumer-api.md)
- [IIO Subsystem](../driver-interfaces/iio-subsystem.md)
- [Input Subsystem](../driver-interfaces/input-subsystem.md)
- [Runtime PM](../power-management/runtime-pm.md)
- [Module Signing And Hardening](../configuration-and-platform-policy/module-signing-and-hardening.md)

## Official References

- [GPIO Character Device Userspace API](https://docs.kernel.org/userspace-api/gpio/chardev.html)
- [I2C device interface](https://docs.kernel.org/i2c/dev-interface.html)
- [SPI userspace API](https://docs.kernel.org/spi/spidev.html)
- [Driver API](https://docs.kernel.org/driver-api/index.html)

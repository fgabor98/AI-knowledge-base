---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# Common Driver Interfaces

This track covers the kernel subsystem APIs that embedded drivers commonly use to talk to board resources, peripheral buses, and standard userspace-facing subsystems.

It builds on:

- [Linux Device Driver Fundamentals](../fundamentals/index.md)
- [Kernel Source, Build, And Tailoring](../source-build-and-tailoring/index.md)
- [Kernel Foundations For Driver Developers](../foundations/index.md)

## What Problem Does This Solve?

Real drivers rarely control hardware in isolation. A driver usually consumes shared platform resources and registers with standard kernel subsystems:

```text
Device Tree or firmware description
-> GPIO, IRQ, clock, reset, regulator, pinctrl, DMA resources
-> bus API such as I2C or SPI
-> helper API such as regmap
-> subsystem ABI such as IIO, input, networking, TTY, or PWM
```

This chapter teaches the common interfaces that prevent drivers from hard-coding board details or inventing private userspace ABIs.

## Learning Materials

1. [GPIO Consumer API](gpio-consumer-api.md)
2. [GPIO Controller Drivers](gpio-controller-drivers.md)
3. [GPIO Expanders](gpio-expanders.md)
4. [Legacy GPIO Interfaces](legacy-gpio-interfaces.md)
5. [Interrupt Processing Model](interrupt-processing-model.md)
6. [IRQ Handling](irq-handling.md)
7. [Threaded Interrupts](threaded-interrupts.md)
8. [I2C Client Drivers](i2c-client-drivers.md)
9. [SPI Device Drivers](spi-device-drivers.md)
10. [UART And TTY Integration Overview](uart-tty-integration.md)
11. [CAN Driver Integration Overview](can-driver-integration.md)
12. [PWM Driver Overview](pwm-drivers.md)
13. [Regmap](regmap.md)
14. [Clocks](clocks.md)
15. [Resets](resets.md)
16. [Regulators](regulators.md)
17. [Pinctrl](pinctrl.md)
18. [DMA Basics](dma-basics.md)
19. [IIO Subsystem](iio-subsystem.md)
20. [IIO Channels And Sysfs](iio-channels-and-sysfs.md)
21. [IIO Triggers And Buffers](iio-triggers-and-buffers.md)
22. [Input Subsystem](input-subsystem.md)
23. [Polled Input Devices](polled-input-devices.md)
24. [IRQ-Based Input Devices](irq-based-input-devices.md)

## Mental Model

Most of these APIs follow the same provider/consumer pattern:

```text
provider driver
  exposes a resource or service

firmware data
  maps consumer role names to provider resources

consumer driver
  requests a named role through a kernel API
```

Example:

```dts
reset-gpios = <&gpio2 5 GPIO_ACTIVE_LOW>;
vdd-supply = <&vdd_3v3>;
clocks = <&clkctrl 12>;
clock-names = "core";
```

Driver:

```c
reset = devm_gpiod_get(dev, "reset", GPIOD_OUT_HIGH);
vdd = devm_regulator_get(dev, "vdd");
clk = devm_clk_get(dev, "core");
```

The driver asks for roles. Board data maps those roles to real hardware.

## Interface Categories

| Category | Interfaces | Main Question |
| --- | --- | --- |
| Board resources | GPIO, IRQ, clocks, resets, regulators, pinctrl | What does this device need from the board or SoC? |
| Peripheral buses | I2C, SPI, UART, CAN | How does the CPU communicate with the device? |
| Register helpers | regmap | Can register access be centralized and cached safely? |
| Data subsystems | IIO, input, networking, TTY | What standard ABI should userspace see? |
| Movement engines | DMA, DMAengine | How does data move efficiently between memory and devices? |
| Providers | GPIO controllers, expanders, PWM providers | Does this driver expose resources to other drivers? |

## Study Order

For a first driver that talks to a peripheral:

```text
GPIO consumer API
-> IRQ handling and threaded interrupts
-> I2C or SPI client driver
-> regmap
-> clocks/resets/regulators/pinctrl
-> standard subsystem ABI
```

For a board-level resource provider:

```text
GPIO controller or expander
-> irqchip integration if interrupt-capable
-> pinctrl interaction
-> consumer-side testing from another driver
```

For a sensor or converter:

```text
I2C/SPI
-> regmap
-> IRQ or trigger
-> IIO channels
-> IIO buffers/events if streaming
```

For buttons and switches:

```text
GPIO consumer
-> IRQ or polling
-> input subsystem
-> wakeup policy if needed
```

## Design Rules

- Use descriptor APIs instead of global numbers.
- Use named resources instead of positional guesses when a device has several resources.
- Preserve `-EPROBE_DEFER` from provider lookups.
- Use `devm_*` helpers for per-device resource lifetime where appropriate.
- Keep hard IRQ handlers short and non-sleeping.
- Move sleepable bus transfers into threaded IRQs, workqueues, or process context.
- Use regmap for register-oriented I2C/SPI/MMIO devices when it simplifies access.
- Use standard subsystems before inventing character devices or private sysfs files.
- Treat Device Tree properties as ABI.
- Debug runtime state, not only source files.

## Completion Criteria

You are ready to move on when you can:

- request required and optional GPIOs by descriptor
- explain provider versus consumer GPIO drivers
- retrieve and request IRQs from Device Tree-described devices
- choose hard IRQ, threaded IRQ, or workqueue handling
- implement a minimal I2C client driver
- implement a minimal SPI device driver
- decide when to use regmap
- request clocks, resets, regulators, and pinctrl states in probe/runtime PM paths
- choose IIO for sensors and converters
- choose input for buttons, switches, and human-input events
- understand why CAN appears as a network interface and UART appears through TTY
- explain the DMA mapping ownership contract at a high level

## Common Mistakes

- Hard-coding GPIO numbers, IRQ numbers, addresses, or clock names.
- Treating Device Tree as a bag of arbitrary driver parameters.
- Sleeping in a hard IRQ handler.
- Using raw I2C/SPI userspace access as the product interface.
- Implementing a private sysfs layout where IIO/input/netdev/TTY already fits.
- Changing shared clock rates or regulator voltages without board-level validation.
- Forgetting pinctrl, reset, power, or clock sequencing when a bus transaction fails.
- Ignoring cache coherency and ownership rules in DMA paths.
- Debugging a missing device without checking runtime Device Tree and provider drivers.

## Related Topics

- [Linux Device Driver Fundamentals](../fundamentals/index.md)
- [Kernel Execution And Concurrency](../execution-and-concurrency/index.md)
- [Kernel Memory And I/O](../memory-and-io/index.md)
- [Power Management](../power-management/index.md)
- [Device Tree](../../device-tree/index.md)

## Official References

- [Driver API](https://docs.kernel.org/driver-api/index.html)
- [GPIO Descriptor Consumer Interface](https://docs.kernel.org/driver-api/gpio/consumer.html)
- [Linux generic IRQ handling](https://docs.kernel.org/core-api/genericirq.html)
- [Implementing I2C device drivers](https://docs.kernel.org/i2c/writing-clients.html)
- [Overview of Linux kernel SPI support](https://docs.kernel.org/spi/spi-summary.html)
- [Industrial I/O](https://docs.kernel.org/driver-api/iio/index.html)
- [Input Subsystem](https://docs.kernel.org/driver-api/input.html)

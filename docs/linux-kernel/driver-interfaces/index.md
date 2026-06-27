---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# Common Driver Interfaces

This track covers the kernel subsystem APIs that embedded drivers commonly use to talk to board resources and peripheral buses.

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

Drivers should use subsystem APIs instead of open-coding board control. GPIOs, IRQs, clocks, resets, regulators, pinctrl states, and bus transfers are shared platform resources with provider and consumer relationships.

## Completion Criteria

- Request GPIOs, IRQs, clocks, resets, regulators, and pinctrl states through consumer APIs.
- Implement a minimal I2C or SPI client driver.
- Use regmap for register-oriented bus devices.
- Decide when a driver should be a GPIO controller, IIO device, or input device.
- Explain when DMA belongs in the driver design.

## Related Topics

- [Linux Device Driver Fundamentals](../fundamentals/index.md)
- [Kernel Memory And I/O](../memory-and-io/index.md)
- [Device Tree](../../device-tree/index.md)

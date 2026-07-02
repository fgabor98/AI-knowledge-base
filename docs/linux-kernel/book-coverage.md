---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# Madieu Book Topic Coverage

This page maps the topics named in John Madieu's *Linux Device Driver Development - Second Edition* publisher summary and official code repository to this Linux kernel chapter.

The purpose is not to reproduce the book's table of contents. It is to make sure each referenced learning theme has a home in this knowledge base.

## Coverage Sources

- Publisher summary: `https://static.packt-cdn.com/products/9781803240060/summary`
- Official source repository: `https://github.com/PacktPublishing/Linux-Device-Driver-Development-Second-Edition`

## Publisher Summary Topics

| Book topic | Knowledge base location |
|---|---|
| Download, configure, build, and tailor the Linux kernel | [Kernel Source, Build, And Tailoring](source-build-and-tailoring/index.md) |
| Describe hardware using Device Tree | [Device Tree Hardware Description](fundamentals/device-tree-hardware-description.md), [Device Tree Matching From Drivers](fundamentals/device-tree-matching.md) |
| Platform drivers | [Platform Devices And Platform Drivers](fundamentals/platform-devices-and-drivers.md) |
| I2C device drivers | [I2C Client Drivers](driver-interfaces/i2c-client-drivers.md) |
| SPI device drivers | [SPI Device Drivers](driver-interfaces/spi-device-drivers.md) |
| Concurrency managed workqueues | [Concurrency Managed Workqueues](execution-and-concurrency/concurrency-managed-workqueues.md) |
| Timekeeping and time-related APIs | [Timekeeping And Kernel Timers](execution-and-concurrency/timekeeping-and-kernel-timers.md), [Hrtimers](execution-and-concurrency/hrtimers.md) |
| Regmap framework | [Regmap](driver-interfaces/regmap.md) |
| DMA memory copies | [DMA Basics](driver-interfaces/dma-basics.md), [DMA Mapping Basics](memory-and-io/dma-mapping-basics.md) |
| GPIO | [GPIO Consumer API](driver-interfaces/gpio-consumer-api.md), [GPIO Controller Drivers](driver-interfaces/gpio-controller-drivers.md) |
| IIO subsystem | [IIO Subsystem](driver-interfaces/iio-subsystem.md) |
| Input subsystem | [Input Subsystem](driver-interfaces/input-subsystem.md) |
| Locking primitives | [Locking And Atomics](execution-and-concurrency/locking-and-atomics.md) |
| IRQ management and interrupt propagation | [Interrupt Processing Model](driver-interfaces/interrupt-processing-model.md), [IRQ Handling](driver-interfaces/irq-handling.md), [Threaded Interrupts](driver-interfaces/threaded-interrupts.md) |
| Memory management | [Kernel Memory Allocation](memory-and-io/kernel-memory-allocation.md), [Kernel Virtual Memory And VMAs](memory-and-io/kernel-virtual-memory-and-vmas.md) |
| Avoiding user-space GPIO, I2C, and SPI driver shortcuts | [User-Space Hardware Access Vs Kernel Drivers](fundamentals/userspace-hardware-access-vs-kernel-drivers.md) |

## Official Repository Example Topics

| Repository example | Knowledge base location |
|---|---|
| Hello world modules and module parameters | [Kernel Module Lifecycle](fundamentals/kernel-module-lifecycle.md), [Module Parameters And Driver Logging](fundamentals/module-parameters-and-logging.md) |
| Workqueues, tasklets, timers, hrtimers, wait queues | [Kernel Execution And Concurrency](execution-and-concurrency/index.md) |
| Character device driver | [Character Device Basics](fundamentals/character-device-basics.md) |
| Platform dummy character device | [Platform Devices And Platform Drivers](fundamentals/platform-devices-and-drivers.md) |
| I2C EEPROM driver | [I2C Client Drivers](driver-interfaces/i2c-client-drivers.md) |
| SPI EEPROM driver | [SPI Device Drivers](driver-interfaces/spi-device-drivers.md) |
| `kmalloc`, `vmalloc`, and VMA examples | [Kernel Memory And I/O](memory-and-io/index.md) |
| Single-buffer and scatter-gather DMA | [Single-Buffer DMA](memory-and-io/single-buffer-dma.md), [Scatter-Gather DMA](memory-and-io/scatter-gather-dma.md) |
| Kobjects, sysfs groups, and pollable sysfs attributes | [Kobjects And Sysfs Groups](fundamentals/kobjects-and-sysfs-groups.md), [Pollable Sysfs Attributes](fundamentals/pollable-sysfs-attributes.md) |
| IIO dummy random device and channel attributes | [IIO Channels And Sysfs](driver-interfaces/iio-channels-and-sysfs.md) |
| GPIO controller and MCP23016 expander drivers | [GPIO Controller Drivers](driver-interfaces/gpio-controller-drivers.md), [GPIO Expanders](driver-interfaces/gpio-expanders.md) |
| GPIO descriptor and legacy GPIO examples | [GPIO Consumer API](driver-interfaces/gpio-consumer-api.md), [Legacy GPIO Interfaces](driver-interfaces/legacy-gpio-interfaces.md) |
| IRQ-based and polled input buttons | [IRQ-Based Input Devices](driver-interfaces/irq-based-input-devices.md), [Polled Input Devices](driver-interfaces/polled-input-devices.md) |

## Related Topics

- [Linux Kernel Programming](index.md)
- [Linux Kernel Build System](../build-systems/advanced/linux-kernel/index.md)
- [Device Tree](../device-tree/index.md)

## Official References

- [Driver API](https://docs.kernel.org/driver-api/index.html)
- [Core API](https://docs.kernel.org/core-api/index.html)
- [Device Tree bindings](https://docs.kernel.org/devicetree/bindings/)
- [Building external modules](https://docs.kernel.org/kbuild/modules.html)
- [DMA API HOWTO](https://docs.kernel.org/core-api/dma-api-howto.html)
- [Linux generic IRQ handling](https://docs.kernel.org/core-api/genericirq.html)

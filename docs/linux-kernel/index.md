---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# Linux Kernel Programming

Kernel programming topics focused on driver development, kernel subsystems, device tree, debugging, and embedded board support.

For kernel build mechanics, start with the [Linux Kernel Build System](../build-systems/advanced/linux-kernel/index.md).

## Learning Materials

1. [Official Linux Kernel Documentation Reading Checklist](official-docs-reading/index.md)
2. [Kernel Foundations For Driver Developers](foundations/index.md)
3. [Kernel Source, Build, And Tailoring](source-build-and-tailoring/index.md)
4. [Linux Device Driver Fundamentals](fundamentals/index.md)
5. [Common Driver Interfaces](driver-interfaces/index.md)
6. [Kernel Execution And Concurrency](execution-and-concurrency/index.md)
7. [Kernel Memory And I/O](memory-and-io/index.md)
8. [Kernel Configuration And Platform Policy](configuration-and-platform-policy/index.md)
9. [Kernel Debugging Basics](debugging/index.md)
10. [Power Management](power-management/index.md)
11. [Remoteproc, RPMsg, And Heterogeneous SoCs](remoteproc-rpmsg/index.md)
12. [Madieu Book Topic Coverage](book-coverage.md)

## Chapter Boundaries

This chapter is about how kernel code behaves at runtime: drivers, kernel APIs, hardware resources, execution contexts, debugging, and board-level integration.

The build system chapter owns Kconfig, Kbuild, defconfigs, cross-builds, module installation, kernel release artifacts, and reproducible kernel builds. This chapter references those topics only where a runtime concept depends on how the driver was built or configured.

## Suggested Study Order

Start with the foundations track if kernel development is still new. Then learn source/build basics and driver fundamentals before moving into hardware resource APIs, execution context, memory, and debugging.

```text
kernel mental model and lab workflow
-> source, config, and external module build basics
-> module basics
-> device model and probe/remove
-> device tree matching and resource lookup
-> GPIO/I2C/SPI/IRQ/regmap/clocks/resets/regulators/pinctrl
-> execution context, locking, memory, MMIO, DMA
-> debugging and power management
-> remoteproc, RPMsg, and multi-core SoC integration
```

## Kernel Documentation Reading Path

Use the official kernel documentation in a deliberate order. The docs are broad,
and jumping straight into subsystem internals usually creates confusion.

Beginner orientation:

1. [Development process](https://docs.kernel.org/process/index.html)
2. [Linux kernel coding style](https://docs.kernel.org/process/coding-style.html)
3. [Driver API overview](https://docs.kernel.org/driver-api/index.html)
4. [Driver Basics](https://docs.kernel.org/driver-api/basics.html)
5. [Driver Model](https://docs.kernel.org/driver-api/driver-model/index.html)
6. [Core API overview](https://docs.kernel.org/core-api/index.html)

Driver development core:

1. [Building external modules](https://docs.kernel.org/kbuild/modules.html)
2. [Device drivers infrastructure](https://docs.kernel.org/driver-api/infrastructure.html)
3. [Device Tree bindings](https://docs.kernel.org/devicetree/bindings/)
4. [Firmware loading](https://docs.kernel.org/driver-api/firmware/request_firmware.html)
5. [DMA API HOWTO](https://docs.kernel.org/core-api/dma-api-howto.html)
6. [Linux generic IRQ handling](https://docs.kernel.org/core-api/genericirq.html)

Concurrency and runtime behavior:

1. [Unreliable Guide To Hacking The Linux Kernel](https://docs.kernel.org/kernel-hacking/hacking.html)
2. [Unreliable Guide To Locking](https://docs.kernel.org/kernel-hacking/locking.html)
3. [Workqueue](https://docs.kernel.org/core-api/workqueue.html)
4. [Completions](https://docs.kernel.org/scheduler/completion.html)
5. [Delay and sleep mechanisms](https://docs.kernel.org/timers/delay_sleep_functions.html)
6. [Memory allocation guide](https://docs.kernel.org/core-api/memory-allocation.html)

Subsystem and hardware interfaces:

1. [GPIO descriptor consumer interface](https://docs.kernel.org/driver-api/gpio/consumer.html)
2. [I2C and SMBus](https://docs.kernel.org/i2c/index.html)
3. [SPI](https://docs.kernel.org/spi/index.html)
4. [Regmap](https://docs.kernel.org/driver-api/regmap.html)
5. [Common Clock Framework](https://docs.kernel.org/driver-api/clk.html)
6. [Regulator API](https://docs.kernel.org/driver-api/regulator.html)
7. [PINCTRL subsystem](https://docs.kernel.org/driver-api/pin-control.html)
8. [Industrial I/O](https://docs.kernel.org/driver-api/iio/index.html)
9. [Input subsystem](https://docs.kernel.org/input/index.html)

Debugging and validation:

1. [Bug hunting](https://docs.kernel.org/admin-guide/bug-hunting.html)
2. [Dynamic debug HOWTO](https://docs.kernel.org/admin-guide/dynamic-debug-howto.html)
3. [Linux tracing documentation](https://docs.kernel.org/trace/index.html)
4. [ftrace](https://docs.kernel.org/trace/ftrace.html)
5. [Debugging advice for Linux kernel developers](https://docs.kernel.org/process/debugging/index.html)
6. [KASAN](https://docs.kernel.org/dev-tools/kasan.html)
7. [Sparse](https://docs.kernel.org/dev-tools/sparse.html)
8. [Coccinelle](https://docs.kernel.org/dev-tools/coccinelle.html)

Platform, power, and heterogeneous systems:

1. [CPU and device power management](https://docs.kernel.org/driver-api/pm/index.html)
2. [Runtime PM](https://docs.kernel.org/power/runtime_pm.html)
3. [System sleep states](https://docs.kernel.org/admin-guide/pm/sleep-states.html)
4. [CPU idle time management](https://docs.kernel.org/admin-guide/pm/cpuidle.html)
5. [CPU performance scaling](https://docs.kernel.org/admin-guide/pm/cpufreq.html)
6. [Remote Processor Framework](https://docs.kernel.org/staging/remoteproc.html)
7. [Remote Processor Messaging](https://docs.kernel.org/staging/rpmsg.html)

For each topic, use this reading loop:

```text
official overview
-> relevant header files
-> two or three in-tree drivers
-> implementation source only when behavior is unclear
-> local lab or trace to confirm understanding
```

## Roadmap Summary

### Kernel Foundations For Driver Developers

- kernel mental model
- kernel C survival guide
- reading kernel source
- development lab setup
- driver development workflow
- debugging ladder
- failure taxonomy
- execution context primer
- device model primer
- small lab progression
- kernel documentation reading guide for beginners

### Kernel Source, Build, And Tailoring

- kernel source acquisition
- configuring and tailoring the kernel
- kernel build and install overview
- external module build prerequisites
- kernel image, DTB, and module artifacts

### Linux Device Driver Fundamentals

- kernel module lifecycle
- built-in drivers vs loadable modules
- device tree hardware description
- device tree overlays
- `probe` and `remove`
- platform devices and platform drivers
- device tree matching
- `of_match_table`
- `compatible` strings from the driver side
- resource lookup
- `devm_*` managed allocation
- character device basics
- sysfs attributes
- pollable sysfs attributes
- kobjects and sysfs groups
- device classes, uevents, and udev
- module parameters
- driver logging with `dev_*`
- user-space hardware access vs kernel drivers

### Common Driver Interfaces

- GPIO consumer API
- GPIO controller drivers
- GPIO expanders
- legacy GPIO interfaces
- IRQ handling
- threaded interrupts
- I2C client drivers
- SPI device drivers
- UART/TTY integration overview
- CAN driver integration overview
- PWM driver overview
- regmap
- clocks
- resets
- regulators
- pinctrl
- DMA basics
- IIO subsystem
- IIO channels and sysfs
- IIO triggers and buffers
- input subsystem
- polled input devices
- IRQ-based input devices
- interrupt processing model

### Kernel Execution And Concurrency

- interrupt context vs process context
- sleepable vs atomic code
- bottom halves, softirqs, and tasklets
- locking
- atomic operations
- workqueues
- concurrency managed workqueues
- timers
- hrtimers
- kernel timekeeping
- wait queues
- completions
- lifetime and reference counting

### Kernel Memory And I/O

- kernel memory allocation
- allocation flags
- kernel virtual memory and VMAs
- MMIO
- register accessors
- userspace copy helpers
- `ioctl` ABI basics
- DMA mapping basics
- single-buffer DMA
- scatter-gather DMA

### Kernel Configuration And Platform Policy

- debug configs vs production configs
- built-in vs module policy
- kernel command line policy
- namespaces and cgroups overview
- watchdog-related options
- module signing
- kernel hardening options
- LSM overview
- initramfs-related options
- config review workflow

### Kernel Debugging Basics

- `dmesg`
- log levels
- dynamic debug
- ftrace
- tracepoints
- perf overview
- debugfs
- sysfs inspection
- kgdb basics
- crash and panic logs
- watchdog reset diagnosis

### Power Management

- runtime PM
- system suspend/resume
- wake sources
- cpuidle
- cpufreq
- power domains
- regulator constraints
- clock gating
- device tree power dependencies
- suspend/resume debugging

### Remoteproc, RPMsg, And Heterogeneous SoCs

- remoteproc framework
- firmware loading
- reserved memory
- virtio and RPMsg
- PRU integration overview
- R5/M4 firmware lifecycle
- remote core logs
- crash handling
- device tree nodes for remote cores

## Official References

- [Linux kernel documentation](https://docs.kernel.org/)
- [Driver API](https://docs.kernel.org/driver-api/index.html)
- [Core API](https://docs.kernel.org/core-api/index.html)
- [Development process](https://docs.kernel.org/process/index.html)
- [Development tools](https://docs.kernel.org/dev-tools/index.html)
- [Kernel hacking guides](https://docs.kernel.org/kernel-hacking/index.html)
- [Tracing](https://docs.kernel.org/trace/index.html)
- [Administration guide](https://docs.kernel.org/admin-guide/index.html)

## Related Topics

- [Device Tree](../device-tree/index.md)
- [Embedded Linux](../embedded-linux/index.md)
- [Debugging And Diagnostics](../debugging/index.md)
- [Build Systems](../build-systems/index.md)
- [Topic Map](../topic-map.md)

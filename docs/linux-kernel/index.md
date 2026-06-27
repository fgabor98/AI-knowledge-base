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

1. [Kernel Source, Build, And Tailoring](source-build-and-tailoring/index.md)
2. [Linux Device Driver Fundamentals](fundamentals/index.md)
3. [Common Driver Interfaces](driver-interfaces/index.md)
4. [Kernel Execution And Concurrency](execution-and-concurrency/index.md)
5. [Kernel Memory And I/O](memory-and-io/index.md)
6. [Kernel Configuration And Platform Policy](configuration-and-platform-policy/index.md)
7. [Kernel Debugging Basics](debugging/index.md)
8. [Power Management](power-management/index.md)
9. [Remoteproc, RPMsg, And Heterogeneous SoCs](remoteproc-rpmsg/index.md)
10. [Madieu Book Topic Coverage](book-coverage.md)

## Chapter Boundaries

This chapter is about how kernel code behaves at runtime: drivers, kernel APIs, hardware resources, execution contexts, debugging, and board-level integration.

The build system chapter owns Kconfig, Kbuild, defconfigs, cross-builds, module installation, kernel release artifacts, and reproducible kernel builds. This chapter references those topics only where a runtime concept depends on how the driver was built or configured.

## Suggested Study Order

Start with driver fundamentals, then learn the hardware resource APIs used by real embedded drivers. After that, study execution context, memory, and debugging before moving into power management and heterogeneous SoC topics.

```text
module basics
-> device model and probe/remove
-> device tree matching and resource lookup
-> GPIO/I2C/SPI/IRQ/regmap/clocks/resets/regulators/pinctrl
-> execution context, locking, memory, MMIO, DMA
-> debugging and power management
-> remoteproc, RPMsg, and multi-core SoC integration
```

## Roadmap Summary

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

## Related Topics

- [Device Tree](../device-tree/index.md)
- [Embedded Linux](../embedded-linux/index.md)
- [Debugging](../debugging/index.md)
- [Build Systems](../build-systems/index.md)
- [Topic Map](../topic-map.md)

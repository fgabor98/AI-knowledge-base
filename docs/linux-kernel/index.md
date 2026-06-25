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

## Roadmap

### Linux Device Driver Fundamentals

- kernel module lifecycle
- built-in drivers vs loadable modules
- `probe` and `remove`
- platform devices and platform drivers
- device tree matching
- `of_match_table`
- `compatible` strings from the driver side
- resource lookup
- `devm_*` managed allocation
- character device basics
- sysfs attributes
- module parameters
- driver logging with `dev_*`

### Common Driver Interfaces

- GPIO consumer API
- IRQ handling
- threaded interrupts
- I2C client drivers
- SPI device drivers
- UART/TTY integration overview
- CAN driver integration overview
- regmap
- clocks
- resets
- regulators
- pinctrl
- DMA basics

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

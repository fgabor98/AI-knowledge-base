---
status: active
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# Knowledge Guide Companion Checklist

This checklist tracks the knowledge-guide pages that accompany the
[Official Linux Kernel Documentation Reading Checklist](index.md). It uses the
same eight stages so the official material, local explanations, source study,
and labs can be completed together.

All 115 Linux Kernel knowledge-guide pages that existed when this checklist was
created are assigned exactly once. A page may relate to several stages, but it
has only one checkbox here so its completion state remains unambiguous.

## Synchronization Rule

For each stage:

```text
read one coherent group of official P0 documents
-> read the matching knowledge-guide pages below
-> inspect the named headers and in-tree drivers
-> complete the associated exercise or lab
-> check off both trackers
```

Mark a knowledge-guide page complete when you can explain its mental model,
recognize the relevant kernel APIs, and apply its debugging checklist. Reading
the words once is not enough.

## Stage 1: Development Process And Kernel Source

Official tracker: [Development Process And Kernel Source](01-development-process-and-source.md)

- [x] [Linux Kernel Programming Overview](../index.md)
- [x] [Kernel Foundations For Driver Developers](../foundations/index.md)
- [x] [Kernel Mental Model](../foundations/kernel-mental-model.md)
- [x] [Kernel C Survival Guide](../foundations/kernel-c-survival-guide.md)
- [x] [Reading Kernel Source](../foundations/reading-kernel-source.md)
- [x] [Kernel Development Lab Setup](../foundations/kernel-development-lab-setup.md)
- [x] [Driver Development Workflow](../foundations/driver-development-workflow.md)
- [x] [Small Lab Progression](../foundations/small-lab-progression.md)
- [x] [Kernel Documentation Reading Guide For Beginners](../foundations/kernel-documentation-reading-guide-for-beginners.md)

Stage completion:

- [ ] I can locate a driver's Kconfig, Makefile, registration, match, probe, runtime, and teardown paths.
- [ ] I have a recoverable lab and can prove which kernel, DTB, and module are running.

## Stage 2: Build System, Kconfig, And Development Tools

Official tracker: [Build System, Kconfig, And Development Tools](02-build-kconfig-and-devtools.md)

- [ ] [Kernel Source, Build, And Tailoring](../source-build-and-tailoring/index.md)
- [ ] [Kernel Source Acquisition](../source-build-and-tailoring/kernel-source-acquisition.md)
- [ ] [Kernel Configuration And Tailoring](../source-build-and-tailoring/kernel-configuration-and-tailoring.md)
- [ ] [Kernel Build And Install Overview](../source-build-and-tailoring/kernel-build-and-install-overview.md)
- [ ] [Kernel Module Lifecycle](../fundamentals/kernel-module-lifecycle.md)
- [ ] [Built-In Drivers Vs Loadable Modules](../fundamentals/built-in-vs-loadable-modules.md)

Stage completion:

- [ ] I can reproduce the source, configuration, toolchain, build directory, and output artifacts.
- [ ] I can build, deploy, identify, load, inspect, and unload a matching external module.

## Stage 3: Core APIs, Concurrency, Memory, And DMA

Official tracker: [Core APIs, Concurrency, Memory, And DMA](03-core-api-concurrency-memory.md)

### Execution Context And Concurrency

- [ ] [Execution Context Primer](../foundations/execution-context-primer.md)
- [ ] [Kernel Execution And Concurrency](../execution-and-concurrency/index.md)
- [ ] [Context Rules](../execution-and-concurrency/context-rules.md)
- [ ] [Sleepable Vs Atomic Code](../execution-and-concurrency/sleepable-vs-atomic-code.md)
- [ ] [Bottom Halves, Softirqs, And Tasklets](../execution-and-concurrency/bottom-halves-softirqs-and-tasklets.md)
- [ ] [Locking And Atomics](../execution-and-concurrency/locking-and-atomics.md)
- [ ] [Workqueues](../execution-and-concurrency/workqueues.md)
- [ ] [Concurrency Managed Workqueues](../execution-and-concurrency/concurrency-managed-workqueues.md)
- [ ] [Timers](../execution-and-concurrency/timers.md)
- [ ] [Hrtimers](../execution-and-concurrency/hrtimers.md)
- [ ] [Timekeeping And Kernel Timers](../execution-and-concurrency/timekeeping-and-kernel-timers.md)
- [ ] [Wait Queues And Completions](../execution-and-concurrency/wait-queues-and-completions.md)
- [ ] [Reference Counting And Lifetime](../execution-and-concurrency/reference-counting-and-lifetime.md)

### Memory And I/O

- [ ] [Kernel Memory And I/O](../memory-and-io/index.md)
- [ ] [Kernel Memory Allocation](../memory-and-io/kernel-memory-allocation.md)
- [ ] [Kernel Virtual Memory And VMAs](../memory-and-io/kernel-virtual-memory-and-vmas.md)
- [ ] [MMIO And Register Access](../memory-and-io/mmio-and-register-access.md)
- [ ] [Userspace Copy And ioctl ABI](../memory-and-io/userspace-copy-and-ioctl-abi.md)
- [ ] [DMA Mapping Basics](../memory-and-io/dma-mapping-basics.md)
- [ ] [Single-Buffer DMA](../memory-and-io/single-buffer-dma.md)
- [ ] [Scatter-Gather DMA](../memory-and-io/scatter-gather-dma.md)

Stage completion:

- [ ] I can classify each callback's context, sleepability, concurrency, and lifetime constraints.
- [ ] I can explain MMIO ordering and every CPU/device ownership transition for a DMA buffer.

## Stage 4: Driver Model, Device Tree, And Firmware

Official tracker: [Driver Model, Device Tree, And Firmware](04-driver-model-devicetree-firmware.md)

- [ ] [Device Model Primer](../foundations/device-model-primer.md)
- [ ] [Linux Device Driver Fundamentals](../fundamentals/index.md)
- [ ] [Device Tree Hardware Description](../fundamentals/device-tree-hardware-description.md)
- [ ] [Device Tree Overlays](../fundamentals/device-tree-overlays.md)
- [ ] [Driver Binding, Probe, And Remove](../fundamentals/driver-binding-probe-remove.md)
- [ ] [Platform Devices And Platform Drivers](../fundamentals/platform-devices-and-drivers.md)
- [ ] [Device Tree Matching From Drivers](../fundamentals/device-tree-matching.md)
- [ ] [Resource Lookup And Managed Allocation](../fundamentals/resource-lookup-and-devm.md)

Stage completion:

- [ ] I can trace a Device Tree node through device creation, matching, probe, and subsystem registration.
- [ ] I can explain provider dependencies, deferred probe, failure unwinding, and remove ordering.

## Stage 5: Embedded Driver Subsystems

Official tracker: [Embedded Driver Subsystems](05-embedded-driver-subsystems.md)

- [ ] [Common Driver Interfaces](../driver-interfaces/index.md)
- [ ] [GPIO Consumer API](../driver-interfaces/gpio-consumer-api.md)
- [ ] [GPIO Controller Drivers](../driver-interfaces/gpio-controller-drivers.md)
- [ ] [GPIO Expanders](../driver-interfaces/gpio-expanders.md)
- [ ] [Legacy GPIO Interfaces](../driver-interfaces/legacy-gpio-interfaces.md)
- [ ] [Interrupt Processing Model](../driver-interfaces/interrupt-processing-model.md)
- [ ] [IRQ Handling](../driver-interfaces/irq-handling.md)
- [ ] [Threaded Interrupts](../driver-interfaces/threaded-interrupts.md)
- [ ] [I2C Client Drivers](../driver-interfaces/i2c-client-drivers.md)
- [ ] [SPI Device Drivers](../driver-interfaces/spi-device-drivers.md)
- [ ] [UART And TTY Integration](../driver-interfaces/uart-tty-integration.md)
- [ ] [CAN Driver Integration](../driver-interfaces/can-driver-integration.md)
- [ ] [PWM Drivers](../driver-interfaces/pwm-drivers.md)
- [ ] [Regmap](../driver-interfaces/regmap.md)
- [ ] [Clocks](../driver-interfaces/clocks.md)
- [ ] [Resets](../driver-interfaces/resets.md)
- [ ] [Regulators](../driver-interfaces/regulators.md)
- [ ] [Pinctrl](../driver-interfaces/pinctrl.md)
- [ ] [DMA Basics](../driver-interfaces/dma-basics.md)
- [ ] [IIO Subsystem](../driver-interfaces/iio-subsystem.md)
- [ ] [IIO Channels And Sysfs](../driver-interfaces/iio-channels-and-sysfs.md)
- [ ] [IIO Triggers And Buffers](../driver-interfaces/iio-triggers-and-buffers.md)
- [ ] [Input Subsystem](../driver-interfaces/input-subsystem.md)
- [ ] [Polled Input Devices](../driver-interfaces/polled-input-devices.md)
- [ ] [IRQ-Based Input Devices](../driver-interfaces/irq-based-input-devices.md)
- [ ] [User-Space Hardware Access Vs Kernel Drivers](../fundamentals/userspace-hardware-access-vs-kernel-drivers.md)

Stage completion:

- [ ] I can choose the standard subsystem and ABI instead of inventing a private interface.
- [ ] For each project device, I can map consumers, providers, interrupts, power resources, and userspace visibility.

## Stage 6: Debugging, Tracing, And Testing

Official tracker: [Debugging, Tracing, And Testing](06-debugging-tracing-testing.md)

- [ ] [Debugging Ladder](../foundations/debugging-ladder.md)
- [ ] [Failure Taxonomy](../foundations/failure-taxonomy.md)
- [ ] [Kernel Debugging Basics](../debugging/index.md)
- [ ] [Dmesg And Log Levels](../debugging/dmesg-and-log-levels.md)
- [ ] [Dynamic Debug](../debugging/dynamic-debug.md)
- [ ] [Ftrace And Tracepoints](../debugging/ftrace-and-tracepoints.md)
- [ ] [Perf Overview](../debugging/perf-overview.md)
- [ ] [Debugfs And Sysfs Inspection](../debugging/debugfs-and-sysfs-inspection.md)
- [ ] [KGDB Basics](../debugging/kgdb-basics.md)
- [ ] [Oops, Panic, And Crash Logs](../debugging/oops-panic-crash-logs.md)
- [ ] [Watchdog Reset Diagnosis](../debugging/watchdog-reset-diagnosis.md)
- [ ] [Probe Failure Debugging](../debugging/probe-failure-debugging.md)

Stage completion:

- [ ] I classify the failure and preserve the first evidence before changing code.
- [ ] I can select logs, sysfs/debugfs, tracing, profiling, sanitizers, or a debugger based on a concrete question.

## Stage 7: Power Management And Heterogeneous SoCs

Official tracker: [Power Management And Heterogeneous SoCs](07-power-management-remoteproc.md)

### Power Management

- [ ] [Power Management](../power-management/index.md)
- [ ] [Runtime PM](../power-management/runtime-pm.md)
- [ ] [Suspend And Resume](../power-management/suspend-resume.md)
- [ ] [Wake Sources](../power-management/wake-sources.md)
- [ ] [CPU Idle And Frequency Scaling](../power-management/cpuidle-cpufreq.md)
- [ ] [Power Domains](../power-management/power-domains.md)
- [ ] [Regulator And Clock Power Dependencies](../power-management/regulator-clock-power-dependencies.md)
- [ ] [Suspend And Resume Debugging](../power-management/suspend-resume-debugging.md)

### Remoteproc, RPMsg, PRU, R5, And M4

- [ ] [Remoteproc, RPMsg, And Heterogeneous SoCs](../remoteproc-rpmsg/index.md)
- [ ] [Remoteproc Framework](../remoteproc-rpmsg/remoteproc-framework.md)
- [ ] [Firmware Loading](../remoteproc-rpmsg/firmware-loading.md)
- [ ] [Reserved Memory](../remoteproc-rpmsg/reserved-memory.md)
- [ ] [Virtio And RPMsg](../remoteproc-rpmsg/virtio-rpmsg.md)
- [ ] [PRU Integration](../remoteproc-rpmsg/pru-integration.md)
- [ ] [R5 And M4 Firmware Lifecycle](../remoteproc-rpmsg/r5-m4-firmware-lifecycle.md)
- [ ] [Remote Core Logs And Crashes](../remoteproc-rpmsg/remote-core-logs-and-crashes.md)
- [ ] [Device Tree Nodes For Remote Cores](../remoteproc-rpmsg/device-tree-nodes-for-remote-cores.md)

Stage completion:

- [ ] I can draw device power dependencies and explain runtime and system-sleep ordering.
- [ ] I can map remote-core firmware, carveouts, resource tables, vrings, endpoints, lifecycle, and recovery.

## Stage 8: Architecture, Userspace ABI, Administration, And Policy

Official tracker: [Architecture, Userspace ABI, Administration, And Optional Areas](08-architecture-userspace-optional.md)

### Userspace-Facing Driver Interfaces

- [ ] [Character Device Basics](../fundamentals/character-device-basics.md)
- [ ] [Device Classes, Uevents, And udev](../fundamentals/device-classes-uevents-and-udev.md)
- [ ] [Sysfs Attributes](../fundamentals/sysfs-attributes.md)
- [ ] [Kobjects And Sysfs Groups](../fundamentals/kobjects-and-sysfs-groups.md)
- [ ] [Pollable Sysfs Attributes](../fundamentals/pollable-sysfs-attributes.md)
- [ ] [Module Parameters And Driver Logging](../fundamentals/module-parameters-and-logging.md)

### Configuration And Product Policy

- [ ] [Kernel Configuration And Platform Policy](../configuration-and-platform-policy/index.md)
- [ ] [Debug Vs Production Configs](../configuration-and-platform-policy/debug-vs-production-configs.md)
- [ ] [Built-In Vs Module Policy](../configuration-and-platform-policy/built-in-vs-module-policy.md)
- [ ] [Kernel Command-Line Policy](../configuration-and-platform-policy/kernel-command-line-policy.md)
- [ ] [Watchdog Options](../configuration-and-platform-policy/watchdog-options.md)
- [ ] [Module Signing And Hardening](../configuration-and-platform-policy/module-signing-and-hardening.md)
- [ ] [Namespaces, Cgroups, And LSM](../configuration-and-platform-policy/namespaces-cgroups-lsm.md)
- [ ] [Initramfs Options](../configuration-and-platform-policy/initramfs-options.md)
- [ ] [Config Review Workflow](../configuration-and-platform-policy/config-review-workflow.md)

### Coverage Audit

- [ ] [Madieu Book Topic Coverage](../book-coverage.md)

Stage completion:

- [ ] I can identify every userspace ABI exposed by a driver and its stability obligations.
- [ ] I can justify the final production configuration, command line, module, signing, initramfs, security, and watchdog policies.

## Overall Completion

- [ ] All 115 knowledge-guide page checkboxes are complete.
- [ ] All eight official-documentation stage checkboxes are complete.
- [ ] I have recorded version-specific differences between upstream and the active vendor kernel.
- [ ] I have completed at least one end-to-end driver or board-integration lab.
- [ ] I have a list of topics that require another pass or a project-specific deep dive.

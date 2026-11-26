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

All 125 Linux Kernel knowledge-guide pages are assigned exactly once. A page may relate to several stages, but it
has only one checkbox here so its completion state remains unambiguous.

Reading of *The Linux Kernel Module Programming Guide* is tracked on this page as
well, in [LKMPG Study Progress](#lkmpg-study-progress).

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

Added to completed pages since they were ticked:

- [ ] [Branch Hints And Static Keys](../foundations/kernel-c-survival-guide.md#branch-hints-and-static-keys)
- [ ] [QEMU Kernel Lab](../foundations/kernel-development-lab-setup.md#option-2-qemu-kernel-lab): build the kernel, boot it, load Lab 1
- [ ] [Labs 10 to 19](../foundations/small-lab-progression.md#runnable-checkpoints): overview

Stage completion:

- [x] I can locate a driver's Kconfig, Makefile, registration, match, probe, runtime, and teardown paths.
- [x] I have a recoverable lab and can prove which kernel, DTB, and module are running.

## Stage 2: Build System, Kconfig, And Development Tools

Official tracker: [Build System, Kconfig, And Development Tools](02-build-kconfig-and-devtools.md)

- [x] [Kernel Source, Build, And Tailoring](../source-build-and-tailoring/index.md)
- [x] [Kernel Source Acquisition](../source-build-and-tailoring/kernel-source-acquisition.md)
- [x] [Kernel Configuration And Tailoring](../source-build-and-tailoring/kernel-configuration-and-tailoring.md)
- [x] [Kernel Build And Install Overview](../source-build-and-tailoring/kernel-build-and-install-overview.md)
- [x] [Kernel Module Lifecycle](../fundamentals/kernel-module-lifecycle.md)
- [x] [Built-In Drivers Vs Loadable Modules](../fundamentals/built-in-vs-loadable-modules.md)
- [ ] [Exported Symbols And Module Dependencies](../fundamentals/exported-symbols-and-module-dependencies.md)

Added to completed pages since they were ticked:

- [ ] [Register Last, Unregister First](../fundamentals/kernel-module-lifecycle.md#register-last-unregister-first)
- [ ] [Supporting Several Kernel Versions](../fundamentals/kernel-module-lifecycle.md#supporting-several-kernel-versions)

Labs:

- [ ] Lab 16: [Exported symbols](../foundations/small-lab-progression.md#lab-16-exported-symbols)

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
- [ ] [RCU](../execution-and-concurrency/rcu.md)
- [ ] [Kernel Threads](../execution-and-concurrency/kernel-threads.md)

### Memory And I/O

- [ ] [Kernel Memory And I/O](../memory-and-io/index.md)
- [ ] [Kernel Memory Allocation](../memory-and-io/kernel-memory-allocation.md)
- [ ] [Kernel Virtual Memory And VMAs](../memory-and-io/kernel-virtual-memory-and-vmas.md)
- [ ] [MMIO And Register Access](../memory-and-io/mmio-and-register-access.md)
- [ ] [Userspace Copy And ioctl ABI](../memory-and-io/userspace-copy-and-ioctl-abi.md)
- [ ] [DMA Mapping Basics](../memory-and-io/dma-mapping-basics.md)
- [ ] [Single-Buffer DMA](../memory-and-io/single-buffer-dma.md)
- [ ] [Scatter-Gather DMA](../memory-and-io/scatter-gather-dma.md)

### Labs

- [ ] Lab 10: [ioctl ABI](../foundations/small-lab-progression.md#lab-10-ioctl-abi)
- [ ] Lab 11: [Blocking I/O and poll](../foundations/small-lab-progression.md#lab-11-blocking-io-and-poll)
- [ ] Lab 13: [Deferred contexts](../foundations/small-lab-progression.md#lab-13-deferred-contexts)
- [ ] Lab 14: [RCU](../foundations/small-lab-progression.md#lab-14-rcu)

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
- [ ] [PCI And PCIe Device Drivers](../driver-interfaces/pci-drivers.md)
- [ ] [USB Device Drivers](../driver-interfaces/usb-drivers.md)
- [ ] [Network Device Drivers](../driver-interfaces/network-device-drivers.md)

Labs:

- [ ] Lab 15: [Virtual input device](../foundations/small-lab-progression.md#lab-15-virtual-input-device)
- [ ] Lab 18: [PCI, MSI, and DMA](../foundations/small-lab-progression.md#lab-18-pci-msi-and-dma)
- [ ] Lab 19: [Network device](../foundations/small-lab-progression.md#lab-19-network-device)

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
- [ ] [Implementing Debugfs Interfaces](../debugging/implementing-debugfs-interfaces.md)
- [ ] [Kprobes And Dynamic Probes](../debugging/kprobes-and-dynamic-probes.md)

Labs:

- [ ] Lab 12: [seq_file and debugfs](../foundations/small-lab-progression.md#lab-12-seq_file-and-debugfs)
- [ ] Lab 17: [kprobe events](../foundations/small-lab-progression.md#lab-17-kprobe-events)

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
- [ ] [Procfs And seq_file](../fundamentals/procfs-and-seq-file.md)

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
- [ ] [LKMPG Topic Coverage](../lkmpg-coverage.md)

Stage completion:

- [ ] I can identify every userspace ABI exposed by a driver and its stability obligations.
- [ ] I can justify the final production configuration, command line, module, signing, initramfs, security, and watchdog policies.

## LKMPG Study Progress

This list tracks the reading of
[The Linux Kernel Module Programming Guide](https://sysprog21.github.io/lkmpg/)
independently of the eight stages. It has 94 reading items and 19 items for
running the examples, based on the edition of 7 September 2026.
[LKMPG Topic Coverage](../lkmpg-coverage.md) names the matching page and lab for
each section, and the points where the guide is outdated.

Marks: `[ ]` open, `[->]` in progress, `[x]` done, `[-]` skipped on purpose.
Items tagged *skim* are covered better in this chapter or are outside its scope;
items tagged *optional* can be skipped. Tick an example only after you have loaded it and can explain
its output. Run the examples in a VM.

```text
Edition in use:
Current section:
Kernel used for the examples:
Open questions:
```

### Chapter 1: Getting Started

#### 1. Introduction

- [x] 1.1–1.2 Authorship and acknowledgements *(optional)*
- [x] 1.3 What is a kernel module?
- [x] 1.4 Kernel module package
- [x] 1.5 What modules are in my kernel?
- [x] 1.6 Is there a need to download and compile the kernel?
- [x] 1.7 Before we begin
- [x] 1.8 QEMU-based development environment

#### 2–3. Headers And Examples

- [ ] 2 Kernel headers
- [ ] 3 Examples

#### 4. Hello World

- [ ] 4.1 The simplest module
- [ ] 4.2 Hello and goodbye
- [ ] 4.3 The `__init` and `__exit` macros
- [ ] 4.4 Licensing and module documentation
- [ ] 4.5 Passing command-line arguments to a module
- [ ] 4.6 Modules spanning multiple files
- [ ] 4.7 Building modules for a precompiled kernel *(skim)*
- [ ] Examples run: `hello-1`, `hello-2`, `hello-3`, `hello-4`, `hello-5`, `hello-6`, `start`, `stop`

### Chapter 2: Core Kernel Interfaces

#### 5. Preliminaries

- [ ] 5.1 How modules begin and end
- [ ] 5.2 Functions available to modules
- [ ] 5.3 User space vs kernel space
- [ ] 5.4 Name space
- [ ] 5.5 Code space
- [ ] 5.6 Device drivers

#### 6. Character Device Drivers

- [ ] 6.1 The `file_operations` structure
- [ ] 6.2 The `file` structure
- [ ] 6.3 Registering a device
- [ ] 6.4 Unregistering a device
- [ ] 6.5 chardev.c
- [ ] 6.6 Writing modules for multiple kernel versions
- [ ] Examples run: `chardev`

#### 7. The /proc Filesystem

- [ ] 7 Introduction
- [ ] 7.1 The `proc_ops` structure
- [ ] 7.2 Read and write a /proc file
- [ ] 7.3 Manage /proc file metadata and access hooks
- [ ] 7.4 Manage /proc file with seq_file
- [ ] Examples run: `procfs1`, `procfs2`, `procfs3`, `procfs4`

#### 8–9. sysfs And debugfs

- [ ] 8 sysfs: interacting with your module
- [ ] 9 debugfs: flexible debugging interfaces
- [ ] Examples run: `hello-sysfs`, `hello-debugfs`, `hello-debugfs-file`

#### 10. Talking To Device Files

- [ ] 10 Introduction and ioctl
- [ ] 10.1 poll, select, and epoll
- [ ] 10.2 Asynchronous notification *(skim)*
- [ ] Examples run: `ioctl`, `chardev2`, `userspace_ioctl`

### Chapter 3: Execution Context And Control Flow

#### 11. System Calls

- [ ] 11 System calls *(skim)*

#### 12. Blocking Processes And Threads

- [ ] 12.1 Sleep
- [ ] 12.2 Completions
- [ ] Examples run: `sleep`, `cat_nonblock`, `completions`

#### 13. Synchronization

- [ ] 13.1 Mutex
- [ ] 13.2 Spinlocks
- [ ] 13.3 Read and write locks
- [ ] 13.4 Atomic operations
- [ ] 13.5 Read-copy-update (RCU)
- [ ] Examples run: `example_mutex`, `example_spinlock`, `example_rwlock`, `example_atomic`, the RCU example

#### 14. Replacing Print Macros

- [ ] 14.1 Replacement *(optional)*
- [ ] 14.2 Flashing keyboard LEDs *(optional)*
- [ ] Examples run: `print_string`, `kbleds`

### Chapter 4: Memory, Time, And Data Movement

#### 15–18. Time, Memory, User Access, MMIO

- [ ] 15 Timekeeping and deferred execution
- [ ] 16 Memory allocation and ownership
- [ ] 17 Copying data across the user-kernel boundary
- [ ] 18 Memory-mapped I/O and register access
- [ ] Examples run: `kmem_cache`

#### 19. DMA And mmap

- [ ] 19 Introduction
- [ ] 19.1 DMA addressing and ownership rules
- [ ] 19.2 Scatter-gather and DMA API patterns
- [ ] 19.3 DMA debugging and common mistakes
- [ ] Examples run: `dma`

#### 20. Modern Interface Choices

- [ ] 20 Modern interface choices for Linux 5.10 and later

### Chapter 5: Hardware Integration And Advanced Topics

#### 21. GPIO

- [ ] 21.1 GPIO *(skim)*
- [ ] 21.2 Control the LED's on/off state *(skim)*
- [ ] 21.3 DHT11 sensor *(optional)*
- [ ] Examples run: `led`, `dht11`

#### 22. Scheduling Tasks

- [ ] 22.1 Tasklets *(skim)*
- [ ] 22.2 Work queues
- [ ] Examples run: `example_tasklet`, `sched`

#### 23. Interrupt Handlers

- [ ] 23.1 Interrupt handlers *(skim)*
- [ ] 23.2 Detecting button presses *(skim)*
- [ ] 23.3 Bottom half
- [ ] 23.4 Threaded IRQ
- [ ] Examples run: `intrpt`, `bottomhalf`, `bh_threaded`

#### 24. Virtual Input Device Driver

- [ ] 24 Virtual input device driver
- [ ] Examples run: `vinput`, `vkbd`

#### 25. PCI Drivers

- [ ] 25 PCI drivers

#### 26. USB Drivers

- [ ] 26.1 USB device model basics
- [ ] 26.2 Endpoints, pipes, and URBs
- [ ] 26.3 USB and userspace interfaces
- [ ] 26.4 Hotplug, suspend, and failure modes

#### 27. Block Drivers

- [ ] 27.1 The modern block stack *(skim)*
- [ ] 27.2 BIOs, requests, and completion *(skim)*
- [ ] 27.3 Queue limits, flushes, and reality *(skim)*
- [ ] Examples run: `blkram`

#### 28. Network Drivers

- [ ] 28.1 `net_device` and `netdev_ops`
- [ ] 28.2 Socket buffers and receive paths
- [ ] 28.3 NAPI, interrupt moderation, and throughput
- [ ] 28.4 Offloads, link state, and userspace
- [ ] Examples run: `vnetloop`

#### 29. The Device Model

- [ ] 29 Standardizing the interfaces: the device model
- [ ] Examples run: `devicemodel`

#### 30. Device Tree

- [ ] 30.1–30.2 Introduction, Device Tree and kernel modules
- [ ] 30.3–30.4 Example module and source
- [ ] 30.5 Testing Device Tree modules
- [ ] 30.6 Common Device Tree functions
- [ ] Examples run: `devicetree`

#### 31. Optimizations

- [ ] 31.1 Likely and unlikely conditions
- [ ] 31.2 Static keys
- [ ] Examples run: `static_key`

#### 32. Common Pitfalls

- [ ] 32.1 Using standard libraries
- [ ] 32.2 Disabling interrupts
- [ ] 32.3 Kernel stack is small
- [ ] 32.4 No floating point in the kernel
- [ ] 32.5 Initialize buffers before copying to user space
- [ ] 32.6 Double-underscore functions are internal

#### 33–34. Secure Boot And Further Reading

- [ ] 33 Secure Boot signing guide (33.1–33.5)
- [ ] 34 Where to go from here?

### LKMPG Completion

- [ ] All reading items are done or deliberately skipped.
- [ ] All example items are done or deliberately skipped.
- [ ] I can explain every entry of [Read With Care](../lkmpg-coverage.md#read-with-care).
- [ ] I have rewritten one LKMPG character-device example as a platform driver with per-device state.

## Overall Completion

- [ ] All 125 knowledge-guide page checkboxes are complete.
- [ ] All topic-lab checkboxes (Labs 10 to 19) are complete.
- [ ] All eight official-documentation stage checkboxes are complete.
- [ ] The [LKMPG Study Progress](#lkmpg-study-progress) list is complete.
- [ ] I have recorded version-specific differences between upstream and the active vendor kernel.
- [ ] I have completed at least one end-to-end driver or board-integration lab.
- [ ] I have a list of topics that require another pass or a project-specific deep dive.

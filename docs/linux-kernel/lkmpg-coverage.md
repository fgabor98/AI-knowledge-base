---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# LKMPG Topic Coverage

This page maps *The Linux Kernel Module Programming Guide* (LKMPG) to this chapter and says how to use the two together. Reading progress is tracked in the [LKMPG Study Progress](official-docs-reading/knowledge-guide-companion.md#lkmpg-study-progress) list.

- Edition compared: 7 September 2026
- Online: <https://sysprog21.github.io/lkmpg/>
- Source and examples: <https://github.com/sysprog21/lkmpg>

LKMPG is strong on small complete programs for general kernel interfaces. This chapter is strong on the embedded driver stack: Device Tree, regmap, clocks, regulators, pinctrl, IIO, power management, and remoteproc, which LKMPG barely touches. Read LKMPG for its examples and this chapter for design rules and diagnosis.

## Coverage Map

| LKMPG section | Knowledge base | Lab | Advice |
| --- | --- | --- | --- |
| 1-3 Introduction, headers, examples | [Lab Setup](foundations/kernel-development-lab-setup.md) | QEMU lab | read |
| 4 Hello World | [Kernel Module Lifecycle](fundamentals/kernel-module-lifecycle.md), [Parameters](fundamentals/module-parameters-and-logging.md), [Kbuild objects](../build-systems/advanced/linux-kernel/kbuild-objects-and-directories.md) | 1, 2, 15 | read |
| 5 Preliminaries | [Kernel Mental Model](foundations/kernel-mental-model.md), [Register Last, Unregister First](fundamentals/kernel-module-lifecycle.md#register-last-unregister-first), [Exported Symbols](fundamentals/exported-symbols-and-module-dependencies.md) | 16 | read |
| 6 Character device drivers | [Character Device Basics](fundamentals/character-device-basics.md), [Several Kernel Versions](fundamentals/kernel-module-lifecycle.md#supporting-several-kernel-versions) | 3 | read |
| 7 /proc | [Procfs And seq_file](fundamentals/procfs-and-seq-file.md) | 12 | read |
| 8 sysfs | [Sysfs Attributes](fundamentals/sysfs-attributes.md), [Kobjects](fundamentals/kobjects-and-sysfs-groups.md) | - | read |
| 9 debugfs | [Implementing Debugfs Interfaces](debugging/implementing-debugfs-interfaces.md) | 12 | read |
| 10 ioctl, poll, async notification | [Userspace Copy And ioctl ABI](memory-and-io/userspace-copy-and-ioctl-abi.md), [Wait Queues](execution-and-concurrency/wait-queues-and-completions.md) | 10, 11 | read, see cautions |
| 11 System calls | [Kprobes And Dynamic Probes](debugging/kprobes-and-dynamic-probes.md), [Why Some Symbols Are Not Exported](fundamentals/exported-symbols-and-module-dependencies.md#why-some-symbols-are-not-exported) | 17 | skim |
| 12 Sleep, completions | [Wait Queues And Completions](execution-and-concurrency/wait-queues-and-completions.md), [Kernel Threads](execution-and-concurrency/kernel-threads.md) | 11, 13 | read |
| 13 Synchronization | [Locking And Atomics](execution-and-concurrency/locking-and-atomics.md), [RCU](execution-and-concurrency/rcu.md) | 14 | read |
| 14 Replacing print macros | - | - | optional |
| 15 Time and deferred execution | [Timers](execution-and-concurrency/timers.md), [Hrtimers](execution-and-concurrency/hrtimers.md), [Timekeeping](execution-and-concurrency/timekeeping-and-kernel-timers.md) | 13 | read |
| 16 Memory allocation | [Kernel Memory Allocation](memory-and-io/kernel-memory-allocation.md) | - | read |
| 17 User-kernel copies | [Why The Helpers Work](memory-and-io/userspace-copy-and-ioctl-abi.md#why-the-helpers-work) | 10 | read |
| 18 MMIO | [MMIO And Register Access](memory-and-io/mmio-and-register-access.md) | 18 | read |
| 19 DMA and mmap | [DMA Mapping Basics](memory-and-io/dma-mapping-basics.md), [Scatter-Gather DMA](memory-and-io/scatter-gather-dma.md), [VMAs](memory-and-io/kernel-virtual-memory-and-vmas.md) | 18 | read |
| 20 Modern interface choices | this chapter as a whole | - | read |
| 21 GPIO | [GPIO Consumer API](driver-interfaces/gpio-consumer-api.md), [Legacy GPIO](driver-interfaces/legacy-gpio-interfaces.md) | 6 | skim, prefer this chapter |
| 22 Tasklets, work queues | [Bottom Halves](execution-and-concurrency/bottom-halves-softirqs-and-tasklets.md), [Workqueues](execution-and-concurrency/workqueues.md) | 13 | read 22.2, skim 22.1 |
| 23 Interrupt handlers | [IRQ Handling](driver-interfaces/irq-handling.md), [Threaded Interrupts](driver-interfaces/threaded-interrupts.md), [BH Workqueues](execution-and-concurrency/bottom-halves-softirqs-and-tasklets.md#bh-workqueues) | 8 | skim 23.1-23.2 |
| 24 Virtual input | [Input Subsystem](driver-interfaces/input-subsystem.md), [Class Attributes](fundamentals/device-classes-uevents-and-udev.md#static-classes-and-class-attributes) | 15 | read |
| 25 PCI | [PCI And PCIe Device Drivers](driver-interfaces/pci-drivers.md) | 18 | read |
| 26 USB | [USB Device Drivers](driver-interfaces/usb-drivers.md) | - | read |
| 27 Block | - | - | skim |
| 28 Network | [Network Device Drivers](driver-interfaces/network-device-drivers.md) | 19 | read |
| 29 Device model | [Device Model Primer](foundations/device-model-primer.md), [Driver Binding](fundamentals/driver-binding-probe-remove.md), [Power Management](power-management/index.md) | 4 | read |
| 30 Device Tree | [Device Tree](../device-tree/index.md), [Device Tree Matching](fundamentals/device-tree-matching.md) | 5 | read |
| 31 Optimizations | [Branch Hints And Static Keys](foundations/kernel-c-survival-guide.md#branch-hints-and-static-keys) | - | read |
| 32 Pitfalls | [Kernel C Survival Guide](foundations/kernel-c-survival-guide.md) | - | read |
| 33 Secure Boot signing | [Signing Your Own Module](configuration-and-platform-policy/module-signing-and-hardening.md#signing-your-own-module-on-a-secure-boot-pc) | - | read |

## Deliberately Not Integrated

| LKMPG topic | Reason |
| --- | --- |
| System call table patching (11) | does not work on current kernels; kprobes cover the legitimate use |
| Writing to the caller's tty, flashing keyboard LEDs (14) | curiosities; use printk, dynamic debug, tracepoints |
| DHT11 bit banging (21.3) | an IIO driver exists in the kernel; timing in a driver belongs to hardware |
| Block drivers (27) | a specialist field; read LKMPG and the block documentation when a project needs it |

## Read With Care

Points where LKMPG is outdated or imprecise, checked against Linux 6.18 and 7.0:

| Section | LKMPG says | Better |
| --- | --- | --- |
| 4.5 | module parameters are good for port or I/O addresses | hardware addresses come from Device Tree, ACPI, or bus enumeration |
| 6.1 | `read` and `write` are thread-safe through the file-position lock | that lock applies to regular files and directories, not to character devices; protect your state |
| 7 | a module with an open `/proc` file can be removed without protection | `remove_proc_entry()` waits for running operations |
| 10 | a read ioctl sends data *to* the kernel | `_IOR` means userspace reads from the kernel |
| 12.1 | there is no way to wake only one waiter | exclusive waiters exist, for example `wait_event_interruptible_exclusive()` |
| 13.2 | a spinlock section may take up to a few milliseconds | keep it to microseconds |
| 21, 23 | integer GPIO numbers and `gpio_to_irq()` | GPIO descriptors; IRQs from firmware data |
| 22.2 | work is scheduled by CFS | the fair scheduler uses EEVDF since 6.6 |
| 23.1 | interrupts are "short" or "long" | historical; choose hard handler, threaded handler, or deferred work by context |
| many | global state and one device per module | per-device state from `probe()` |

## Related Topics

- [Madieu Book Topic Coverage](book-coverage.md)
- [Small Lab Progression](foundations/small-lab-progression.md)
- [Knowledge Guide Companion Checklist](official-docs-reading/knowledge-guide-companion.md)

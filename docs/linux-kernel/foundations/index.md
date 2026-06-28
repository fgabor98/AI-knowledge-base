---
status: draft
reviewed: false
domain: linux-kernel
difficulty: beginner
last_reviewed: null
---

# Kernel Foundations For Driver Developers

This track is the pre-intermediate runway for learning Linux kernel internals and driver development. It is for readers who know C and Linux basics, but are not yet comfortable reading kernel code, loading modules, interpreting probe failures, or debugging runtime kernel behavior.

The goal is practical: after this track, the learner should be able to enter the driver-specific pages without being blocked by the kernel's vocabulary, development loop, source layout, or debugging style.

## Learning Materials

1. [Kernel Mental Model](kernel-mental-model.md)
2. [Kernel C Survival Guide](kernel-c-survival-guide.md)
3. [Reading Kernel Source](reading-kernel-source.md)
4. [Kernel Development Lab Setup](kernel-development-lab-setup.md)
5. [Driver Development Workflow](driver-development-workflow.md)
6. [Debugging Ladder](debugging-ladder.md)
7. [Failure Taxonomy](failure-taxonomy.md)
8. [Execution Context Primer](execution-context-primer.md)
9. [Device Model Primer](device-model-primer.md)
10. [Small Lab Progression](small-lab-progression.md)
11. [Kernel Documentation Reading Guide For Beginners](kernel-documentation-reading-guide-for-beginners.md)

## Prerequisites

You do not need to know kernel internals before starting this track, but you should already be comfortable with:

- C structs, pointers, arrays, function pointers, and basic preprocessor macros.
- Basic Linux command-line work: `cd`, `ls`, `find`, `rg` or `grep`, `make`, `dmesg`, `journalctl`, and `sudo`.
- Basic Git usage: clone, checkout, diff, log, and status.
- Basic Linux runtime concepts: processes, files, permissions, device nodes, and services.
- A safe test environment: VM, QEMU target, spare board, or recoverable development board.

If C is still shaky, work through the C and build-system foundations before writing kernel code. Kernel mistakes are less forgiving than userspace mistakes.

## Mental Model

Kernel development becomes manageable when the learner separates four questions:

```text
what code is running?
-> what object owns the state?
-> what context is it running in?
-> what evidence can prove the failure mode?
```

Those questions prevent common beginner failure modes:

- Editing the right source but loading an old module.
- Calling a sleepable API from interrupt context.
- Debugging userspace permissions when the real problem is failed driver binding.
- Adding logs everywhere instead of proving whether the device exists, matched, probed, and acquired resources.
- Treating a driver like a userspace library rather than code owned by kernel object lifetimes.

## The Driver Developer's First Map

Most beginner driver work can be mapped like this:

```text
source tree
-> Kconfig and Kbuild selection
-> kernel image or .ko module
-> device description or bus discovery
-> device object
-> driver match
-> probe
-> subsystem registration
-> userspace-visible ABI
-> runtime callbacks
-> remove, shutdown, or suspend
```

Each arrow is a place where a failure can happen. The foundations track teaches how to inspect each boundary before the learner studies subsystem-specific APIs.

## Learning Path

### 1. Understand the Kernel Boundary

Start with [Kernel Mental Model](kernel-mental-model.md). The important distinction is not just "kernel space vs user space"; it is ownership. The kernel owns hardware resources, scheduling, memory protection, interrupt dispatch, power state, and the stable interfaces userspace relies on.

Example:

```text
cat /dev/input/event0
-> read system call
-> input subsystem file operations
-> input device event queue
-> input driver reports events
-> hardware interrupt or polling path detects state
```

The userspace command is small. The kernel path behind it has multiple objects and ownership boundaries.

### 2. Learn Kernel C Reading Patterns

Work through [Kernel C Survival Guide](kernel-c-survival-guide.md). You do not need to memorize every helper, but you must recognize:

- embedded structs and `container_of`
- callback tables
- `struct file_operations`, `struct platform_driver`, and similar operation tables
- `ERR_PTR`, `IS_ERR`, and `PTR_ERR`
- intrusive lists with `struct list_head`
- reverse-order cleanup with `goto`

These patterns appear everywhere in kernel code.

### 3. Learn to Read One Driver End to End

Use [Reading Kernel Source](reading-kernel-source.md) before writing much code. Pick a simple driver and follow:

```text
Kconfig symbol
-> Makefile object
-> match table
-> driver registration
-> probe
-> subsystem registration
-> runtime callbacks
-> cleanup path
```

Do this with one real in-tree driver before writing your own. Existing drivers are the strongest source of idiomatic examples.

### 4. Build a Safe Lab

Use [Kernel Development Lab Setup](kernel-development-lab-setup.md). The minimum safe setup is:

- recoverable machine or board
- matching kernel source and config
- serial/console log capture
- known-good boot path
- ability to rebuild and reload one module

Do not make early experiments on a device you cannot recover.

### 5. Use a Tight Development Loop

Use [Driver Development Workflow](driver-development-workflow.md). A productive loop is:

```text
edit one small behavior
-> build
-> deploy exact artifact
-> load or trigger probe
-> inspect dmesg and sysfs
-> run one test
-> unload or reboot
-> record result
```

Avoid changing source, Device Tree, kernel config, module install paths, and userspace test code in the same iteration.

### 6. Debug from Cheap Evidence to Expensive Evidence

Use [Debugging Ladder](debugging-ladder.md). Most beginner driver issues are solved before KGDB or crash dumps are needed:

```text
dmesg
-> sysfs
-> modinfo and lsmod
-> dynamic debug
-> debugfs
-> ftrace and tracepoints
-> perf
-> sanitizers, kgdb, crash dump
```

The ladder matters because heavy tools can perturb the system and slow learning.

### 7. Classify Failures Before Fixing

Use [Failure Taxonomy](failure-taxonomy.md). Ask where the failure occurs:

- build time
- module load time
- device creation time
- match/bind time
- probe/resource acquisition time
- runtime operation
- teardown
- suspend/resume
- crash/hang/reset

Classification turns a vague "driver does not work" into a targeted investigation.

### 8. Learn Execution Context Early

Use [Execution Context Primer](execution-context-primer.md). The beginner rule is:

> Before calling any kernel API, ask whether the current context may sleep.

Examples:

- `probe` usually runs in sleepable context.
- a hard IRQ handler must not sleep.
- a threaded IRQ handler may sleep.
- workqueue callbacks run in process context and may sleep.
- timer callbacks must not sleep.

This one habit prevents a large class of kernel bugs.

### 9. Learn the Device Model

Use [Device Model Primer](device-model-primer.md). Many driver problems are device-model problems:

- no device object exists
- the device exists but the driver did not register
- the match table is wrong
- `probe` ran but failed
- userspace did not create the expected node

Learn to inspect `/sys/bus`, `/sys/devices`, `/sys/class`, `modalias`, `driver`, `uevent`, and `driver_override`.

### 10. Progress Through Small Labs

Use [Small Lab Progression](small-lab-progression.md). The recommended order is:

```text
hello module
-> module parameters
-> character device
-> platform driver
-> Device Tree matched platform driver
-> GPIO consumer
-> I2C or SPI client
-> IRQ or threaded IRQ
-> tracing exercise
```

Each lab should add one new idea. Keep the previous lab working as a known-good checkpoint.

## Completion Criteria

By the end of this track, the learner should be able to:

- Explain kernel space vs user space and why drivers live in the kernel.
- Build, load, inspect, and unload a trivial module in a safe lab.
- Recognize `container_of`, embedded structs, callback tables, error pointers, intrusive lists, and cleanup labels.
- Follow a simple driver from registration to probe to userspace-visible state.
- Explain the difference between device, driver, bus, class, subsystem, and userspace ABI.
- Classify failures as build, load, device, match, probe, resource, runtime, teardown, crash, hang, or race problems.
- Use `dmesg`, `modinfo`, `lsmod`, sysfs, debugfs, dynamic debug, and basic ftrace as an ordered debugging ladder.
- Explain why execution context determines whether code may sleep.
- Know where to look in official kernel documentation and how to confirm details in source.

## Recommended Practice Environment

Use one of these:

- **Best for first kernel experiments:** QEMU or a VM snapshot.
- **Best for embedded bring-up:** a spare development board with serial console and recoverable boot media.
- **Best for real product debugging:** the actual target board plus a known-good recovery image and captured serial logs.

Avoid using your daily workstation kernel for first experiments unless you are only building and reading source.

## Related Topics

- [Kernel Source, Build, And Tailoring](../source-build-and-tailoring/index.md)
- [Linux Device Driver Fundamentals](../fundamentals/index.md)
- [Kernel Debugging Basics](../debugging/index.md)
- [Device Tree](../../device-tree/index.md)
- [Build Systems Linux Kernel Build System](../../build-systems/advanced/linux-kernel/index.md)

## References

- Linux kernel Driver API: <https://docs.kernel.org/driver-api/index.html>
- Linux kernel Core API: <https://docs.kernel.org/core-api/index.html>
- Linux kernel Development tools: <https://docs.kernel.org/dev-tools/index.html>
- Linux kernel hacking guides: <https://docs.kernel.org/kernel-hacking/index.html>
- Linux kernel development process: <https://docs.kernel.org/process/index.html>

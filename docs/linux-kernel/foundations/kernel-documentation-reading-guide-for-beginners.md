---
status: draft
reviewed: false
domain: linux-kernel
difficulty: beginner
last_reviewed: null
---

# Kernel Documentation Reading Guide For Beginners

## What Problem Does This Solve?

The official Linux kernel documentation is large and uneven. Some pages are tutorial-like, some are API references, some are maintainer process notes, and some assume the reader already knows the subsystem. Beginners need a curated way to use the docs without drowning.

This page gives a reading strategy for driver developers.

## Core Concepts

- process documentation
- driver API documentation
- core API documentation
- dev-tools documentation
- kernel hacking documentation
- subsystem documentation
- Device Tree bindings
- UAPI documentation
- ABI documentation
- source comments
- examples in existing drivers

## Reading Strategy

Use documentation in this order:

```text
1. local question
2. official docs for orientation
3. headers for exact types and prototypes
4. nearby in-tree drivers for usage patterns
5. source implementation for precise behavior
6. commit history if behavior changed
```

Do not use documentation as a substitute for reading the source. Use it to know where to read.

## Official Documentation Areas

### Process Documentation

Start here when you need to understand how kernel development works:

- coding style
- submitting patches
- stable API expectations
- maintainer workflow
- bug reporting
- development process

Useful for:

- writing code that looks like kernel code
- understanding why internal APIs change
- understanding why userspace ABI stability matters

Reference:

- <https://docs.kernel.org/process/index.html>

### Driver API

Start here when learning drivers and subsystems.

Topics include:

- driver basics
- driver model
- device links
- firmware loading
- DMA
- GPIO
- I2C
- SPI
- IIO
- input
- PWM
- power management
- regmap
- TTY/serial

Reference:

- <https://docs.kernel.org/driver-api/index.html>

How to use it:

```text
read the subsystem overview
-> identify key structs and callbacks
-> search headers for exact definitions
-> search drivers for examples
```

Example for IIO:

```bash
rg "struct iio_info" include drivers/iio
rg "devm_iio_device_alloc" drivers/iio
```

### Core API

Start here for cross-cutting kernel mechanisms.

Topics include:

- printk
- linked lists
- reference counting
- workqueues
- completions
- timers
- generic IRQ
- memory allocation
- DMA API
- kobjects
- debug objects
- tracepoints

Reference:

- <https://docs.kernel.org/core-api/index.html>

Example reading path for workqueues:

```text
Core API workqueue docs
-> include/linux/workqueue.h
-> kernel/workqueue.c only if deeper behavior is needed
-> drivers using INIT_WORK and cancel_work_sync
```

### Dev-Tools Documentation

Start here when learning debugging and validation tools.

Topics include:

- sparse
- smatch
- Coccinelle
- KASAN
- KMSAN
- UBSAN
- lockdep
- ftrace
- gcov
- kunit
- coccinelle

Reference:

- <https://docs.kernel.org/dev-tools/index.html>

Use it when:

- a bug smells like memory corruption
- a lock warning appears
- a static-analysis warning appears
- you want to add a small kernel test

### Static Analysis Tools

Static analysis is part of normal kernel development. These tools catch problems before runtime testing:

| Tool | Typical use |
|---|---|
| sparse | address-space annotations such as `__user` and `__iomem`, endian annotations, type issues |
| smatch | deeper static checks used by many kernel developers and CI setups |
| Coccinelle | semantic patching and tree-wide API migrations |

Examples:

```bash
make C=1 M=$PWD
make C=2 M=$PWD
```

`C=1` and `C=2` enable sparse checking through the kernel build system when sparse is installed.

Coccinelle semantic patches live under:

```text
scripts/coccinelle/
```

Example conceptual use:

```bash
make coccicheck MODE=report
```

Practical beginner rule:

- run sparse when working with `__user` and `__iomem`
- treat static-analysis warnings as design feedback, not just noise
- use Coccinelle mostly for learning existing API migration patterns until you are comfortable with semantic patches

### Kernel Hacking Guides

Use these after the first driver workflow is comfortable.

Topics include:

- locking
- kernel internals
- common pitfalls
- debugging approaches
- deeper development practices

Reference:

- <https://docs.kernel.org/kernel-hacking/index.html>

These are useful, but some pages assume more context than a beginner has. Read them alongside real code.

### Trace Documentation

Use for runtime ordering and timing questions.

References:

- <https://docs.kernel.org/trace/index.html>
- <https://docs.kernel.org/trace/ftrace.html>

Use when logs cannot answer:

- which callback ran first?
- how long did the handler take?
- did workqueue execution occur?
- did an IRQ fire?

### Admin Guide

Use for runtime knobs and system-level behavior.

Topics include:

- kernel parameters
- dynamic debug
- sysctl
- module parameters
- kernel taint
- bug hunting

References:

- <https://docs.kernel.org/admin-guide/index.html>
- <https://docs.kernel.org/admin-guide/dynamic-debug-howto.html>
- <https://docs.kernel.org/admin-guide/kernel-parameters.html>

## Device Tree Bindings

Device Tree bindings are part of the driver contract.

Location in source:

```text
Documentation/devicetree/bindings/
```

Search:

```bash
rg "example,demo" Documentation/devicetree/bindings
rg "gpio-controller" Documentation/devicetree/bindings
```

Use bindings to answer:

- which properties are required?
- which properties are optional?
- what are valid values?
- what child nodes are allowed?
- what compatible strings are valid?

Do not invent properties because they are convenient for one driver. Describe hardware, not driver policy.

## ABI Documentation

Kernel userspace interfaces should be documented under:

```text
Documentation/ABI/
```

Search examples:

```bash
rg "in_voltage" Documentation/ABI
rg "gpio" Documentation/ABI
rg "debugfs" Documentation/ABI
```

Use ABI docs when exposing or consuming:

- sysfs files
- procfs files
- debugfs diagnostics
- subsystem-specific userspace interfaces

Important distinction:

- `Documentation/ABI/stable/`: interfaces intended to remain stable
- `Documentation/ABI/testing/`: interfaces under development or less settled

## UAPI Headers

Headers under `include/uapi/` define interfaces visible to userspace. They are different from internal headers under `include/linux/`.

Examples:

```bash
rg "struct input_event" include/uapi
rg "_IOW|_IOR|_IOWR" include/uapi
```

Use UAPI headers when designing or reading:

- ioctl command numbers
- structs copied between kernel and userspace
- event formats
- netlink constants
- userspace-visible flags

Rules:

- use explicit UAPI integer types such as `__u32`, `__u64`, and `__s32`
- avoid exposing kernel pointers, `long`-dependent layouts, or internal structs
- preserve compatibility once userspace can depend on the interface
- document the ABI in the right place when adding new userspace-visible behavior

## Headers Are Documentation Too

Headers often contain the exact struct definitions and comments.

Examples:

```bash
rg "struct file_operations" include/linux
rg "struct platform_driver" include/linux
rg "struct i2c_driver" include/linux
rg "struct spi_driver" include/linux
rg "struct gpio_desc" include/linux
```

Read headers for:

- struct fields
- callback prototypes
- expected return values
- helper function comments
- annotations such as `__user` and `__iomem`

## Existing Drivers Are Usage Examples

After reading docs and headers, read a few nearby drivers.

Examples:

```bash
rg "devm_gpiod_get" drivers/input drivers/iio drivers/misc
rg "devm_request_threaded_irq" drivers/iio drivers/input
rg "devm_regmap_init_i2c" drivers
rg "module_i2c_driver" drivers/iio
```

Compare patterns:

- error handling
- `dev_err_probe` usage
- runtime PM usage
- locking
- cleanup
- subsystem registration

Do not copy blindly. Existing drivers may carry old patterns for compatibility.

## Version Awareness

Kernel APIs change. Always match documentation to the kernel version you are using.

Check version:

```bash
uname -r
git describe --tags
git branch --show-current
```

If using vendor BSP kernels:

- upstream docs may not match exactly
- APIs may be backported
- drivers may carry downstream changes
- Device Tree bindings may differ

Use the source tree's own `Documentation/` directory when possible.

## Reading Recipes

### Recipe: Learn A New Subsystem

```text
read subsystem overview in docs
-> list key structs
-> find headers
-> find three simple drivers
-> identify registration function
-> identify userspace ABI
-> write a tiny example or notes page
```

Example for input:

```bash
rg "input_register_device" drivers/input
rg "input_report_key" drivers/input
rg "struct input_dev" include/linux
```

### Recipe: Understand A Helper API

```text
read kernel-doc comment in header or source
-> search all call sites
-> compare success and failure handling
-> inspect return type
-> note context constraints
```

Example:

```bash
rg "devm_platform_ioremap_resource" .
rg "devm_platform_ioremap_resource" drivers | head -20
```

### Recipe: Diagnose A Runtime Interface

```text
find ABI doc
-> find subsystem implementation
-> find driver callback
-> trigger userspace operation
-> trace/log callback
```

Example:

```bash
rg "in_voltage0_raw" Documentation/ABI drivers/iio
rg "read_raw" drivers/iio
```

## Beginner Reading Order

Suggested order for this knowledge base:

1. [Kernel Mental Model](kernel-mental-model.md)
2. [Kernel C Survival Guide](kernel-c-survival-guide.md)
3. [Reading Kernel Source](reading-kernel-source.md)
4. Official Driver API overview
5. Official Core API pages for lists, printk, workqueues, IRQs, memory allocation, and kobjects
6. One existing simple driver in the target subsystem
7. Local subsystem page in this knowledge base

## What To Record In Notes

For each new API or subsystem, record:

- key structs
- registration function
- match mechanism
- callback table
- expected callback context
- userspace ABI
- common error codes
- cleanup requirements
- one simple in-tree example
- official documentation link

Example note:

```text
Subsystem: IIO
Key structs: iio_dev, iio_info, iio_chan_spec
Register: devm_iio_device_register
Userspace ABI: /sys/bus/iio/devices/iio:deviceX
Common callback: read_raw
Context: sleepable for typical reads, but check driver
Example drivers: drivers/iio/adc/...
Docs: docs.kernel.org/driver-api/iio/
```

## Common Mistakes

- Reading random web examples before official docs and in-tree drivers.
- Using docs for a different kernel version without noticing.
- Reading subsystem internals before understanding the public driver-facing API.
- Ignoring `Documentation/ABI` for sysfs behavior.
- Ignoring Device Tree bindings and copying properties from board files blindly.
- Treating one old driver as authoritative for modern style.
- Forgetting that vendor kernels may differ from upstream.

## Debugging Checklist

- Am I reading docs from the same kernel version?
- Did I read the relevant header?
- Did I inspect at least one in-tree user of the API?
- Did I check the userspace ABI documentation?
- Did I distinguish UAPI from internal kernel API?
- Did I check Device Tree bindings if firmware data is involved?
- Did I note callback context and cleanup requirements?
- Did I run the relevant static-analysis tool for the annotations I touched?

## Related Topics

- [Reading Kernel Source](reading-kernel-source.md)
- [Kernel Source Acquisition](../source-build-and-tailoring/kernel-source-acquisition.md)
- [Kernel Debugging Basics](../debugging/index.md)
- [Device Tree](../../device-tree/index.md)

## Official References

- Linux kernel Driver API: <https://docs.kernel.org/driver-api/index.html>
- Linux kernel Core API: <https://docs.kernel.org/core-api/index.html>
- Linux kernel Development tools: <https://docs.kernel.org/dev-tools/index.html>
- Linux kernel hacking guides: <https://docs.kernel.org/kernel-hacking/index.html>
- Linux kernel process documentation: <https://docs.kernel.org/process/index.html>
- Linux kernel trace documentation: <https://docs.kernel.org/trace/index.html>
- Dynamic debug HOWTO: <https://docs.kernel.org/admin-guide/dynamic-debug-howto.html>
- Sparse: <https://docs.kernel.org/dev-tools/sparse.html>
- Coccinelle: <https://docs.kernel.org/dev-tools/coccinelle.html>

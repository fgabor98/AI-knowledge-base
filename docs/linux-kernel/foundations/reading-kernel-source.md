---
status: draft
reviewed: false
domain: linux-kernel
difficulty: beginner
last_reviewed: null
---

# Reading Kernel Source

## What Problem Does This Solve?

The Linux kernel source tree is too large to read from top to bottom. Driver developers need a method for entering the tree through one concrete question, following ownership and callbacks, and stopping before the search becomes unbounded.

This page gives a practical workflow for reading kernel source like a driver developer.

## Core Concepts

- source tree layout
- Kconfig
- Kbuild and Makefiles
- `drivers/`
- `include/linux/`
- `Documentation/`
- `git grep`
- `rg`
- call chains
- callback tables
- struct-first reading
- registration paths
- subsystem examples
- log-message search

## Source Tree Layout For Driver Readers

Important top-level directories:

| Path | Why it matters |
|---|---|
| `drivers/` | most hardware drivers and subsystem-specific driver code |
| `include/linux/` | public in-kernel headers used across subsystems |
| `include/uapi/` | headers exposed to userspace ABI |
| `Documentation/` | rendered at `docs.kernel.org`; includes driver, core API, and subsystem docs |
| `arch/` | architecture-specific code, including boot, IRQ, MMU, and platform pieces |
| `kernel/` | scheduler, workqueues, timekeeping, printk, module core, and other central code |
| `mm/` | memory management |
| `fs/` | VFS and filesystems |
| `net/` | networking stack |
| `scripts/` | build, config, and documentation tools |
| `tools/` | userspace tooling such as `perf` and testing helpers |

For driver development, most reading starts in:

```text
drivers/<subsystem>/
include/linux/<subsystem>.h
Documentation/driver-api/
Documentation/devicetree/bindings/
```

## Choose A Concrete Entry Point

Do not start with "understand GPIO" or "understand the driver model." Start with one of these:

- a source file
- a kernel log line
- a Kconfig symbol
- a Device Tree `compatible`
- a function name
- a struct name
- a sysfs path
- a module name

Examples:

```bash
rg "compatible = \"gpio-leds\"" arch/arm64/boot/dts drivers Documentation
rg "dev_err.*probe" drivers/iio
rg "struct platform_driver" drivers/gpio
rg "MODULE_DEVICE_TABLE\\(of" drivers/input
```

Each search produces a bounded path into the code.

## Follow A Driver End To End

Use this reading order for a normal platform driver:

```text
Kconfig
-> Makefile
-> match table
-> driver registration
-> probe
-> resource acquisition
-> subsystem registration
-> runtime callbacks
-> remove/shutdown/suspend
```

### 1. Find The Kconfig Symbol

Example:

```bash
rg "config .*DEMO" drivers
rg "tristate.*demo" drivers
```

What to learn:

- can this driver be built as a module?
- what dependencies does it need?
- what subsystem menu owns it?
- what help text explains it?

Kconfig example shape:

```text
config SENSORS_DEMO
        tristate "Demo sensor"
        depends on I2C
        help
          Say Y or M here if your board has the demo sensor.
```

Interpretation:

- `tristate` means built-in, module, or disabled may be possible.
- `depends on I2C` means the symbol cannot be selected unless I2C support is present.

### 2. Find The Makefile Entry

Example:

```bash
rg "SENSORS_DEMO" drivers
```

Makefile shape:

```make
obj-$(CONFIG_SENSORS_DEMO) += demo-sensor.o
```

Interpretation:

- source file is built only if the Kconfig symbol is selected
- `=y` builds into the kernel
- `=m` builds a `.ko`

### 3. Find The Match Table

Device Tree match example:

```c
static const struct of_device_id demo_of_match[] = {
        { .compatible = "example,demo-sensor" },
        { }
};
MODULE_DEVICE_TABLE(of, demo_of_match);
```

Search:

```bash
rg "example,demo-sensor" .
rg "demo_of_match" drivers
```

What to learn:

- exact `compatible` string
- whether module autoload metadata exists
- whether match data selects hardware variants

### 4. Find Driver Registration

Examples:

```c
module_platform_driver(demo_driver);
module_i2c_driver(demo_i2c_driver);
module_spi_driver(demo_spi_driver);
```

Search:

```bash
rg "module_.*driver\\(" drivers/iio
```

What to learn:

- bus type
- lifecycle callbacks
- driver name
- match table

### 5. Read `probe`

Read `probe` slowly. It usually tells the whole driver story.

Common probe sequence:

```text
allocate private state
-> parse firmware data
-> acquire regulators, clocks, resets, GPIOs, IRQs, MMIO
-> initialize locks and state
-> initialize hardware
-> register with subsystem
-> enable runtime PM or interrupts
```

Example:

```c
static int demo_probe(struct i2c_client *client)
{
        struct demo_data *data;
        int ret;

        data = devm_kzalloc(&client->dev, sizeof(*data), GFP_KERNEL);
        if (!data)
                return -ENOMEM;

        data->regmap = devm_regmap_init_i2c(client, &demo_regmap_config);
        if (IS_ERR(data->regmap))
                return dev_err_probe(&client->dev, PTR_ERR(data->regmap),
                                     "failed to init regmap\n");

        ret = demo_check_id(data);
        if (ret)
                return ret;

        i2c_set_clientdata(client, data);
        return demo_register_with_subsystem(data);
}
```

Questions to ask:

- What state is allocated?
- What is device-managed?
- Which providers can defer probe?
- What subsystem receives the device?
- What work must be undone on remove?

### 6. Find Runtime Callbacks

Runtime behavior usually lives in callback tables:

```c
static const struct file_operations demo_fops = { ... };
static const struct iio_info demo_info = { ... };
static const struct input_dev_ops demo_ops = { ... };
static const struct dev_pm_ops demo_pm_ops = { ... };
```

Search by struct name:

```bash
rg "struct .*_ops" drivers/path/to/driver.c
rg "\\.read_raw|\\.write_raw|\\.open|\\.read|\\.suspend|\\.resume" drivers/path/to/driver.c
```

What to learn:

- which kernel subsystem calls each callback
- whether callbacks run in sleepable context
- what locks protect state
- what userspace action triggers the callback

## Search From A Log Message

If runtime logs show this:

```text
demo 1-0048: failed to read chip id: -121
```

Search exact text without variable parts:

```bash
rg "failed to read chip id" drivers
```

Then inspect surrounding code:

```bash
rg -n -C 5 "failed to read chip id" drivers
```

What to learn:

- which function failed
- which return code was preserved
- which resource or hardware transaction was active
- whether the message uses `dev_err_probe`, `dev_err`, or `dev_dbg`

## Search From A Sysfs Path

Suppose you see:

```text
/sys/bus/iio/devices/iio:device0/in_voltage0_raw
```

Reading path:

```text
IIO device
-> channel attribute
-> read_raw callback
-> driver register read
```

Search:

```bash
rg "read_raw" drivers/iio
rg "in_voltage" Documentation/ABI Documentation/driver-api drivers/iio
```

The file may be generated by subsystem code from channel definitions rather than created directly by the driver.

## Struct-First Reading

Before reading functions, read the main private struct.

Example:

```c
struct demo_data {
        struct device *dev;
        struct regmap *regmap;
        struct mutex lock;
        int irq;
        bool enabled;
};
```

This tells you:

- driver uses regmap
- driver has one lock
- driver has an IRQ
- driver tracks enabled state
- driver stores a `struct device` for logging and managed APIs

Then inspect where fields are written:

```bash
rg "enabled" drivers/path/to/demo.c
rg "mutex_lock\\(&data->lock" drivers/path/to/demo.c
```

## Compare With A Nearby Driver

The best examples are often in the same subsystem.

Example:

```bash
ls drivers/iio/adc
rg "devm_iio_device_alloc" drivers/iio/adc
```

Compare:

- allocation pattern
- match table
- resource lookup
- locking
- error handling
- subsystem registration
- runtime PM

Avoid copying blindly. Use nearby drivers to identify patterns, then confirm with subsystem documentation.

## Reading Kconfig And Build Failures

If a symbol does not appear in `.config`, inspect dependencies:

```bash
rg "config MY_DRIVER" -n drivers
scripts/config --state MY_DRIVER
```

Common causes:

- parent menu not enabled
- dependency missing
- architecture dependency
- symbol name changed
- fragment requested `=m`, but dependency is built-in-only or absent

## Reading Device Tree Bindings

For Device Tree matched drivers, read the binding before inventing properties.

Search:

```bash
rg "example,demo" Documentation/devicetree/bindings
rg "compatible:.*demo" Documentation/devicetree/bindings
```

Binding tells you:

- required properties
- optional properties
- valid child nodes
- examples
- whether a property is board-specific or driver policy

## Minimal Reading Exercise

Pick an in-tree platform driver and answer:

1. What Kconfig symbol selects it?
2. Which object file is built?
3. Can it be a module?
4. Which bus does it register with?
5. Which match tables exist: Device Tree, ACPI, ID table?
6. What does `probe` allocate?
7. What resources does `probe` request?
8. What subsystem does it register with?
9. Which callbacks can userspace trigger?
10. What stops during `remove`?

If you can answer those, you can read most simple drivers.

## Common Mistakes

- Starting from helper functions instead of the driver lifecycle.
- Reading only `.c` files and ignoring Kconfig, Makefile, bindings, and docs.
- Assuming sysfs files are always created directly by the driver.
- Searching for a module name with hyphens when kernel module names in some contexts use underscores.
- Following generic framework internals too early.
- Ignoring whether the code being read matches the target kernel version.

## Debugging Checklist

- What exact kernel version or vendor tree am I reading?
- What concrete question am I trying to answer?
- What is my entry point: symbol, log, path, compatible string, or callback?
- What owns the object I am reading?
- What registers this callback?
- What userspace or hardware event triggers this path?
- Which locks or context constraints apply?
- Is there a simpler nearby driver with the same subsystem pattern?

## Related Topics

- [Kernel Source Acquisition](../source-build-and-tailoring/kernel-source-acquisition.md)
- [Platform Devices And Platform Drivers](../fundamentals/platform-devices-and-drivers.md)
- [Kernel Documentation Reading Guide For Beginners](kernel-documentation-reading-guide-for-beginners.md)
- [Device Model Primer](device-model-primer.md)

## Official References

- Linux kernel Driver API: <https://docs.kernel.org/driver-api/index.html>
- Linux kernel Core API: <https://docs.kernel.org/core-api/index.html>
- Linux kernel process documentation: <https://docs.kernel.org/process/index.html>
- Linux kernel source browser entry point: <https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git>

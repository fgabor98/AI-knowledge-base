---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# Module Parameters And Driver Logging

## What Problem Does This Solve?

Module parameters and kernel logs provide controlled configuration and observability during driver development and deployment.

They answer different questions:

```text
module parameter
  -> small policy or diagnostic knob selected by operator/build/boot flow

kernel log
  -> timestamped evidence of what the driver did and why it failed
```

Use both sparingly. A driver that requires many module parameters is often missing proper firmware data, Device Tree properties, subsystem policy, or userspace configuration. A driver that logs too much becomes hard to debug and can damage timing-sensitive paths.

## Core Concepts

- `module_param()`
- `module_param_named()`
- `module_param_array()`
- `MODULE_PARM_DESC()`
- parameter type
- parameter permissions
- boot-time parameters
- `/sys/module/<module>/parameters/`
- `pr_*()`
- `dev_*()`
- log levels
- rate-limited logging
- `dev_dbg()`
- dynamic debug
- `printk()` format rules
- device-scoped logging

## Mental Model

Use parameters for small, explicit knobs. Use Device Tree or other firmware data for board wiring. Use `dev_*()` once a driver has a `struct device *`, because device-scoped logs preserve which device instance produced the message.

```text
before device exists:
  pr_info(), pr_err()

inside probe/remove/runtime callbacks:
  dev_info(), dev_err(), dev_dbg()

high-frequency path:
  rate-limited logs, tracepoints, counters, or no log
```

## Module Parameter Basics

Example:

```c
static int poll_interval_ms = 100;
module_param(poll_interval_ms, int, 0644);
MODULE_PARM_DESC(poll_interval_ms, "Polling interval in milliseconds");
```

Load:

```sh
sudo insmod demo.ko poll_interval_ms=250
```

Inspect:

```sh
cat /sys/module/demo/parameters/poll_interval_ms
```

Change at runtime if writable:

```sh
echo 500 | sudo tee /sys/module/demo/parameters/poll_interval_ms
```

The permission bits control sysfs exposure:

| Permission | Meaning |
| --- | --- |
| `0` | Not exposed in sysfs. |
| `0444` | Read-only. |
| `0644` | Root-writable, readable. |
| `0600` | Root-only read/write. |

Do not make parameters writable unless runtime changes are safe.

## Parameter Types

Common types:

```c
static bool debug;
static int threshold = 10;
static unsigned int timeout_ms = 1000;
static char *mode = "normal";

module_param(debug, bool, 0644);
module_param(threshold, int, 0644);
module_param(timeout_ms, uint, 0644);
module_param(mode, charp, 0444);
```

Named parameter:

```c
static int retries = 3;
module_param_named(max_retries, retries, int, 0644);
MODULE_PARM_DESC(max_retries, "Maximum transfer retry count");
```

Array parameter:

```c
static int channels[8];
static int num_channels;

module_param_array(channels, int, &num_channels, 0444);
MODULE_PARM_DESC(channels, "Channel list");
```

Load:

```sh
sudo insmod demo.ko channels=1,3,5
```

## Validate Parameter Values

Basic `module_param()` assignment does not automatically enforce your semantic range. Validate before use.

Example:

```c
static int timeout_ms = 1000;
module_param(timeout_ms, int, 0644);

static int demo_validate_params(void)
{
    if (timeout_ms < 10 || timeout_ms > 60000) {
        pr_err("demo: timeout_ms must be between 10 and 60000\n");
        return -EINVAL;
    }

    return 0;
}
```

Call validation from init or probe before the value controls hardware behavior.

For writable parameters, consider whether changing the value while the device is active requires locking:

```c
mutex_lock(&priv->lock);
priv->timeout_ms = timeout_ms;
mutex_unlock(&priv->lock);
```

If changing the value safely is complex, make the parameter read-only.

## Parameters For Built-In Drivers

Module parameters can also apply to built-in code. They are usually set on the kernel command line using:

```text
module_name.parameter=value
```

Example:

```text
demo.poll_interval_ms=250
```

Check runtime values:

```sh
ls /sys/module/demo/parameters
cat /sys/module/demo/parameters/poll_interval_ms
```

This depends on the parameter permissions and driver configuration.

For product builds, document parameters in boot policy. Hidden tribal knowledge in bootargs makes driver behavior hard to reproduce.

## What Should Not Be A Module Parameter?

Avoid module parameters for:

- MMIO base addresses
- IRQ numbers
- GPIO numbers
- clock names
- regulator names
- board variant wiring
- device `compatible` identity
- permanent product ABI settings

Those usually belong in Device Tree, ACPI, platform data, subsystem configuration, or userspace policy.

Acceptable uses:

- temporary debug flags
- conservative timeout override
- feature workaround for a known broken device revision
- lab-only fault injection
- compatibility mode while migrating old systems

Even then, document them.

## Logging APIs

Generic logging:

```c
pr_info("demo: loaded\n");
pr_err("demo: failed to allocate buffer\n");
```

Device-scoped logging:

```c
dev_info(dev, "probed\n");
dev_err(dev, "failed to request irq: %d\n", ret);
dev_dbg(dev, "status register: %#x\n", status);
```

Prefer `dev_*()` once you have `struct device *`. It prefixes logs with device identity, which matters when the same driver binds to multiple devices.

Example probe:

```c
static int demo_probe(struct platform_device *pdev)
{
    struct device *dev = &pdev->dev;
    int irq;

    irq = platform_get_irq(pdev, 0);
    if (irq < 0)
        return dev_err_probe(dev, irq, "failed to get irq\n");

    dev_info(dev, "using irq %d\n", irq);
    return 0;
}
```

`dev_err_probe()` is useful in probe paths because it handles `-EPROBE_DEFER` more cleanly and attaches the device identity to the message.

## Log Levels

Common levels:

| API | Use |
| --- | --- |
| `pr_emerg()` / `dev_emerg()` | System unusable. Rare in drivers. |
| `pr_alert()` / `dev_alert()` | Immediate action needed. Rare. |
| `pr_crit()` / `dev_crit()` | Critical condition. |
| `pr_err()` / `dev_err()` | Operation failed. |
| `pr_warn()` / `dev_warn()` | Suspicious but not fatal. |
| `pr_notice()` / `dev_notice()` | Notable normal condition. |
| `pr_info()` / `dev_info()` | Informational. Use sparingly. |
| `pr_debug()` / `dev_dbg()` | Debug messages, normally compiled/controlled by debug config. |

Do not log every successful operation in a hot path. Logs are shared system resources.

## Rate-Limited Logging

Use rate-limited logs when an error can repeat:

```c
dev_warn_ratelimited(dev, "device not ready\n");
```

Generic:

```c
pr_err_ratelimited("demo: repeated transfer failure\n");
```

Useful for:

- interrupt storms
- transient hardware errors
- retry loops
- timeout polling
- invalid userspace requests that could spam logs

Do not hide a first critical failure only behind a rate limit if it is important for diagnosis.

## One-Time Logging

Use once-only logs for warnings that should not repeat:

```c
dev_warn_once(dev, "legacy mode is deprecated\n");
```

This avoids log floods while still making the condition visible.

## Dynamic Debug

`dev_dbg()` and `pr_debug()` can be controlled by dynamic debug when the kernel is configured with support.

Check:

```sh
mount | grep debugfs
ls /sys/kernel/debug/dynamic_debug/control
```

Enable debug for a driver source file:

```sh
echo 'file drivers/misc/demo.c +p' | sudo tee /sys/kernel/debug/dynamic_debug/control
```

Enable by module:

```sh
echo 'module demo +p' | sudo tee /sys/kernel/debug/dynamic_debug/control
```

Disable:

```sh
echo 'module demo -p' | sudo tee /sys/kernel/debug/dynamic_debug/control
```

Dynamic debug is better than adding temporary `dev_info()` everywhere because it lets you keep debug statements in the code without making normal logs noisy.

## Print Formatting Rules

Kernel formatting is not identical to userspace `printf()`. Use kernel-supported specifiers.

Examples:

```c
dev_info(dev, "irq=%d size=%zu\n", irq, size);
dev_info(dev, "phys=%pa\n", &res->start);
dev_info(dev, "dma=%pad\n", &dma_handle);
dev_info(dev, "ptr=%p\n", ptr);
```

Avoid printing raw kernel pointers casually. Pointer hashing and restrictions exist for security reasons. Use symbolic identifiers, device names, resource ranges, and error codes when possible.

## Logging Errors Correctly

Bad:

```c
ret = platform_get_irq(pdev, 0);
if (ret < 0) {
    dev_err(dev, "failed\n");
    return ret;
}
```

Better:

```c
irq = platform_get_irq(pdev, 0);
if (irq < 0)
    return dev_err_probe(dev, irq, "failed to get irq\n");
```

Good logs tell you:

- which device
- which operation
- which resource
- which error code
- whether retry/defer is expected

Example:

```c
ret = clk_prepare_enable(priv->clk);
if (ret)
    return dev_err_probe(dev, ret, "failed to enable core clock\n");
```

## Logging In Hot Paths

Be careful in:

- IRQ handlers
- high-frequency timers
- fast read/write paths
- polling loops
- DMA completion paths
- error retries

Prefer:

- counters exposed through debugfs or sysfs where appropriate
- tracepoints
- `dev_dbg()` with dynamic debug
- rate-limited logs
- logging only state transitions

Bad:

```c
static irqreturn_t demo_irq(int irq, void *data)
{
    dev_info(dev, "irq fired\n");
    return IRQ_HANDLED;
}
```

Better:

```c
static irqreturn_t demo_irq(int irq, void *data)
{
    struct demo_priv *priv = data;

    priv->irq_count++;
    return IRQ_WAKE_THREAD;
}
```

Use explicit debugging only when needed:

```c
dev_dbg_ratelimited(priv->dev, "irq count=%u\n", priv->irq_count);
```

## Complete Parameter And Logging Example

```c
#include <linux/module.h>
#include <linux/platform_device.h>

static unsigned int timeout_ms = 1000;
module_param(timeout_ms, uint, 0644);
MODULE_PARM_DESC(timeout_ms, "Operation timeout in milliseconds");

static bool verbose;
module_param(verbose, bool, 0644);
MODULE_PARM_DESC(verbose, "Enable extra informational logs");

static int demo_probe(struct platform_device *pdev)
{
    struct device *dev = &pdev->dev;

    if (timeout_ms < 10 || timeout_ms > 60000)
        return dev_err_probe(dev, -EINVAL,
                             "timeout_ms must be between 10 and 60000\n");

    if (verbose)
        dev_info(dev, "verbose logging enabled\n");

    dev_dbg(dev, "timeout_ms=%u\n", timeout_ms);
    return 0;
}
```

Load:

```sh
sudo insmod demo.ko timeout_ms=250 verbose=1
```

Inspect:

```sh
cat /sys/module/demo/parameters/timeout_ms
dmesg | tail -n 80
```

## Common Failure Modes

| Symptom | Likely Cause | First Checks |
| --- | --- | --- |
| Parameter not accepted by `insmod` | Name or type mismatch | `modinfo -p demo.ko` |
| Parameter not visible in sysfs | Permission set to `0` or built/config issue | `/sys/module/<name>/parameters` |
| Built-in parameter has no effect | Wrong bootarg prefix | `cat /proc/cmdline`, docs |
| Logs do not appear | log level, dynamic debug, config | `dmesg -n`, dynamic debug control |
| Logs too noisy | logging hot path or retry loop | rate limit, `dev_dbg`, tracepoints |
| Multiple devices indistinguishable | using `pr_*` instead of `dev_*` | convert to device-scoped logs |
| Probe deferral looks like hard failure | plain `dev_err()` on `-EPROBE_DEFER` | use `dev_err_probe()` |

## Practice Exercises

### Exercise 1: Add A Validated Parameter

Add:

```c
static unsigned int sample_period_ms = 100;
module_param(sample_period_ms, uint, 0644);
```

Validate:

```c
if (sample_period_ms < 20 || sample_period_ms > 10000)
    return -EINVAL;
```

Test:

```sh
sudo insmod demo.ko sample_period_ms=1
sudo insmod demo.ko sample_period_ms=250
```

### Exercise 2: Convert Logs To Device-Scoped Logs

Replace:

```c
pr_info("probe complete\n");
```

with:

```c
dev_info(&pdev->dev, "probe complete\n");
```

Load two instances and compare log clarity.

### Exercise 3: Enable Dynamic Debug

Add:

```c
dev_dbg(dev, "register value=%#x\n", val);
```

Enable at runtime:

```sh
echo 'module demo +p' | sudo tee /sys/kernel/debug/dynamic_debug/control
```

Confirm debug logs appear only when enabled.

## Debugging Checklist

- Does `modinfo -p <module>.ko` show the expected parameters?
- Are parameter permissions intentional?
- Are writable parameters protected by locking if runtime changes matter?
- Does a built-in parameter use the correct `module.parameter=value` bootarg?
- Are board facts in Device Tree instead of parameters?
- Are logs device-scoped with `dev_*()` after probe begins?
- Are repeated logs rate-limited?
- Are high-frequency paths free of noisy logging?
- Does probe use `dev_err_probe()` for provider/resource failures?
- Can dynamic debug enable detailed logs without rebuilding?

## Related Topics

- [Kernel Module Lifecycle](kernel-module-lifecycle.md)
- [Dmesg And Log Levels](../debugging/dmesg-and-log-levels.md)
- [Dynamic Debug](../debugging/dynamic-debug.md)
- [Probe Failure Debugging](../debugging/probe-failure-debugging.md)
- [Device Tree Hardware Description](device-tree-hardware-description.md)

## Official References

- [Message logging with printk](https://docs.kernel.org/core-api/printk-basics.html)
- [Dynamic debug](https://docs.kernel.org/admin-guide/dynamic-debug-howto.html)
- [The Kernel's Command-Line Parameters](https://docs.kernel.org/admin-guide/kernel-parameters.html)

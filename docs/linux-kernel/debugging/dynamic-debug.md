---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# Dynamic Debug

## What Problem Does This Solve?

Dynamic debug enables selected debug logs at runtime without rebuilding the kernel.

It lets you keep useful `pr_debug()` and `dev_dbg()` call sites in code while normal boots stay quiet.

Instead of changing source from `dev_dbg()` to `dev_info()` and rebuilding, you can turn on only the call sites you need:

```text
module demo +p
file drivers/iio/adc/demo.c +p
func demo_read_raw +p
```

## Core Concepts

- `pr_debug`
- `dev_dbg`
- dynamic debug control file
- file filters
- function filters
- module filters
- format filters
- boot-time enablement

## Mental Model

Dynamic debug turns compiled-in debug call sites into targeted runtime instrumentation.

```text
driver contains dev_dbg()
-> kernel builds call site metadata
-> dynamic debug control file selects call sites
-> selected messages print at runtime
```

Use it when you need state evidence from a known path. Use tracing when you need timing/order evidence from many paths.

## Requirements

Kernel config commonly needs:

```text
CONFIG_DYNAMIC_DEBUG
CONFIG_DEBUG_FS
```

Exact dependencies vary by kernel version and distribution.

Check runtime:

```sh
mount | grep debugfs
ls /sys/kernel/debug/dynamic_debug/control
```

Mount debugfs in a lab if appropriate:

```sh
sudo mount -t debugfs none /sys/kernel/debug
```

Do not treat debugfs as a production ABI. Its availability is a product policy decision.

## Writing Useful Call Sites

Good:

```c
dev_dbg(dev, "status=%#x irq_count=%u\n",
        status, priv->irq_count);
```

Good:

```c
dev_dbg(dev, "rx len=%zu dma=%pad\n", len, &dma);
```

Bad:

```c
dev_dbg(dev, "here\n");
```

A debug call site should record state that helps answer a question.

## Control File Basics

The control file lists dynamic debug call sites and accepts commands.

```sh
sudo cat /sys/kernel/debug/dynamic_debug/control | head
```

Enable a whole module:

```sh
echo 'module demo +p' | sudo tee /sys/kernel/debug/dynamic_debug/control
```

Disable it:

```sh
echo 'module demo -p' | sudo tee /sys/kernel/debug/dynamic_debug/control
```

Enable a source file:

```sh
echo 'file drivers/misc/demo.c +p' | sudo tee /sys/kernel/debug/dynamic_debug/control
```

Enable a function:

```sh
echo 'func demo_probe +p' | sudo tee /sys/kernel/debug/dynamic_debug/control
```

Enable by format substring:

```sh
echo 'format "status=%#x" +p' | sudo tee /sys/kernel/debug/dynamic_debug/control
```

Use narrow filters first. Broad module-wide debug can flood logs.

## Flags

The most common flag is:

```text
+p   enable printing
-p   disable printing
```

Dynamic debug also supports additional decorations on many kernels, such as function, line, module, thread ID, or timestamp style information through flags. Check the dynamic debug documentation and the control file help for the target kernel.

Keep commands simple until you need extra metadata.

## Boot-Time Enablement

Dynamic debug can be enabled at boot through command-line parameters, depending on kernel configuration.

Common shapes:

```text
dyndbg="module demo +p"
demo.dyndbg="+p"
```

Use boot-time enablement when the evidence happens before userspace can write the control file, such as early probe.

Archive the exact boot argument in the bug report.

## Probe Debugging Example

Driver code:

```c
static int demo_probe(struct platform_device *pdev)
{
    struct device *dev = &pdev->dev;
    int ret;

    dev_dbg(dev, "probe start\n");

    ret = demo_get_resources(pdev);
    if (ret)
        return dev_err_probe(dev, ret, "failed to get resources\n");

    dev_dbg(dev, "resources ready\n");
    return 0;
}
```

Enable:

```sh
echo 'func demo_probe +p' | sudo tee /sys/kernel/debug/dynamic_debug/control
```

Run bind/unbind or reload module:

```sh
echo 48000000.demo | sudo tee /sys/bus/platform/drivers/demo/unbind
echo 48000000.demo | sudo tee /sys/bus/platform/drivers/demo/bind
```

Collect:

```sh
dmesg --time-format=iso | grep demo
```

Use bind/unbind only in controlled lab conditions.

## Runtime Path Example

For a read callback:

```c
dev_dbg(dev, "read count=%zu ready=%d\n", count, ready);
```

Enable only that function:

```sh
echo 'func demo_read +p' | sudo tee /sys/kernel/debug/dynamic_debug/control
```

Avoid enabling every call site in a high-frequency driver unless you are prepared for log volume and timing changes.

## Dynamic Debug Versus `dev_info()`

Use `dev_info()` for important normal events that should appear without special setup.

Use `dev_dbg()` for diagnostic state that is useful only while investigating.

Wrong:

```c
dev_info(dev, "status=%#x\n", status);
```

in a hot path.

Better:

```c
dev_dbg(dev, "status=%#x\n", status);
```

Then enable it only when needed.

## Dynamic Debug Versus Ftrace

| Need | Prefer |
| --- | --- |
| print driver state from known call site | dynamic debug |
| trace many function calls | ftrace |
| measure callback duration | function graph tracer or tracepoints |
| capture IRQ/workqueue scheduling | trace events |
| profile CPU cost | perf |

Dynamic debug is still logging. It does not automatically show all call order or timing.

## Evidence Discipline

When using dynamic debug, record:

```text
kernel version
command used
time enabled
test workload
time disabled
log capture
```

Disable after collection:

```sh
echo 'module demo -p' | sudo tee /sys/kernel/debug/dynamic_debug/control
```

## Common Failure Modes

| Symptom | Likely Cause | First Checks |
| --- | --- | --- |
| control file missing | config/debugfs not available | `CONFIG_DYNAMIC_DEBUG`, mount debugfs |
| no messages appear | call site not matched or path not executed | control query and test path |
| log flood | filter too broad | disable and narrow filter |
| early probe logs missing | enabled too late | boot-time dyndbg |
| production exposure concern | debugfs mounted/enabled | product debugfs policy |
| timing changes | too many prints in hot path | use tracepoints/ftrace |

## Practice Exercises

### Exercise 1: Module Filter

Enable all dynamic debug call sites for one module:

```sh
echo 'module demo +p' | sudo tee /sys/kernel/debug/dynamic_debug/control
```

Run one test, save logs, then disable it.

### Exercise 2: Function Filter

Enable only a probe function or read callback. Confirm log volume is smaller than module-wide debug.

### Exercise 3: Add A Useful Call Site

Add one `dev_dbg()` that prints state needed for a real question. Avoid "entered function" logs unless call order itself is the question.

## Debugging Checklist

- Confirm dynamic debug is enabled in the config.
- Confirm the call site exists.
- Use narrow filters to avoid log floods.
- Capture the command used to enable logging.
- Use boot-time enablement for early probe evidence.
- Disable debug after collecting logs.
- Prefer `dev_dbg()` over temporary noisy `dev_info()`.
- Do not depend on debugfs in production behavior.

## Related Topics

- [Dmesg And Log Levels](dmesg-and-log-levels.md)
- [Debug Vs Production Configs](../configuration-and-platform-policy/debug-vs-production-configs.md)
- [Probe Failure Debugging](probe-failure-debugging.md)
- [Ftrace And Tracepoints](ftrace-and-tracepoints.md)

## Official References

- [Dynamic debug](https://docs.kernel.org/admin-guide/dynamic-debug-howto.html)
- [Debugfs](https://docs.kernel.org/filesystems/debugfs.html)

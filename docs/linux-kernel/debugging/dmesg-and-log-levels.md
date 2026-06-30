---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# Dmesg And Log Levels

## What Problem Does This Solve?

Kernel logs are the first evidence for boot, probe, runtime, and crash diagnosis.

They answer:

```text
what did the kernel report?
when did it happen?
which device or subsystem reported it?
what was the exact error code?
what happened before the final failure?
```

Logs are not a substitute for correct locking, tracing, or crash analysis, but they are almost always the first evidence to preserve.

## Core Concepts

- ring buffer
- `dmesg`
- log levels
- `printk`
- `pr_*`
- `dev_*`
- rate limiting
- persistent logs

## Mental Model

Logs should identify the subsystem, device, failure point, and error code. Device-scoped logs make correlation practical on real boards.

```text
bad log:
  failed

useful log:
  demo 48000000.demo: failed to enable vdd regulator: -517
```

The useful log gives device identity, operation, resource, and return code.

## Capturing Logs

Basic:

```sh
dmesg
dmesg --time-format=iso
dmesg --follow
```

With systemd journal:

```sh
journalctl -k -b
journalctl -k -f
journalctl -k -b -1
```

For board bring-up, serial logs are often better than logs captured after login because failures can happen before persistent storage is mounted.

Recommended capture for a bug report:

```text
power-on serial log
dmesg --time-format=iso
cat /proc/cmdline
uname -a
cat /proc/sys/kernel/tainted
```

## Filtering Logs

Useful filters:

```sh
dmesg | grep -i demo
dmesg | grep -E "probe|defer|fail|error|timeout|irq|reset"
journalctl -k -b -g demo
```

Do not rely only on filtered logs. Preserve the full log first, then filter a copy.

Why:

```text
the first failure may come from a regulator, clock, pinctrl, or bus provider,
not from the driver you are searching for
```

## Log Levels

Kernel messages have priority levels.

| Level | Name | Typical Driver Use |
| --- | --- | --- |
| 0 | `KERN_EMERG` | system unusable; almost never ordinary driver code |
| 1 | `KERN_ALERT` | immediate action needed |
| 2 | `KERN_CRIT` | critical condition |
| 3 | `KERN_ERR` | operation failed |
| 4 | `KERN_WARNING` | suspicious or degraded behavior |
| 5 | `KERN_NOTICE` | notable normal condition |
| 6 | `KERN_INFO` | informational, use sparingly |
| 7 | `KERN_DEBUG` | debug messages |

Common APIs:

```c
dev_err(dev, "transfer failed: %d\n", ret);
dev_warn(dev, "using fallback mode\n");
dev_info(dev, "probed revision %#x\n", rev);
dev_dbg(dev, "status=%#x\n", status);
```

Use `dev_*()` once a `struct device *` exists. It adds device identity.

Use `pr_*()` for code that does not have a device, such as module init before device registration.

## Console Log Level

The kernel ring buffer can contain messages that are not printed to the console.

Inspect:

```sh
cat /proc/sys/kernel/printk
```

Temporarily change console verbosity:

```sh
dmesg -n 7
```

Boot arguments can also affect logging:

```text
loglevel=7
ignore_loglevel
quiet
printk.time=1
```

Do not confuse "not printed on console" with "not present in the ring buffer."

## Device-Scoped Logging

Example probe path:

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

The log carries the device name:

```text
demo 48000000.demo: using irq 42
```

That matters when one driver supports multiple device instances.

## `dev_err_probe()`

Use `dev_err_probe()` for probe failures, especially resource/provider lookup:

```c
priv->vdd = devm_regulator_get(dev, "vdd");
if (IS_ERR(priv->vdd))
    return dev_err_probe(dev, PTR_ERR(priv->vdd),
                         "failed to get vdd regulator\n");
```

Benefits:

- includes device identity
- returns the original error code
- handles `-EPROBE_DEFER` logging appropriately
- keeps probe error paths consistent

Preserving the original return code matters. Do not collapse every probe failure to `-EINVAL`.

## Rate-Limited Logs

Use rate-limited logs when a message can repeat:

```c
dev_warn_ratelimited(dev, "device not ready\n");
pr_err_ratelimited("demo: transfer failed\n");
```

Good places:

- retry loops
- interrupt storms
- repeated invalid userspace requests
- periodic polling failures
- transient hardware failures

Avoid logging unconditionally from high-frequency paths.

Wrong:

```c
static irqreturn_t demo_irq(int irq, void *data)
{
    dev_info(dev, "irq fired\n");
    return IRQ_HANDLED;
}
```

Better:

```c
dev_dbg_ratelimited(priv->dev, "irq count=%u\n", priv->irq_count);
```

Or use tracepoints/counters instead of logs.

## One-Time Logs

Use once-only logs for known degraded modes:

```c
dev_warn_once(dev, "legacy register layout detected\n");
```

This gives evidence without flooding the log.

## Printing Error Codes

Always include return codes:

```c
ret = clk_prepare_enable(priv->clk);
if (ret)
    return dev_err_probe(dev, ret, "failed to enable core clock\n");
```

Convert when reading logs:

```text
-517 -> -EPROBE_DEFER
-22  -> -EINVAL
-19  -> -ENODEV
-2   -> -ENOENT
-110 -> -ETIMEDOUT
-5   -> -EIO
```

Use kernel headers or `errno` references for exact names. Error code meaning depends on call context, so do not stop at the number.

## Persistent Logs

After a crash or watchdog reset, normal ring-buffer contents may be gone.

Useful mechanisms:

- serial console capture
- persistent journal, if storage is available
- pstore/ramoops
- netconsole in some lab setups
- bootloader reset-reason logs
- early boot scripts that copy previous logs

If the product must diagnose field resets, plan persistent evidence before failures occur.

## Log Quality Checklist For Driver Code

Useful logs answer:

```text
which device?
which operation?
which resource?
what value or state?
what return code?
is this fatal, degraded, or debug-only?
can this repeat?
```

Example:

```c
dev_err(dev, "failed to request threaded irq %d: %d\n", irq, ret);
```

Not:

```c
dev_err(dev, "irq failed\n");
```

## Common Failure Modes

| Symptom | Likely Cause | First Checks |
| --- | --- | --- |
| no boot logs | wrong console or loglevel | command line, serial wiring |
| logs missing after crash | no persistent capture | pstore/serial/journal |
| driver messages lack device identity | using `pr_*()` after device exists | convert to `dev_*()` |
| probe deferral looks fatal | plain `dev_err()` on `-EPROBE_DEFER` | use `dev_err_probe()` |
| log flood | hot-path logging | rate limit or trace |
| final error misleading | first error lost | full boot log |

## Practice Exercises

### Exercise 1: Full Evidence Capture

Capture:

```sh
dmesg --time-format=iso > dmesg.txt
cat /proc/cmdline > cmdline.txt
uname -a > uname.txt
cat /proc/sys/kernel/tainted > tainted.txt
```

Then filter `dmesg.txt` for your driver without losing the original.

### Exercise 2: Improve Probe Logs

Convert a probe error path to `dev_err_probe()`. Confirm the log contains the resource name and original return code.

### Exercise 3: Hot-Path Logging

Add a debug message to a repeated path, then change it to `dev_dbg_ratelimited()` or trace-based evidence.

## Debugging Checklist

- Capture logs from power-on, not only after login.
- Preserve timestamps.
- Keep the first error, not only the final failure.
- Decode negative error codes.
- Prefer `dev_*()` after a device exists.
- Use `dev_err_probe()` in probe provider/resource paths.
- Rate-limit repeated messages.
- Archive logs with kernel version, command line, and taint state.

## Related Topics

- [Module Parameters And Driver Logging](../fundamentals/module-parameters-and-logging.md)
- [Probe Failure Debugging](probe-failure-debugging.md)
- [Embedded Linux](../../embedded-linux/index.md)
- [Oops, Panic, And Crash Logs](oops-panic-crash-logs.md)

## Official References

- [Message logging with printk](https://docs.kernel.org/core-api/printk-basics.html)
- [The kernel command-line parameters](https://docs.kernel.org/admin-guide/kernel-parameters.html)
- [Tainted kernels](https://docs.kernel.org/admin-guide/tainted-kernels.html)

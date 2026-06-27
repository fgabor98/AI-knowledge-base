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

## Practice Skeleton

- Capture full boot logs.
- Filter logs by driver name.
- Add one useful `dev_err_probe` path.
- Compare noisy and rate-limited logging.

## Debugging Checklist

- Capture logs from power-on, not only after login.
- Preserve timestamps.
- Keep the first error, not only the final failure.
- Decode negative error codes.

## Related Topics

- [Module Parameters And Driver Logging](../fundamentals/module-parameters-and-logging.md)
- [Probe Failure Debugging](probe-failure-debugging.md)
- [Embedded Linux](../../embedded-linux/index.md)

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

## Core Concepts

- `module_param`
- parameter permissions
- boot-time parameters
- `pr_*`
- `dev_*`
- log levels
- rate-limited logging
- dynamic debug

## Mental Model

Use module parameters sparingly for policy or diagnostics that cannot come from firmware data. Use `dev_*` logging once a driver has a device context, because it preserves the device identity in logs.

## Practice Skeleton

- Add one validated module parameter.
- Convert generic logs to device-scoped logs.
- Add a rate-limited warning path.
- Enable dynamic debug for the driver.

## Debugging Checklist

- Check effective parameter values.
- Check whether the parameter exists for built-in drivers.
- Prefer device-scoped logs after probe begins.
- Avoid noisy logs in interrupt paths.

## Related Topics

- [Kernel Module Lifecycle](kernel-module-lifecycle.md)
- [Dmesg And Log Levels](../debugging/dmesg-and-log-levels.md)
- [Dynamic Debug](../debugging/dynamic-debug.md)

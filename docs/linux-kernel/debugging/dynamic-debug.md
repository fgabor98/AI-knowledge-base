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

## Practice Skeleton

- Enable debug logs for one module.
- Enable logs for one source file.
- Disable logs after collecting evidence.
- Add a useful `dev_dbg` call site.

## Debugging Checklist

- Confirm dynamic debug is enabled in the config.
- Confirm the call site exists.
- Use narrow filters to avoid log floods.
- Capture the command used to enable logging.

## Related Topics

- [Dmesg And Log Levels](dmesg-and-log-levels.md)
- [Debug Vs Production Configs](../configuration-and-platform-policy/debug-vs-production-configs.md)
- [Probe Failure Debugging](probe-failure-debugging.md)

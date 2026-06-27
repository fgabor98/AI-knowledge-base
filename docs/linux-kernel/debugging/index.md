---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# Kernel Debugging Basics

This track covers the first debugging tools and habits for kernel and driver work.

## Learning Materials

1. [Dmesg And Log Levels](dmesg-and-log-levels.md)
2. [Dynamic Debug](dynamic-debug.md)
3. [Ftrace And Tracepoints](ftrace-and-tracepoints.md)
4. [Perf Overview](perf-overview.md)
5. [Debugfs And Sysfs Inspection](debugfs-and-sysfs-inspection.md)
6. [KGDB Basics](kgdb-basics.md)
7. [Oops, Panic, And Crash Logs](oops-panic-crash-logs.md)
8. [Watchdog Reset Diagnosis](watchdog-reset-diagnosis.md)
9. [Probe Failure Debugging](probe-failure-debugging.md)

## Mental Model

Kernel debugging starts with observable state: boot logs, driver probe logs, runtime filesystems, trace data, and crash evidence. Use stronger tools only after the failure is classified.

## Completion Criteria

- Read and filter kernel logs effectively.
- Enable dynamic debug for a driver.
- Capture a basic ftrace trace.
- Classify probe failures, panics, and watchdog resets.

## Related Topics

- [Debugging](../../debugging/index.md)
- [Kernel Execution And Concurrency](../execution-and-concurrency/index.md)
- [Debug Vs Production Configs](../configuration-and-platform-policy/debug-vs-production-configs.md)

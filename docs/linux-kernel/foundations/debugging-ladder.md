---
status: draft
reviewed: false
domain: linux-kernel
difficulty: beginner
last_reviewed: null
---

# Debugging Ladder

## What Problem Does This Solve?

Kernel debugging tools vary in cost and complexity. A ladder helps beginners start with cheap evidence before reaching for heavy tools.

## Core Concepts

- `dmesg`
- `dev_*` logging
- dynamic debug
- sysfs inspection
- debugfs inspection
- ftrace
- tracepoints
- perf
- kgdb
- crash dumps

## Mental Model

Use the least invasive tool that can answer the current question. Move up the ladder when the evidence is insufficient.

```text
logs
-> runtime filesystem inspection
-> targeted debug logs
-> tracing
-> profiling
-> interactive debugging or crash analysis
```

## Practice Skeleton

- Diagnose a probe failure with `dmesg`.
- Enable dynamic debug for one driver.
- Inspect device state in sysfs.
- Capture one short ftrace trace.

## Debugging Checklist

- State the question before selecting a tool.
- Preserve the first failure evidence.
- Avoid adding noisy logs in hot paths.
- Prefer targeted tracing over broad tracing.

## Related Topics

- [Kernel Debugging Basics](../debugging/index.md)
- [Dmesg And Log Levels](../debugging/dmesg-and-log-levels.md)
- [Dynamic Debug](../debugging/dynamic-debug.md)
- [Ftrace And Tracepoints](../debugging/ftrace-and-tracepoints.md)

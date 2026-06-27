---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# Ftrace And Tracepoints

## What Problem Does This Solve?

Ftrace and tracepoints capture runtime execution and timing evidence without relying only on logs.

## Core Concepts

- tracing filesystem
- function tracing
- function graph tracing
- trace events
- tracepoints
- filters
- trace markers
- trace-cmd

## Mental Model

Tracing answers "what happened when?" Logs explain selected states; traces reveal execution order, timing, and callback paths.

## Practice Skeleton

- Enable function tracing for one driver.
- Capture IRQ or scheduler tracepoints.
- Add a filter to keep output small.
- Save a trace for review.

## Debugging Checklist

- Keep tracing windows short.
- Use filters before enabling broad tracing.
- Check tracing overhead.
- Correlate trace timestamps with logs.

## Related Topics

- [Perf Overview](perf-overview.md)
- [Context Rules](../execution-and-concurrency/context-rules.md)
- [Debug Vs Production Configs](../configuration-and-platform-policy/debug-vs-production-configs.md)

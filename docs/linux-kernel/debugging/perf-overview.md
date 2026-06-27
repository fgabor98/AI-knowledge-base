---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# Perf Overview

## What Problem Does This Solve?

Perf helps investigate CPU cost, scheduling behavior, counters, and profiling data across kernel and userspace.

## Core Concepts

- sampling
- call graphs
- hardware counters
- software events
- scheduler events
- kernel symbols
- flame graphs
- overhead

## Mental Model

Use perf when the problem is performance or timing, not when the first problem is missing hardware resources or failed probe.

## Practice Skeleton

- Record a short CPU profile.
- Capture call graphs with symbols.
- Inspect scheduler-related events.
- Compare idle and active workloads.

## Debugging Checklist

- Confirm symbol availability.
- Check sampling overhead.
- Record the workload and duration.
- Do not infer causality from samples alone.

## Related Topics

- [Ftrace And Tracepoints](ftrace-and-tracepoints.md)
- [Debugging](../../debugging/index.md)
- [Debug Vs Production Configs](../configuration-and-platform-policy/debug-vs-production-configs.md)

---
status: draft
reviewed: false
domain: linux-kernel
difficulty: advanced
last_reviewed: null
---

# Suspend And Resume Debugging

## What Problem Does This Solve?

Suspend and resume failures can appear as hangs, immediate wakeups, missing devices, corrupted state, or watchdog resets.

## Core Concepts

- suspend test modes
- wakeup statistics
- PM debug messages
- ftrace suspend tracing
- noirq failures
- device ordering
- persistent logs
- rollback paths

## Mental Model

Classify the failure by phase: entering suspend, staying suspended, waking unexpectedly, resuming devices, or recovering userspace.

## Practice Skeleton

- Run suspend test modes.
- Enable PM debug logs.
- Trace suspend and resume callbacks.
- Capture wakeup source data.

## Debugging Checklist

- Check the last device before failure.
- Check wakeup counters.
- Check runtime PM state before suspend.
- Compare failure behavior with one device disabled.

## Related Topics

- [Suspend And Resume](suspend-resume.md)
- [Ftrace And Tracepoints](../debugging/ftrace-and-tracepoints.md)
- [Watchdog Reset Diagnosis](../debugging/watchdog-reset-diagnosis.md)

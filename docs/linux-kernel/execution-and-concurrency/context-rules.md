---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# Context Rules

## What Problem Does This Solve?

Kernel APIs are only valid in certain execution contexts. Calling a sleepable API from atomic context can break the system.

## Core Concepts

- process context
- interrupt context
- softirq context
- threaded IRQ context
- preemption
- scheduling
- sleepable APIs
- atomic context

## Mental Model

Before calling an API, ask where the current code is running and whether it may sleep. Context determines which locks, allocations, and subsystem calls are legal.

## Practice Skeleton

- Annotate a driver with expected contexts for each callback.
- Move sleepable work out of an IRQ handler.
- Trigger warnings with debug configs in a controlled lab.

## Debugging Checklist

- Check stack traces for "scheduling while atomic".
- Check lockdep warnings.
- Check whether the callback is called from IRQ, workqueue, file operation, or probe.
- Review allocation flags.

## Related Topics

- [Sleepable Vs Atomic Code](sleepable-vs-atomic-code.md)
- [IRQ Handling](../driver-interfaces/irq-handling.md)
- [Debug Vs Production Configs](../configuration-and-platform-policy/debug-vs-production-configs.md)

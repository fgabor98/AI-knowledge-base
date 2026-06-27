---
status: draft
reviewed: false
domain: linux-kernel
difficulty: beginner
last_reviewed: null
---

# Execution Context Primer

## What Problem Does This Solve?

Many driver bugs come from calling the right API in the wrong context.

## Core Concepts

- process context
- interrupt context
- threaded IRQ context
- workqueue context
- timer context
- sleepable context
- atomic context
- blocking APIs
- allocation flags

## Mental Model

The first question before calling a kernel API is: can this code sleep? The answer determines which locks, allocations, bus operations, and helper APIs are legal.

## Practice Skeleton

- Mark each callback in a small driver as sleepable or atomic.
- Move an I2C or SPI operation out of hard IRQ context.
- Compare a hard IRQ handler, threaded IRQ handler, and workqueue callback.

## Debugging Checklist

- Look for "scheduling while atomic" warnings.
- Check whether the path holds a spinlock.
- Check allocation flags.
- Check whether bus transactions can sleep.

## Related Topics

- [Context Rules](../execution-and-concurrency/context-rules.md)
- [Sleepable Vs Atomic Code](../execution-and-concurrency/sleepable-vs-atomic-code.md)
- [Threaded Interrupts](../driver-interfaces/threaded-interrupts.md)

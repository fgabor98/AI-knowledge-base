---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# Threaded Interrupts

## What Problem Does This Solve?

Threaded interrupts let drivers handle interrupt-triggered work in a kernel thread where sleeping operations are allowed.

## Core Concepts

- hard IRQ handler
- threaded IRQ handler
- `request_threaded_irq`
- `IRQ_WAKE_THREAD`
- oneshot interrupts
- sleepable context
- interrupt masking

## Mental Model

Use the hard handler to decide whether the interrupt belongs to the device. Use the threaded handler for bus transactions, register reads, locking that can sleep, and user-visible state updates.

## Practice Skeleton

- Convert a hard IRQ handler to a threaded IRQ.
- Move I2C or SPI transactions into the thread function.
- Test interrupt storm behavior.

## Debugging Checklist

- Confirm the hard handler returns the right IRQ status.
- Use `IRQF_ONESHOT` when required.
- Check lock ordering between IRQ and process context.
- Avoid long unbounded work in the threaded handler.

## Related Topics

- [IRQ Handling](irq-handling.md)
- [Sleepable Vs Atomic Code](../execution-and-concurrency/sleepable-vs-atomic-code.md)
- [Workqueues](../execution-and-concurrency/workqueues.md)

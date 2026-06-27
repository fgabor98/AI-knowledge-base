---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# Sleepable Vs Atomic Code

## What Problem Does This Solve?

Drivers must separate operations that can sleep from operations that must complete without scheduling.

## Core Concepts

- `GFP_KERNEL`
- `GFP_ATOMIC`
- mutex
- spinlock
- hard IRQ
- threaded IRQ
- workqueue
- blocking bus transfers

## Mental Model

If code might wait for hardware, memory, I/O, locks, or scheduling, it needs a sleepable context. If the current context cannot sleep, defer the work.

## Practice Skeleton

- Classify callbacks in a sample driver.
- Replace an invalid sleepable call in IRQ context with a threaded IRQ or workqueue.
- Audit allocation flags.

## Debugging Checklist

- Look for I2C, SPI, regulator, clock, or memory allocation calls under spinlocks.
- Check `might_sleep` warnings.
- Use lockdep-enabled kernels during development.

## Related Topics

- [Threaded Interrupts](../driver-interfaces/threaded-interrupts.md)
- [Workqueues](workqueues.md)
- [Kernel Memory Allocation](../memory-and-io/kernel-memory-allocation.md)

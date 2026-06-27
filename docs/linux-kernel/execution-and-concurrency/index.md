---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# Kernel Execution And Concurrency

This track covers where kernel code runs, what it is allowed to do in each context, and how drivers protect shared state.

## Learning Materials

1. [Context Rules](context-rules.md)
2. [Sleepable Vs Atomic Code](sleepable-vs-atomic-code.md)
3. [Bottom Halves, Softirqs, And Tasklets](bottom-halves-softirqs-and-tasklets.md)
4. [Locking And Atomics](locking-and-atomics.md)
5. [Workqueues](workqueues.md)
6. [Concurrency Managed Workqueues](concurrency-managed-workqueues.md)
7. [Timers](timers.md)
8. [Hrtimers](hrtimers.md)
9. [Timekeeping And Kernel Timers](timekeeping-and-kernel-timers.md)
10. [Wait Queues And Completions](wait-queues-and-completions.md)
11. [Reference Counting And Lifetime](reference-counting-and-lifetime.md)

## Mental Model

Most driver bugs are lifetime or context bugs: code sleeps where it cannot, touches state after teardown, races interrupt and process context, or assumes callbacks stop immediately.

## Completion Criteria

- Identify process context, interrupt context, softirq context, and threaded interrupt context.
- Choose between mutexes, spinlocks, atomics, completions, and workqueues.
- Choose between tasklets, workqueues, threaded interrupts, timers, and hrtimers.
- Stop asynchronous activity during remove and shutdown.
- Explain why reference ownership matters for devices and files.

## Related Topics

- [IRQ Handling](../driver-interfaces/irq-handling.md)
- [Threaded Interrupts](../driver-interfaces/threaded-interrupts.md)
- [Kernel Debugging Basics](../debugging/index.md)

---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# Locking And Atomics

## What Problem Does This Solve?

Locking protects shared state from concurrent access across CPUs, interrupts, timers, workqueues, and userspace-facing callbacks.

## Core Concepts

- mutex
- spinlock
- rwsem
- atomic variables
- memory ordering overview
- lock ordering
- lockdep
- interrupt-safe locking

## Mental Model

Choose the simplest lock that is legal in all contexts that touch the state. Then document and preserve lock ordering.

## Practice Skeleton

- Protect driver state with a mutex.
- Protect IRQ-shared state with a spinlock.
- Replace a counter with an atomic only when the operation is truly atomic enough.
- Trigger lockdep on an intentional inversion in a lab.

## Debugging Checklist

- Check all access paths to shared state.
- Check whether locks can sleep.
- Check lock ordering and nested locks.
- Avoid using atomics as a substitute for protecting compound state.

## Related Topics

- [Context Rules](context-rules.md)
- [Wait Queues And Completions](wait-queues-and-completions.md)
- [Kernel Debugging Basics](../debugging/index.md)

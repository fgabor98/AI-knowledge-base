---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# Reference Counting And Lifetime

## What Problem Does This Solve?

Kernel objects often outlive the function that created them. Reference counting prevents use-after-free and premature teardown.

## Core Concepts

- object ownership
- kref
- device references
- file private data
- callback lifetime
- teardown state
- use-after-free
- double free

## Mental Model

Every pointer crossing an async boundary needs a lifetime story: who owns it, who can use it, and what prevents it from being freed too early.

## Practice Skeleton

- Store driver state in file private data.
- Block new opens during removal.
- Wait for active users before freeing state.
- Add explicit teardown state.

## Debugging Checklist

- Check all async callbacks.
- Check open file descriptors during device removal.
- Use KASAN-enabled kernels during development.
- Avoid freeing state before work, timers, IRQs, and users are stopped.

## Related Topics

- [Driver Binding, Probe, And Remove](../fundamentals/driver-binding-probe-remove.md)
- [Workqueues](workqueues.md)
- [Oops, Panic, And Crash Logs](../debugging/oops-panic-crash-logs.md)

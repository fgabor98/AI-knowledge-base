---
status: draft
reviewed: false
domain: linux-kernel
difficulty: advanced
last_reviewed: null
---

# KGDB Basics

## What Problem Does This Solve?

KGDB allows source-level debugging of the running kernel through a debugger connection.

## Core Concepts

- kgdb
- kgdboc
- gdb
- breakpoints
- serial console conflicts
- debug symbols
- remote debugging
- stop-the-world behavior

## Mental Model

KGDB is a heavy debugging tool for cases where logs and tracing are not enough. It changes system behavior and needs a controlled lab setup.

## Practice Skeleton

- Build a kernel with debug symbols.
- Configure KGDB over serial.
- Break into the kernel.
- Inspect a driver data structure.

## Debugging Checklist

- Confirm serial wiring and console ownership.
- Match symbols to the running kernel.
- Avoid using KGDB on timing-sensitive failures without accounting for perturbation.
- Keep a recovery path for hung sessions.

## Related Topics

- [Oops, Panic, And Crash Logs](oops-panic-crash-logs.md)
- [Debug Vs Production Configs](../configuration-and-platform-policy/debug-vs-production-configs.md)
- [Debugging](../../debugging/index.md)

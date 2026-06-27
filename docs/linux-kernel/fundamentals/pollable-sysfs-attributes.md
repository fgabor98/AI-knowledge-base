---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# Pollable Sysfs Attributes

## What Problem Does This Solve?

Pollable sysfs attributes let userspace wait for state changes without repeatedly reading a file.

## Core Concepts

- `poll`
- `select`
- `sysfs_notify`
- wait queues
- state-change notification
- sysfs callbacks
- userspace event loops

## Mental Model

Polling sysfs should notify that state changed, not transport large data. Userspace still reads the attribute to observe the current value.

## Practice Skeleton

- Add a sysfs attribute representing state.
- Notify userspace when the state changes.
- Wait with `poll` or `select`.
- Confirm repeated events behave predictably.

## Debugging Checklist

- Keep a real state variable behind the notification.
- Avoid missed events by rereading state after wakeup.
- Confirm permissions allow intended userspace access.
- Do not use pollable sysfs as a high-rate data path.

## Related Topics

- [Sysfs Attributes](sysfs-attributes.md)
- [Wait Queues And Completions](../execution-and-concurrency/wait-queues-and-completions.md)
- [Debugfs And Sysfs Inspection](../debugging/debugfs-and-sysfs-inspection.md)

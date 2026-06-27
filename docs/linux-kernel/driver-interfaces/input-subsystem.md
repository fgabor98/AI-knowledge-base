---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# Input Subsystem

## What Problem Does This Solve?

The input subsystem exposes buttons, keys, touch devices, switches, and similar event-producing devices through a common event interface.

## Core Concepts

- input device
- event types
- key codes
- absolute and relative axes
- `input_report_*`
- `input_sync`
- `/dev/input/event*`
- `evtest`

## Mental Model

Input drivers report semantic events, not raw GPIO transitions. Userspace should receive key, switch, touch, or axis events through the standard input event interface.

## Practice Skeleton

- Allocate an input device.
- Set supported event bits.
- Report one key event.
- Inspect the result with `evtest`.

## Debugging Checklist

- Check event type and code.
- Call `input_sync` after reports.
- Check `/dev/input/event*` permissions.
- Confirm the event represents product behavior, not just electrical state.

## Related Topics

- [Polled Input Devices](polled-input-devices.md)
- [IRQ-Based Input Devices](irq-based-input-devices.md)
- [GPIO Consumer API](gpio-consumer-api.md)

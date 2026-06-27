---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# IRQ-Based Input Devices

## What Problem Does This Solve?

IRQ-based input drivers report input events in response to hardware interrupts, commonly from buttons, switches, or touch controllers.

## Core Concepts

- GPIO IRQ
- interrupt trigger type
- debounce
- threaded IRQ
- `input_report_key`
- `input_sync`
- wakeup source
- event device

## Mental Model

Use the interrupt to detect a possible state change, then report the semantic input event through the input subsystem.

## Practice Skeleton

- Request a GPIO button and IRQ.
- Register an input device.
- Report key events from the interrupt path.
- Test wake-from-suspend behavior where needed.

## Debugging Checklist

- Check interrupt polarity and trigger type.
- Debounce noisy buttons.
- Avoid sleeping in hard IRQ context.
- Confirm userspace receives both press and release events.

## Related Topics

- [Input Subsystem](input-subsystem.md)
- [Threaded Interrupts](threaded-interrupts.md)
- [Wake Sources](../power-management/wake-sources.md)

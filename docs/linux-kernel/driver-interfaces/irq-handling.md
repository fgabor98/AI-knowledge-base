---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# IRQ Handling

## What Problem Does This Solve?

Interrupt handling lets hardware notify the kernel about events without polling.

## Core Concepts

- IRQ number
- interrupt controller
- top half
- interrupt context
- `request_irq`
- `devm_request_irq`
- interrupt flags
- shared IRQs
- wake IRQs

## Mental Model

The hard IRQ handler must do the smallest safe amount of work, acknowledge or classify the interrupt, and defer slower work to a sleepable context.

## Practice Skeleton

- Retrieve an IRQ from a platform device.
- Register a minimal interrupt handler.
- Count events safely.
- Confirm the interrupt appears in `/proc/interrupts`.

## Debugging Checklist

- Check trigger type in Device Tree.
- Confirm the interrupt controller mapping.
- Check whether the interrupt line is masked by hardware.
- Avoid sleeping in hard IRQ context.

## Related Topics

- [Threaded Interrupts](threaded-interrupts.md)
- [Context Rules](../execution-and-concurrency/context-rules.md)
- [Device Tree](../../device-tree/index.md)

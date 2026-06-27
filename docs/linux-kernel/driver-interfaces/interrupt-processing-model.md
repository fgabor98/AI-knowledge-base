---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# Interrupt Processing Model

## What Problem Does This Solve?

Interrupt processing connects a hardware signal from an interrupt controller to the kernel action registered by a driver.

## Core Concepts

- interrupt controller
- interrupt domain
- hardware IRQ
- Linux IRQ number
- `irq_desc`
- `irq_chip`
- `irqaction`
- interrupt flow handler
- chained interrupt handler
- threaded interrupt
- interrupt affinity

## Mental Model

Interrupts travel through layers: controller hardware, interrupt-domain mapping, generic IRQ core, flow handling, and finally driver-registered actions.

```text
hardware line
-> interrupt controller
-> irqdomain mapping
-> generic IRQ core
-> flow handler
-> driver handler or threaded handler
```

## Practice Skeleton

- Map a Device Tree interrupt specifier to a Linux IRQ.
- Inspect `/proc/interrupts`.
- Register a driver interrupt handler.
- Move sleepable work into a threaded handler.

## Debugging Checklist

- Check interrupt-parent and trigger type.
- Check whether the interrupt controller driver probed.
- Check IRQ domain mapping.
- Check whether the handler is firing, masked, shared, or storming.

## Related Topics

- [IRQ Handling](irq-handling.md)
- [Threaded Interrupts](threaded-interrupts.md)
- [Bottom Halves, Softirqs, And Tasklets](../execution-and-concurrency/bottom-halves-softirqs-and-tasklets.md)

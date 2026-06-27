---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# Resource Lookup And Managed Allocation

## What Problem Does This Solve?

Drivers need to acquire memory, MMIO regions, IRQs, clocks, GPIOs, regulators, and other resources without leaking them across probe failures or device removal.

## Core Concepts

- `devm_*`
- resource lifetime
- probe failure cleanup
- `devm_kzalloc`
- `devm_platform_ioremap_resource`
- `devm_request_threaded_irq`
- provider dependencies
- `-EPROBE_DEFER`

## Mental Model

Managed resources are tied to the device lifetime. They simplify cleanup, but they do not remove the need to stop active hardware and asynchronous work in the right order.

## Practice Skeleton

- Convert manual allocation to `devm_kzalloc`.
- Request an MMIO resource with a managed helper.
- Request an IRQ with managed cleanup.
- Add one failure path and confirm cleanup behavior.

## Debugging Checklist

- Check whether a provider driver has probed.
- Check for `-EPROBE_DEFER`.
- Confirm cleanup ordering around interrupts and workqueues.
- Avoid mixing managed and unmanaged lifetimes without a reason.

## Related Topics

- [Driver Binding, Probe, And Remove](driver-binding-probe-remove.md)
- [Kernel Memory Allocation](../memory-and-io/kernel-memory-allocation.md)
- [MMIO And Register Access](../memory-and-io/mmio-and-register-access.md)

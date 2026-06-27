---
status: draft
reviewed: false
domain: linux-kernel
difficulty: advanced
last_reviewed: null
---

# Regulator And Clock Power Dependencies

## What Problem Does This Solve?

Devices often need regulators, clocks, resets, pin states, and power domains to be sequenced correctly before register access or bus traffic.

## Core Concepts

- power sequencing
- regulator enable
- clock enable
- reset deassertion
- pinctrl state
- runtime PM ordering
- startup delay
- shared dependencies

## Mental Model

Bring-up and power management fail when the driver controls one dependency while assuming the others are already valid.

## Practice Skeleton

- Write a power-up sequence for a device.
- Write the matching power-down sequence.
- Add delays required by the hardware manual.
- Test repeated enable and disable cycles.

## Debugging Checklist

- Check provider availability.
- Check enable order.
- Check startup delay requirements.
- Check shared-resource side effects.

## Related Topics

- [Clocks](../driver-interfaces/clocks.md)
- [Regulators](../driver-interfaces/regulators.md)
- [Resets](../driver-interfaces/resets.md)

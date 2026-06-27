---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# Resets

## What Problem Does This Solve?

Reset APIs let drivers control reset lines for hardware blocks without hard-coding reset controller details.

## Core Concepts

- reset controller
- reset consumer
- assert
- deassert
- shared resets
- exclusive resets
- reset sequencing

## Mental Model

Reset lines are part of the hardware block lifecycle. Drivers should request named reset controls and apply the sequence expected by the hardware.

## Practice Skeleton

- Request a reset control.
- Assert and deassert during probe.
- Handle optional resets.
- Combine reset, clock, and regulator sequencing.

## Debugging Checklist

- Check reset provider availability.
- Confirm reset polarity and shared ownership.
- Respect timing requirements from the hardware manual.
- Check whether boot firmware left hardware in a surprising state.

## Related Topics

- [Clocks](clocks.md)
- [Regulators](regulators.md)
- [Power Domains](../power-management/power-domains.md)

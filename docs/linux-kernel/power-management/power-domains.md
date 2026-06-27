---
status: draft
reviewed: false
domain: linux-kernel
difficulty: advanced
last_reviewed: null
---

# Power Domains

## What Problem Does This Solve?

Power domains group devices that share power control and sequencing constraints.

## Core Concepts

- generic PM domains
- providers
- consumers
- domain hierarchy
- device links
- always-on domains
- runtime PM interaction
- firmware-controlled domains

## Mental Model

A device may not be independently power-controlled. Its availability can depend on parent domains and firmware policy.

## Practice Skeleton

- Identify a device's power domain.
- Inspect runtime PM state.
- Test driver behavior when the domain powers down.
- Document parent and child dependencies.

## Debugging Checklist

- Check power-domain properties.
- Check provider probe order.
- Check runtime PM dependencies.
- Check firmware ownership boundaries.

## Related Topics

- [Runtime PM](runtime-pm.md)
- [Device Tree](../../device-tree/index.md)
- [Regulator And Clock Power Dependencies](regulator-clock-power-dependencies.md)

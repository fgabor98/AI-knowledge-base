---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# Clocks

## What Problem Does This Solve?

Clock APIs let drivers enable, disable, and sometimes configure clock inputs required by hardware blocks.

## Core Concepts

- clock providers
- clock consumers
- `clk_get`
- `clk_prepare_enable`
- `clk_disable_unprepare`
- clock rates
- assigned clocks
- runtime PM interaction

## Mental Model

A driver consumes named clocks. Board and SoC descriptions decide which provider supplies each clock and what constraints apply.

## Practice Skeleton

- Request a required clock.
- Enable it during probe or runtime resume.
- Read the effective rate.
- Disable it in the matching power-down path.

## Debugging Checklist

- Check the `clocks` and `clock-names` properties.
- Check debugfs clock summaries.
- Check whether runtime PM gates the clock unexpectedly.
- Avoid changing shared clock rates without understanding consumers.

## Related Topics

- [Runtime PM](../power-management/runtime-pm.md)
- [Device Tree](../../device-tree/index.md)
- [Regulator And Clock Power Dependencies](../power-management/regulator-clock-power-dependencies.md)

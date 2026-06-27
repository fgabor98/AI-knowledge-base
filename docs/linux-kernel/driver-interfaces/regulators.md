---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# Regulators

## What Problem Does This Solve?

Regulator APIs let drivers request and control power rails needed by devices.

## Core Concepts

- regulator provider
- regulator consumer
- fixed regulator
- PMIC regulator
- voltage constraints
- enable count
- optional supplies
- supply names

## Mental Model

A driver requests supplies by role. Board policy defines voltage limits, startup delays, and which provider supplies the rail.

## Practice Skeleton

- Request required and optional supplies.
- Enable a supply during probe or runtime resume.
- Validate configured voltage constraints.
- Disable supplies in the matching shutdown path.

## Debugging Checklist

- Check supply property names.
- Check regulator constraints in Device Tree.
- Check enable counts and always-on rails.
- Do not change voltage without board-level validation.

## Related Topics

- [Power Management](../power-management/index.md)
- [Device Tree](../../device-tree/index.md)
- [Regulator And Clock Power Dependencies](../power-management/regulator-clock-power-dependencies.md)

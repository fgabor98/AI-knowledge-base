---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# PWM Driver Overview

## What Problem Does This Solve?

PWM drivers and consumers control pulse-width modulated signals for backlights, motors, buzzers, regulators, and other hardware functions.

## Core Concepts

- PWM provider
- PWM consumer
- period
- duty cycle
- polarity
- enable state
- Device Tree PWM specifiers
- subsystem-specific consumers

## Mental Model

PWM hardware should be exposed through the PWM subsystem. Product drivers should consume a named PWM rather than program timer registers directly.

## Practice Skeleton

- Identify a PWM provider.
- Consume a PWM from a driver.
- Configure period and duty cycle.
- Disable the PWM safely during teardown.

## Debugging Checklist

- Check pinmux.
- Check period and duty units.
- Check polarity.
- Check whether another subsystem already owns the PWM.

## Related Topics

- [Pinctrl](pinctrl.md)
- [Device Tree](../../device-tree/index.md)
- [Power Management](../power-management/index.md)

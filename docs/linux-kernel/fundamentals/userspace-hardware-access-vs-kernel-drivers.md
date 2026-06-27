---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# User-Space Hardware Access Vs Kernel Drivers

## What Problem Does This Solve?

Embedded prototypes often access GPIO, I2C, or SPI from userspace. Product drivers usually need kernel integration for interrupts, power management, sharing, and subsystem APIs.

## Core Concepts

- userspace GPIO tools
- `i2c-dev`
- `spidev`
- kernel subsystem drivers
- ownership
- power management
- interrupt handling
- ABI stability
- security policy

## Mental Model

Userspace access is useful for bring-up and experiments. A kernel driver is appropriate when the device needs subsystem integration, resource arbitration, IRQ handling, PM, or a stable product ABI.

## Practice Skeleton

- Prototype a register read from userspace.
- Identify what the prototype cannot handle safely.
- Move the device protocol into a kernel I2C or SPI client driver.
- Expose only the intended product interface to userspace.

## Debugging Checklist

- Check whether another kernel driver should own the device.
- Avoid bypassing regulator, clock, pinctrl, or runtime PM policy.
- Avoid shipping broad raw bus access as the product interface.
- Keep security and permissions explicit.

## Related Topics

- [I2C Client Drivers](../driver-interfaces/i2c-client-drivers.md)
- [SPI Device Drivers](../driver-interfaces/spi-device-drivers.md)
- [GPIO Consumer API](../driver-interfaces/gpio-consumer-api.md)

---
status: draft
reviewed: false
domain: linux-kernel
difficulty: beginner
last_reviewed: null
---

# Small Lab Progression

## What Problem Does This Solve?

A staged lab path lets beginners build confidence before touching complex real hardware drivers.

## Core Concepts

- hello module
- module parameters
- character device
- platform driver
- Device Tree matching
- GPIO consumer
- I2C client
- SPI client
- IRQ handling
- tracing

## Mental Model

Each lab should add one new kernel concept while reusing the previous workflow. Avoid combining new hardware, new build setup, and new subsystem code in the same first attempt.

## Practice Skeleton

1. Build and load a hello module.
2. Add module parameters and logging.
3. Create a dummy character device.
4. Convert it into a platform driver.
5. Match the platform driver from Device Tree.
6. Add a GPIO consumer.
7. Add a simple I2C or SPI client.
8. Add an IRQ or threaded IRQ.
9. Capture a short trace of the driver path.

## Debugging Checklist

- Verify each lab before adding the next concept.
- Keep one known-good module revision.
- Capture commands and logs for each stage.
- Prefer fake or dummy devices before relying on unstable hardware.

## Related Topics

- [Kernel Module Lifecycle](../fundamentals/kernel-module-lifecycle.md)
- [Character Device Basics](../fundamentals/character-device-basics.md)
- [Platform Devices And Platform Drivers](../fundamentals/platform-devices-and-drivers.md)
- [GPIO Consumer API](../driver-interfaces/gpio-consumer-api.md)

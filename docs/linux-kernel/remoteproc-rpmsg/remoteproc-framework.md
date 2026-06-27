---
status: draft
reviewed: false
domain: linux-kernel
difficulty: advanced
last_reviewed: null
---

# Remoteproc Framework

## What Problem Does This Solve?

Remoteproc provides a kernel framework for controlling auxiliary processors from Linux.

## Core Concepts

- remote processor
- firmware image
- resource table
- start and stop
- carveouts
- virtio devices
- crash handling
- sysfs control

## Mental Model

Remoteproc is the Linux-side lifecycle manager for a remote core. Platform drivers provide the SoC-specific power, reset, and memory operations.

## Practice Skeleton

- Identify remoteproc devices.
- Load and start firmware.
- Stop a remote core.
- Inspect resource table effects.

## Debugging Checklist

- Check firmware path.
- Check power and reset dependencies.
- Check reserved memory.
- Check remoteproc state transitions.

## Related Topics

- [Firmware Loading](firmware-loading.md)
- [Reserved Memory](reserved-memory.md)
- [Virtio And RPMsg](virtio-rpmsg.md)

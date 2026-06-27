---
status: draft
reviewed: false
domain: linux-kernel
difficulty: advanced
last_reviewed: null
---

# PRU Integration Overview

## What Problem Does This Solve?

PRU integration connects Linux with programmable real-time units commonly used for deterministic I/O tasks.

## Core Concepts

- PRU cores
- PRU firmware
- remoteproc
- shared memory
- interrupts
- RPMsg
- pinmux
- deterministic timing

## Mental Model

PRU work splits responsibilities: Linux owns orchestration and integration, while PRU firmware owns deterministic close-to-hardware timing.

## Practice Skeleton

- Identify PRU remoteproc instances.
- Load a test firmware.
- Validate shared-memory or RPMsg communication.
- Check pinmux for PRU-controlled pins.

## Debugging Checklist

- Check firmware compatibility.
- Check pin ownership.
- Check interrupt routing.
- Check shared-memory layout.

## Related Topics

- [Remoteproc Framework](remoteproc-framework.md)
- [Virtio And RPMsg](virtio-rpmsg.md)
- [Pinctrl](../driver-interfaces/pinctrl.md)

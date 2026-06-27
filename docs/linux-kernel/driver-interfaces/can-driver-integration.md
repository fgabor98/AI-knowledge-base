---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# CAN Driver Integration Overview

## What Problem Does This Solve?

CAN driver integration connects controller drivers and transceivers to the Linux networking stack.

## Core Concepts

- SocketCAN
- CAN controller
- CAN transceiver
- bit timing
- network device
- error frames
- bus-off recovery
- termination

## Mental Model

CAN appears to userspace as a network interface, but bring-up depends on controller clocks, pinmux, transceiver control, and physical bus conditions.

## Practice Skeleton

- Identify the CAN network device.
- Bring the interface up with a fixed bitrate.
- Send and receive a test frame.
- Review controller and transceiver Device Tree properties.

## Debugging Checklist

- Check clocks and pinmux.
- Check transceiver enable GPIO or regulator.
- Check bitrate and sample point.
- Check termination and physical wiring.

## Related Topics

- [Clocks](clocks.md)
- [Regulators](regulators.md)
- [Networking](../../networking/index.md)

---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# UART And TTY Integration Overview

## What Problem Does This Solve?

UART and TTY integration connects serial hardware drivers to Linux terminal, console, and line discipline behavior.

## Core Concepts

- UART controller
- serial core
- TTY layer
- console
- line discipline
- termios
- RS-485
- flow control

## Mental Model

Most product work uses existing UART controller drivers. Driver development requires understanding where controller code ends and TTY behavior begins.

## Practice Skeleton

- Identify the active UART driver.
- Map a Device Tree UART node to `/dev/tty*`.
- Inspect console configuration.
- Review RS-485 or flow-control properties where relevant.

## Debugging Checklist

- Check pinmux and baud rate.
- Check kernel command line console selection.
- Check systemd getty configuration.
- Distinguish electrical UART issues from TTY configuration issues.

## Related Topics

- [Device Tree](../../device-tree/index.md)
- [Dmesg And Log Levels](../debugging/dmesg-and-log-levels.md)
- [Embedded Linux](../../embedded-linux/index.md)

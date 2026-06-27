---
status: draft
reviewed: false
domain: linux-kernel
difficulty: beginner
last_reviewed: null
---

# Kernel Development Lab Setup

## What Problem Does This Solve?

Kernel experiments can crash or hang the system. A safe lab keeps learning fast without risking the main workstation or product board state.

## Core Concepts

- VM
- QEMU
- spare embedded board
- serial console
- recovery kernel
- known-good boot path
- matching source and config
- module build tree
- snapshots
- log capture

## Mental Model

Assume experimental kernel code can break boot, hang the system, or corrupt runtime state. Build a recovery path before loading custom modules.

## Practice Skeleton

- Prepare either a VM/QEMU target or a spare board.
- Capture boot logs from a serial console or VM console.
- Build and load a trivial external module.
- Confirm a recovery kernel or snapshot can restore the lab.

## Debugging Checklist

- Confirm kernel source, config, and modules match the target.
- Confirm serial or console logs are captured before login.
- Keep a known-good boot entry.
- Avoid first experiments on an irreplaceable production device.

## Related Topics

- [Kernel Build And Install Overview](../source-build-and-tailoring/kernel-build-and-install-overview.md)
- [Kernel Module Lifecycle](../fundamentals/kernel-module-lifecycle.md)
- [Dmesg And Log Levels](../debugging/dmesg-and-log-levels.md)

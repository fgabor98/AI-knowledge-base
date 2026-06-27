---
status: draft
reviewed: false
domain: linux-kernel
difficulty: advanced
last_reviewed: null
---

# R5 And M4 Firmware Lifecycle

## What Problem Does This Solve?

R5 and M4 class remote cores need explicit lifecycle management across boot, firmware updates, crashes, and system shutdown.

## Core Concepts

- firmware ownership
- bootloader-started cores
- Linux-started cores
- remoteproc attach
- reset control
- shared memory
- crash recovery
- update compatibility

## Mental Model

The first question is ownership: did firmware start before Linux, or does Linux own start and stop through remoteproc?

## Practice Skeleton

- Identify who starts the remote core.
- Test Linux attach or start flow.
- Stop and restart firmware where supported.
- Record firmware and kernel compatibility.

## Debugging Checklist

- Check bootloader handoff.
- Check reset and power domains.
- Check memory carveouts.
- Check firmware ABI version.

## Related Topics

- [Remoteproc Framework](remoteproc-framework.md)
- [Firmware Loading](firmware-loading.md)
- [Device Tree Nodes For Remote Cores](device-tree-nodes-for-remote-cores.md)

---
status: draft
reviewed: false
domain: linux-kernel
difficulty: advanced
last_reviewed: null
---

# Remoteproc, RPMsg, And Heterogeneous SoCs

This track covers Linux integration with auxiliary cores, firmware loading, shared memory, virtio, and RPMsg communication.

## Learning Materials

1. [Remoteproc Framework](remoteproc-framework.md)
2. [Firmware Loading](firmware-loading.md)
3. [Reserved Memory](reserved-memory.md)
4. [Virtio And RPMsg](virtio-rpmsg.md)
5. [PRU Integration Overview](pru-integration.md)
6. [R5 And M4 Firmware Lifecycle](r5-m4-firmware-lifecycle.md)
7. [Remote Core Logs And Crashes](remote-core-logs-and-crashes.md)
8. [Device Tree Nodes For Remote Cores](device-tree-nodes-for-remote-cores.md)

## Mental Model

Linux may be only one participant in a multi-core SoC. Remote cores need firmware, memory carveouts, power and reset control, communication channels, and crash policy.

## Completion Criteria

- Explain how remoteproc loads and starts firmware.
- Identify reserved memory used by a remote core.
- Explain RPMsg communication at a high level.
- Capture basic remote-core crash evidence.

## Related Topics

- [Device Tree](../../device-tree/index.md)
- [TI Processor SDK Firmware And Heterogeneous Cores](../../build-systems/advanced/ti-processor-sdk/firmware-and-heterogeneous-cores.md)
- [Embedded Linux](../../embedded-linux/index.md)

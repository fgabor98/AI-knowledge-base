---
status: draft
reviewed: false
domain: linux-kernel
difficulty: advanced
last_reviewed: null
---

# Remoteproc, RPMsg, And Heterogeneous SoCs

## What Problem Does This Solve?

Many embedded SoCs are not single-CPU Linux systems. Linux may run on the main
application cores while other processors handle real-time control, power
management, safety tasks, sensor fusion, industrial protocols, audio, vision,
or low-latency I/O.

Examples:

```text
application cores:
  Cortex-A running Linux

remote cores:
  Cortex-R5 safety/control firmware
  Cortex-M4 low-power firmware
  PRU real-time I/O firmware
  DSP signal-processing firmware
```

The Linux kernel must integrate those remote cores without pretending they are
ordinary Linux threads. Remote cores have their own instruction memory, reset
lines, power domains, firmware images, address maps, interrupts, shared memory,
and failure modes.

This chapter explains the Linux-side integration model:

- how remoteproc starts, stops, attaches to, and monitors auxiliary processors
- how firmware is loaded and matched to the platform
- how reserved memory and firmware resource tables define shared memory
- how virtio and RPMsg provide message-based communication
- how PRU, R5, and M4 class cores differ in ownership and lifecycle
- how to debug remote-core logs, crashes, and Device Tree integration

## Prerequisites

This chapter assumes you are comfortable with:

- platform devices and Device Tree matching
- reserved memory and DMA address concepts
- interrupts, mailboxes, resets, clocks, and power domains
- firmware files under `/lib/firmware`
- runtime inspection through sysfs, debugfs, and `dmesg`
- basic embedded build and release packaging

Remoteproc work is advanced not because the individual APIs are impossible, but
because every failure crosses a boundary between Linux, firmware, bootloader,
hardware, and product release engineering.

## Mental Model

Think of a remote core as a separate computer inside the SoC:

```text
Linux side
  -> remoteproc driver
     -> SoC power/reset/clock/mailbox operations
     -> firmware loader
     -> reserved memory / carveouts
     -> virtio/RPMsg devices

remote side
  -> firmware image
     -> vector table / entry point
     -> resource table
     -> shared memory usage
     -> interrupt protocol
     -> application protocol
```

The remoteproc framework provides the common Linux lifecycle. Platform drivers
provide the SoC-specific operations. Firmware provides code and often a resource
table. Device Tree describes the integration points.

## Linux Is Not Always The Owner

The first question in any heterogeneous-core design is ownership:

```text
Who starts the remote core?
Who may stop it?
Who owns its memory?
Who handles a crash?
Who updates the firmware?
```

Common ownership models:

| Model | Description | Linux Role |
| --- | --- | --- |
| Linux-started | remote core is held in reset until Linux loads firmware | load, start, stop, recover |
| bootloader-started | bootloader starts firmware before Linux boots | attach, monitor, maybe stop later |
| firmware-owned | secure or system firmware owns lifecycle | request services through provider |
| always-on companion | remote core is part of platform management | avoid disrupting it |
| development-only | Linux starts/stops test firmware interactively | useful for lab, not final policy |

A driver or product image that assumes the wrong ownership model can break
safety firmware, power management, or boot-time services.

## Main Components

| Component | Linux-Side Meaning |
| --- | --- |
| remoteproc core | common framework for remote processor lifecycle |
| remoteproc platform driver | SoC-specific power, reset, memory, mailbox, and start/stop code |
| firmware image | remote-core executable, commonly ELF |
| resource table | firmware-provided description of carveouts, traces, and virtio devices |
| carveout | memory region used by remote firmware |
| reserved memory | Device Tree memory region Linux must not allocate normally |
| virtio device | virtual device exposed through remoteproc resource table |
| vring | shared-memory ring used by virtio messaging |
| RPMsg | message bus built on top of virtio for remote-core communication |
| mailbox | hardware interrupt/message mechanism between Linux and remote core |
| trace buffer | firmware log buffer exposed through remoteproc when supported |

## What Remoteproc Does And Does Not Do

Remoteproc handles common lifecycle mechanics:

- finding firmware
- parsing supported firmware formats
- loading segments into memory
- processing resource tables
- creating virtio devices
- starting and stopping the remote core through platform callbacks
- reporting crashes
- optionally triggering recovery
- exposing sysfs/debugfs state

Remoteproc does not automatically solve:

- product protocol design over RPMsg
- firmware ABI versioning
- real-time correctness of remote firmware
- shared-memory cache coherency mistakes
- security policy for firmware images
- board-level pinmux, clocks, resets, and power-domain design
- safe update and rollback policy
- remote-side logging discipline

Those are product and platform responsibilities.

## Chapter Structure

Read the pages in this order:

1. [Remoteproc Framework](remoteproc-framework.md) explains the core lifecycle
   and sysfs/debugfs model.
2. [Firmware Loading](firmware-loading.md) explains firmware naming, packaging,
   resource tables, compatibility, and update policy.
3. [Reserved Memory](reserved-memory.md) explains carveouts, shared memory,
   cacheability, and address contracts.
4. [Virtio And RPMsg](virtio-rpmsg.md) explains remote-core messaging, vrings,
   endpoints, and protocol design.
5. [PRU Integration Overview](pru-integration.md) covers deterministic I/O with
   programmable real-time units.
6. [R5 And M4 Firmware Lifecycle](r5-m4-firmware-lifecycle.md) covers common
   Cortex-R/M ownership, boot, restart, and update patterns.
7. [Remote Core Logs And Crashes](remote-core-logs-and-crashes.md) covers trace
   buffers, coredumps, watchdogs, recovery, and diagnostics.
8. [Device Tree Nodes For Remote Cores](device-tree-nodes-for-remote-cores.md)
   ties together bindings, memory regions, mailboxes, resets, power domains,
   and firmware names.

## Typical System Flow

Linux-started remote core:

```text
boot kernel
  -> remoteproc platform driver probes
  -> resources acquired from Device Tree
  -> firmware name selected
  -> userspace or auto-boot starts remoteproc
  -> kernel requests firmware
  -> ELF segments loaded into carveouts
  -> resource table parsed
  -> virtio/RPMsg devices created if present
  -> platform start callback releases reset
  -> remote firmware runs
  -> RPMsg clients bind
```

Bootloader-started remote core:

```text
bootloader starts remote firmware
  -> Linux boots
  -> remoteproc driver probes
  -> driver attaches to already-running core
  -> Linux discovers communication resources
  -> Linux may monitor, exchange messages, or detach later
```

Crash flow:

```text
remote firmware faults or watchdog fires
  -> platform reports crash to remoteproc
  -> remoteproc marks core crashed
  -> coredump or trace evidence captured if configured
  -> recovery policy decides restart or leave stopped
  -> RPMsg devices disappear and may reappear
```

## Address Vocabulary

Remoteproc work often involves several address spaces.

| Address Type | Used By | Example Meaning |
| --- | --- | --- |
| CPU virtual address | Linux kernel after mapping | pointer used by Linux code |
| physical address | SoC physical memory map | RAM address in hardware manual |
| DMA address | bus address used by a device | may differ with IOMMU or DMA offsets |
| device address | remote core's view of memory | address in firmware linker script |
| resource table address | firmware-declared memory requirement | carveout or vring location |

Do not assume they are identical. Remoteproc platform drivers and firmware
resource tables exist partly because the remote core's address view may differ
from Linux's.

## Product Release Contract

Remoteproc systems require a release contract:

```text
kernel version
Device Tree
bootloader handoff
remote firmware image
remote firmware resource table
RPMsg protocol version
reserved-memory layout
userspace service
recovery policy
```

Changing one part can break another. For example:

```text
firmware changes vring size
  -> Device Tree reserved memory is too small
  -> remoteproc start fails or memory is corrupted
```

or:

```text
kernel image expects firmware name A
rootfs installs firmware name B
  -> remoteproc cannot start
```

Treat remote firmware as part of the platform software release, not as a manual
file copied onto a target after the image is built.

## Common Failure Patterns

| Symptom | Common Cause |
| --- | --- |
| remoteproc device missing | platform driver disabled, Device Tree node disabled, provider probe failure |
| firmware load fails | wrong filename, missing `/lib/firmware` file, initramfs/rootfs timing issue |
| start hangs | reset, clock, power domain, mailbox, or memory mapping problem |
| RPMsg device missing | firmware resource table lacks vdev, name service did not announce channel |
| messages time out | mailbox interrupt, vring memory, endpoint name, or protocol mismatch |
| Linux crashes after remote start | overlapping reserved memory or bad shared-memory cache handling |
| remote core crashes immediately | wrong firmware for SoC/core, bad linker script, missing memory |
| crash evidence missing | coredump disabled, trace buffer not declared, automatic recovery too fast |
| works on vendor image only | firmware, DTB, and kernel came from incompatible releases |

## Completion Criteria

You understand this chapter when you can:

- explain the difference between Linux-started, attached, and firmware-owned
  remote cores
- inspect `/sys/class/remoteproc` and identify remote processor state
- explain what a firmware resource table contributes
- map `memory-region` properties to reserved-memory nodes
- describe how virtio vrings and RPMsg endpoints fit together
- identify the Device Tree providers needed by a remoteproc node
- design a minimal RPMsg protocol with versioning and error handling
- collect useful evidence from a remote-core crash
- explain why firmware, kernel, DTB, and userspace must be released together

## Official References

- [Remote Processor Framework](https://docs.kernel.org/staging/remoteproc.html)
- [Remote Processor Messaging](https://docs.kernel.org/staging/rpmsg.html)
- [Firmware Loading API](https://docs.kernel.org/driver-api/firmware/request_firmware.html)
- [Remoteproc Sysfs ABI](https://docs.kernel.org/ABI/testing/sysfs-class-remoteproc)
- [Reserved Memory Binding](https://docs.kernel.org/devicetree/bindings/reserved-memory/reserved-memory.yaml)

## Related Topics

- [Device Tree](../../device-tree/index.md)
- [TI Processor SDK Firmware And Heterogeneous Cores](../../build-systems/advanced/ti-processor-sdk/firmware-and-heterogeneous-cores.md)
- [Embedded Linux](../../embedded-linux/index.md)

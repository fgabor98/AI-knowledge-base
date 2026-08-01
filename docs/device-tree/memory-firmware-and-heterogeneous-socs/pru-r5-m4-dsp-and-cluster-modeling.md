---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# PRU, R5/M4, DSP, And Cluster Modeling

“Remote core” covers processors with very different integration. A PRU may be tightly coupled to industrial I/O; an R5 cluster may support lockstep or split mode; an M4 may own low-power services; a DSP may use an MMU and large external-memory carveouts. Their DT nodes must follow the platform binding, not a generic template.

## Begin With The Processing Topology

Inventory:

- number and type of cores
- cluster and core hierarchy
- split, lockstep, single-CPU, or other supported modes
- local tightly coupled memories and shared SRAM
- boot vector and reset granularity
- interrupt controller and routing
- DMA masters and peripheral access
- address-translation units or MMUs
- power, clock, and isolation domains
- safety, secure, or boot-firmware ownership

Then identify which choices are board policy and which are fixed in silicon or by earlier firmware.

## R5 And Similar Safety Clusters

An R5 cluster can expose modes with materially different hardware:

- **lockstep**: cores execute together for fault detection and often appear as one logical remote processor
- **split**: cores execute independently and need separate firmware, memory, interrupts, and lifecycle
- **single-core or vendor-specific modes**: only a documented subset is available

Changing mode is not merely toggling `status`. It can alter TCM ownership, reset coupling, boot addresses, interrupt routing, and safety assumptions. Some platforms allow mode selection only before a lifecycle boundary or through system firmware.

Review cluster mode against:

- hardware straps and system-firmware configuration
- binding constraints on child nodes
- remoteproc driver capabilities
- carveout partitioning
- linker scripts for every core
- safety certification and fault-handling policy

Never enable two split-core nodes that reference overlapping writable memory.

## Cortex-M-Class Cores

An M-class core often has a 32-bit address view, local SRAM, and aliases for system memory. It may remain active in suspend to handle wake, sensors, or power policy.

Key questions:

- Is firmware loaded by Linux, boot ROM, or system firmware?
- Does the core see DRAM directly, through an offset, or through a translation unit?
- Which SRAM bank contains vectors and executable code?
- Who owns the mailbox and wake interrupt?
- Which supplies and clocks remain active during system sleep?
- Can Linux reset it without losing platform services?

If the M-class core is the power-management authority, modeling it as an ordinary stoppable coprocessor is unsafe.

## PRU And Real-Time I/O Subsystems

Programmable real-time units are commonly embedded in a larger subsystem containing local instruction/data RAM, shared RAM, interrupt routing, configuration registers, and direct pins or industrial interfaces. The subsystem, individual cores, and client-facing functionality can be distinct devices in the binding.

Pin ownership matters as much as memory. A Linux peripheral driver and PRU firmware cannot both drive the same pad or own the same interrupt route. Coordinate:

- pinctrl state and mux ownership
- interrupt-controller/event mappings
- shared RAM offsets
- direct peripheral windows
- core-specific firmware names
- client driver or userspace control interface

Do not expose arbitrary physical memory to userspace merely because a development PRU workflow needs fast iteration.

## DSPs And Accelerators

DSPs may have:

- large code/data carveouts
- L1/L2 SRAM with aliases
- an internal MMU
- system IOMMU attachment
- high-bandwidth DMA
- shared cache or noncoherent interconnect

Separate instruction loading from runtime data buffers. A static firmware carveout does not need to become the heap for every accelerator request. Prefer controlled DMA allocation and IOMMU mappings where the ABI permits them.

For performance failures, verify bandwidth, cache policy, page size, TLB pressure, and memory locality before increasing carveouts.

## Parent And Child Nodes

A cluster node can own shared power/reset/configuration while child core nodes own per-core resources. Whether child nodes use addressable unit names, `reg`, `ranges`, or core-index properties is binding-defined.

Watch for resource duplication:

- cluster driver and core driver controlling the same reset
- shared clock disabled when one of several cores stops
- TCM described once per alias rather than once per physical bank
- interrupt router configured by two independent owners
- one power domain reference omitted because the parent happens to be active

The DT hierarchy should match the Linux driver ownership model and the hardware's control granularity.

## Peripheral Assignment

If firmware owns a UART, SPI controller, timer, or Ethernet path:

1. prevent the corresponding Linux functional driver from binding
2. describe any remote-side mapping only as the platform binding permits
3. configure pinctrl, clocks, resets, interrupts, and firewalls under one authority
4. define ownership during remote stop and crash

`status = "disabled"` on the Linux peripheral node can prevent normal probing, but it does not grant the remote processor access or configure security hardware.

## Variant And Product Design

Put silicon topology in the SoC `.dtsi`; put package and board wiring in shared layers; put product policy such as which core runs which role in the narrowest appropriate board/product layer. Avoid editing the SoC description so one product can repurpose a core.

For cluster modes, prefer complete, reviewed overlays or includes that change the full contract together—mode, enabled cores, memory regions, firmware names, and assigned peripherals. Partial overrides create configurations that are individually plausible and jointly impossible.

## Review Matrix

| Topic | Evidence |
|---|---|
| mode | binding, straps/system firmware, runtime driver log |
| memory | physical map, remote view, linker map, no overlap |
| lifecycle | reset/power ownership and safe stop policy |
| I/O | pins, interrupts, DMA, firewall, Linux driver disabled |
| sleep | always-on resources and wake path |
| recovery | containment of core and independent DMA masters |

## Authoritative References

- [Linux remoteproc binding directory](https://github.com/torvalds/linux/tree/master/Documentation/devicetree/bindings/remoteproc)
- [TI K3 R5F remoteproc binding schema](https://github.com/torvalds/linux/blob/master/Documentation/devicetree/bindings/remoteproc/ti,k3-r5f-rproc.yaml)
- [TI PRU remoteproc binding schema](https://github.com/torvalds/linux/blob/master/Documentation/devicetree/bindings/remoteproc/ti,pru-rproc.yaml)
- [STM32 remoteproc binding schema](https://github.com/torvalds/linux/blob/master/Documentation/devicetree/bindings/remoteproc/st,stm32-rproc.yaml)
- [Linux remoteproc framework](https://docs.kernel.org/staging/remoteproc.html)

## Continue

Proceed to [Trusted Firmware, OP-TEE, And Secure-World Boundaries](trusted-firmware-op-tee-and-secure-world-boundaries.md).

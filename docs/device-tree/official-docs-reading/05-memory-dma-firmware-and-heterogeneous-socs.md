---
status: active
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# 5. Memory, DMA, Firmware, Remoteproc, And Secure Boundaries

Official sources: Linux reserved-memory/IOMMU/remoteproc bindings plus the [remoteproc framework](https://docs.kernel.org/staging/remoteproc.html)

Knowledge-guide companion: [Stage 5](knowledge-guide-companion.md#stage-5-memory-dma-firmware-remoteproc-and-secure-boundaries)

## RAM And Reservations

- [ ] **P0** DTSpec `/memory` and the FDT memory reservation block.
- [ ] **P0** Linux [`reserved-memory.yaml`](https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/tree/Documentation/devicetree/bindings/reserved-memory/reserved-memory.yaml).
- [ ] **P0** Static `reg` regions versus dynamically allocated `size`/`alignment`/`alloc-ranges` regions.
- [ ] **P0** `no-map`, `reusable`, and mutually exclusive semantics.
- [ ] **P0** `memory-region` consumer relationships and consumer-defined ordering/names.
- [ ] **P0** FDT reservation-map entries versus `/reserved-memory` nodes.
- [ ] **P1** ramoops, framebuffer, protected/shared-memory, and vendor reserved-memory schemas only when used.

## CMA And Shared DMA Pools

- [ ] **P0** [`shared-dma-pool.yaml`](https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/tree/Documentation/devicetree/bindings/reserved-memory/shared-dma-pool.yaml).
- [ ] **P0** Static carveout versus reusable pool versus default CMA intent.
- [ ] **P0** Pool addressability, device DMA masks, alignment, and IOMMU interactions.
- [ ] **P1** [DMA-BUF heaps](https://docs.kernel.org/userspace-api/dma-buf-heaps.html) when userspace depends on named reserved-memory heaps.
- [ ] **P1** Understand that allocation policy and userspace ABI are not defined only by the DT node.

## DMA Addressing And Coherency

- [ ] **P0** DTSpec `dma-ranges` and nested DMA address translation.
- [ ] **P0** `dma-coherent`/`dma-noncoherent` platform contracts where applicable.
- [ ] **P0** [DMA API HOWTO](https://docs.kernel.org/core-api/dma-api-howto.html) to understand the runtime contract DT is enabling.
- [ ] **P0** Device address width/mask, boundary, segment, and cache-coherency constraints.
- [ ] **P1** Restricted DMA pools and security-driven DMA constraints when used.

## IOMMU Topology

- [ ] **P0** Generic IOMMU binding and provider `#iommu-cells`.
- [ ] **P0** Consumer `iommus` arguments and stream/device ID origin.
- [ ] **P0** IOMMU maps/masks on buses that translate requester IDs.
- [ ] **P0** Domain attachment, bypass, reserved regions, and fault evidence in the exact driver/platform.
- [ ] **P1** [IOMMU userspace/API documentation](https://docs.kernel.org/userspace-api/iommufd.html) only for projects using those interfaces.
- [ ] **P1** Verify security assumptions against the SoC interconnect and firmware, not DT alone.

## Firmware Loading And Contracts

- [ ] **P0** [Firmware search paths and fallback](https://docs.kernel.org/driver-api/firmware/fw_search_path.html).
- [ ] **P0** Device binding properties naming firmware and their stable ABI implications.
- [ ] **P0** Exact firmware artifact identity, compatibility, load address, entry point, and ownership.
- [ ] **P0** Firmware resource-table format and which requirements are declared by firmware versus DT.
- [ ] **P1** SCMI, mailbox, syscon, and SoC firmware interfaces used by the project.

## Remoteproc And RPMsg

- [ ] **P0** [Remote Processor Framework](https://docs.kernel.org/staging/remoteproc.html).
- [ ] **P0** Exact vendor remoteproc binding: power domains, clocks, resets, IOMMUs, mailboxes, memory regions, firmware names, and cluster relationships.
- [ ] **P0** Resource table carveouts, devmem mappings, trace buffers, virtio devices, and vrings.
- [ ] **P0** Host/device/IO virtual address distinctions.
- [ ] **P0** Boot, attach, stop, crash, recovery, and shutdown ownership.
- [ ] **P0** [RPMsg framework](https://docs.kernel.org/staging/rpmsg.html), endpoint/channel naming, and userspace exposure policy.
- [ ] **P1** Remoteproc sysfs/cdev interfaces and security implications from the exact kernel.
- [ ] **P1** PRU, R5/M4, DSP, auxiliary processor, and cluster-specific bindings used by the SoC.

## Trusted And Secure Firmware

- [ ] **P0** Architecture/firmware contract for secure and non-secure memory ownership.
- [ ] **P0** OP-TEE reserved-memory and firmware nodes when present.
- [ ] **P0** PSCI, SCMI, FF-A, SMC, and other firmware interfaces only as used by the platform.
- [ ] **P0** Confirm that protected regions are absent from Linux-owned RAM or explicitly reserved.
- [ ] **P1** Understand which firmware stage may mutate memory/reservation nodes before Linux.
- [ ] **P1** Treat addresses, identities, and permissions from firmware as versioned interfaces with evidence.

## Practical Exercises

- [ ] Build a half-open interval map for RAM, kernel, initrd, FDT, bootloader, secure firmware, CMA, remote firmware, vrings, and crash logs.
- [ ] Mechanically detect overlaps and compare source DTB with final runtime reservations.
- [ ] Trace one DMA master from `dma-ranges` and `iommus` to its runtime domain and fault log.
- [ ] Trace one remote processor's DT regions against its firmware resource table.
- [ ] Start, communicate, stop, restart, and induce/recover from a controlled remote-core fault on a safe lab target.
- [ ] Explain what DT proves, what firmware proves, and what only hardware/runtime evidence can prove.

## Stage Completion

- [ ] I can label CPU physical, DMA/bus, IOVA, remote device, and firmware load addresses correctly.
- [ ] I can produce a non-overlapping ownership map across Linux, boot firmware, secure world, DMA, and remote cores.
- [ ] I can reconcile reserved-memory nodes with firmware resource tables and consumer bindings.
- [ ] I can validate remoteproc/RPMsg lifecycle and recovery without treating successful boot as memory-safety proof.


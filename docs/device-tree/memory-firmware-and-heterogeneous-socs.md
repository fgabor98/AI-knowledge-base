---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# Memory, Firmware, And Heterogeneous SoCs

Modern SoCs are not one CPU surrounded by passive peripherals. Application CPUs, real-time cores, DSPs, accelerators, and secure firmware can have different address views, cache behavior, reset domains, and owners. Device Tree must describe those fixed hardware contracts without pretending that a reserved address alone establishes safe ownership.

## Learning Outcomes

After completing this module, you should be able to:

- distinguish installed RAM, Linux-managed RAM, reserved memory, CMA, and device-private memory
- choose among static carveouts, dynamic reserved-memory allocation, shared DMA pools, and ordinary DMA allocation
- trace a buffer through CPU physical, DMA, I/O virtual, and remote-processor device addresses
- interpret `dma-ranges`, DMA coherency properties, DMA masks, and `iommus` as separate constraints
- explain how a remoteproc driver, firmware image, resource table, carveouts, and boot hardware cooperate
- separate remoteproc lifecycle control from RPMsg transport and application protocols
- model mailboxes, vrings, shared buffers, interrupts, clocks, resets, and power domains without duplicating ownership
- review PRU, R5/M4, DSP, and similar auxiliary-core descriptions against platform-specific operating modes
- identify which properties are normal-world requests rather than authority over secure firmware or secure memory
- diagnose boot, translation, coherency, messaging, crash, and restart failures with evidence from each layer

## Prerequisites

Complete [Graph Bindings And Complex Data Paths](graph-bindings-and-complex-data-paths.md). You should already be comfortable with address translation, provider-consumer relationships, power sequencing, and validation of the final DTB.

## Learning Path

1. [Memory Ownership, RAM, And Reserved Regions](memory-firmware-and-heterogeneous-socs/memory-ownership-ram-and-reserved-regions.md)
2. [CMA, Shared DMA Pools, And Static Carveouts](memory-firmware-and-heterogeneous-socs/cma-shared-dma-pools-and-static-carveouts.md)
3. [DMA Addressing, Coherency, And IOMMU Topology](memory-firmware-and-heterogeneous-socs/dma-addressing-coherency-and-iommu-topology.md)
4. [Firmware Images, Resource Tables, And Host Contracts](memory-firmware-and-heterogeneous-socs/firmware-images-resource-tables-and-host-contracts.md)
5. [Remoteproc Topology, Boot, Stop, And Recovery](memory-firmware-and-heterogeneous-socs/remoteproc-topology-boot-stop-and-recovery.md)
6. [RPMsg, Mailboxes, Virtqueues, And Shared Memory](memory-firmware-and-heterogeneous-socs/rpmsg-mailboxes-virtqueues-and-shared-memory.md)
7. [PRU, R5/M4, DSP, And Cluster Modeling](memory-firmware-and-heterogeneous-socs/pru-r5-m4-dsp-and-cluster-modeling.md)
8. [Trusted Firmware, OP-TEE, And Secure-World Boundaries](memory-firmware-and-heterogeneous-socs/trusted-firmware-op-tee-and-secure-world-boundaries.md)
9. [Heterogeneous SoC Integration And Recovery Lab](memory-firmware-and-heterogeneous-socs/heterogeneous-soc-integration-and-recovery-lab.md)

## Four Contracts, Not One

Keep these questions separate:

| Contract | Question |
|---|---|
| ownership | Which execution domain may use or modify the region or device now? |
| addressing | Which numeric address does each bus master place on its interface? |
| coherency | When do CPU and device observers see each other's writes? |
| lifecycle | Who may load, authenticate, start, stop, reset, or recover the processor? |

A region can be addressable but not owned, owned but not coherent, or visible only after an IOMMU mapping. A remote core can be running while Linux is not its lifecycle owner. Treating these as one “memory configuration” is the source of subtle corruption and security bugs.

## Description Versus Negotiation

DT should describe stable hardware and platform integration: memory windows, wiring, IOMMU attachment, mailboxes, reset topology, and permitted operating modes. A firmware resource table can describe resources requested by one firmware build. Runtime protocols negotiate services and buffer use. None of these sources should silently contradict the others.

Before changing a DTS, write an ownership-and-address matrix:

```text
resource       owner at boot   Linux view      remote view     transition authority
firmware text  boot firmware   phys 0x...      device 0x...     remoteproc or secure FW
vrings         Linux/rproc     DMA 0x...       device 0x...     virtio/rpmsg
data buffers   negotiated      mapped pages    IOVA/device addr DMA API + protocol
trace buffer   remote writer   host mapping    device 0x...     remoteproc debug path
```

If any cell is unknown, the DTS is not yet the right place to guess.

## Completion Check

You are ready for [U-Boot And Bootloader Device Tree](u-boot-and-bootloader-device-tree.md) when you can:

- account for every byte excluded from Linux and name its owner, purpose, and lifecycle
- translate one shared buffer across CPU, DMA/IOMMU, and remote-core address spaces
- justify each `no-map`, `reusable`, `shared-dma-pool`, coherency, and IOMMU decision
- reconcile DT carveouts with ELF load segments and the remoteproc resource table
- derive safe start, stop, crash recovery, suspend, and warm-boot behavior
- distinguish a remote processor that failed to boot from an RPMsg service that failed to appear
- explain how secure firmware constrains normal-world DT without exposing secure resources
- diagnose a schema-valid heterogeneous system that corrupts memory only under load

## Authoritative References

- [Linux reserved-memory binding](https://github.com/torvalds/linux/blob/master/Documentation/devicetree/bindings/reserved-memory/reserved-memory.txt)
- [Upstream shared DMA pool binding schema](https://github.com/devicetree-org/dt-schema/blob/main/dtschema/schemas/reserved-memory/shared-dma-pool.yaml)
- [Linux remoteproc framework](https://docs.kernel.org/staging/remoteproc.html)
- [Linux RPMsg framework](https://docs.kernel.org/staging/rpmsg.html)
- [Linux DMA API guide](https://docs.kernel.org/core-api/dma-api-howto.html)

## Related Topics

- [Standard Nodes And Properties](standard-nodes-and-properties.md)
- [Addressing And Bus Modeling](addressing-and-bus-modeling.md)
- [Provider-Consumer Relationships](provider-consumer-relationships.md)
- [Clocks, Resets, Regulators, And Power](clocks-resets-regulators-and-power.md)
- [Security And Production Lifecycle](security-and-production-lifecycle.md)

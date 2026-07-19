---
status: draft
reviewed: false
domain: device-tree
difficulty: intermediate
last_reviewed: null
---

# Standard Nodes And Properties

Standard nodes form the platform-wide contract between hardware, firmware, and the operating system. They identify the machine, describe processors and RAM, carry boot-time choices, reserve memory, and express relationships that do not follow the parent-child tree.

This module focuses on what each construct means, who may modify it, and how to prove what the kernel actually received.

## Learning Outcomes

After completing this module, you should be able to:

- choose correct root `compatible` and `model` values
- reason about node availability through `status`, including disabled parents
- distinguish aliases, labels, paths, and Linux device numbering
- decode `/cpus` and `/memory` using their governing cell counts
- separate static hardware description from firmware-owned `/chosen` data
- choose between `/memreserve/` and `/reserved-memory`
- decode `interrupts-extended` and interrupt nexus mappings
- explain what `dma-coherent`, `iommus`, `phys`, and `phy-names` assert
- audit the final DTB rather than trusting one DTS input

## Prerequisites

Complete [Provider-Consumer Relationships](provider-consumer-relationships.md). This module assumes that you can resolve phandles and decode provider-specific specifiers.

## Learning Path

1. [Root Identity, Availability, And Aliases](standard-nodes-and-properties/root-identity-availability-and-aliases.md)
2. [CPUs, Topology, And Memory](standard-nodes-and-properties/cpus-topology-and-memory.md)
3. [Chosen And Boot Handoff](standard-nodes-and-properties/chosen-and-boot-handoff.md)
4. [Reserved Memory](standard-nodes-and-properties/reserved-memory.md)
5. [Cross-Cutting Standard Relationships](standard-nodes-and-properties/cross-cutting-standard-relationships.md)
6. [Standard Platform Tree Lab](standard-nodes-and-properties/standard-platform-tree-lab.md)

## Three Ownership Zones

| Zone | Typical content | Usual owner | Review question |
|---|---|---|---|
| Platform identity | root `compatible`, `model`, aliases | board/BSP source | Does it identify the actual product without inventing compatibility? |
| Hardware inventory | `/cpus`, `/memory`, device `status` | SoC/board source, sometimes corrected by firmware | Do addresses, sizes, and availability match this unit? |
| Boot handoff | `/chosen`, reservations, initrd and console data | firmware/bootloader | What is present in the final DTB, not merely the source tree? |

The boundary is deliberate. A bootloader may fix RAM size or choose a console, but that does not make `/chosen` an appropriate place for permanent hardware wiring.

## A Useful Audit Order

When a board boots incorrectly, inspect the delivered tree in this order:

1. Confirm the DTB artifact and root `compatible`.
2. Check ancestor and device `status` values.
3. Verify `/memory`, `/reserved-memory`, and the memory reservation block.
4. Inspect `/chosen` for console, command line, and initrd handoff.
5. Resolve relationship properties through their providers and bindings.
6. Compare the runtime tree with the build artifact and bootloader modifications.

This order catches platform-selection and firmware-mutation problems before driver-level debugging consumes time.

## Completion Check

You are ready for [Addressing And Bus Modeling](addressing-and-bus-modeling.md) when you can explain:

- why `model` must not be used for driver matching
- why `status = "okay"` cannot override a disabled ancestor
- why an alias is neither a source label nor a stable Linux ABI by itself
- which node supplies the cell widths for CPU and memory `reg` values
- why `/chosen` seen at runtime can legitimately differ from the checked-in DTS
- when a consumer needs `memory-region` rather than only a reservation entry
- how every field of one `interrupt-map` row gets its width

## Authoritative References

- [Devicetree Specification: standard nodes](https://devicetree-specification.readthedocs.io/en/stable/devicenodes.html)
- [Devicetree Specification: properties and interrupt mappings](https://devicetree-specification.readthedocs.io/en/stable/devicetree-basics.html)
- [Linux Devicetree usage model](https://docs.kernel.org/devicetree/usage-model.html)
- [Linux Devicetree bindings](https://docs.kernel.org/devicetree/bindings/index.html)

## Related Topics

- [Syntax, Values, And Source Composition](syntax-values-and-source-composition.md)
- [Provider-Consumer Relationships](provider-consumer-relationships.md)
- [Boot-Time Mutation And Ownership](boot-time-mutation-and-ownership.md)

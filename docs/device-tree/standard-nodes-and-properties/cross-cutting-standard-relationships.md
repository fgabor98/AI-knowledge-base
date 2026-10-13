---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# Cross-Cutting Standard Relationships

Several standard properties describe dependencies or address-space crossings outside the parent-child hierarchy. Their syntax is compact, but each entry must be decoded from provider and nexus cell-count contracts.

## `interrupts-extended`

`interrupts-extended` puts an interrupt-parent phandle directly in every entry, so entries from different parents can have different widths and meanings. The BeagleBoard-X15 UART3 functional and wake-up interrupts (`am57xx-beagle-x15-common.dtsi`):

```dts
&uart3 {
        interrupts-extended = <&crossbar_mpu GIC_SPI 69 IRQ_TYPE_LEVEL_HIGH>,
                              <&dra7_pmx_core 0x3f8>;
};
```

Decode it with the [standard phandle-array algorithm](../provider-consumer-relationships.md#the-core-decoding-algorithm), using each provider's `#interrupt-cells` (three for the IRQ crossbar, one for the pad-wakeup controller). If both `interrupts` and `interrupts-extended` exist, the specification gives `interrupts-extended` precedence, but bindings should normally avoid such ambiguity.

## Interrupt Nexus Mapping

An interrupt nexus translates a child's interrupt identity into a parent domain. A row in `interrupt-map` contains:

```text
child unit address
+ child interrupt specifier
+ parent phandle
+ parent unit address
+ parent interrupt specifier
```

Every field width comes from a node:

| Row field | Width source |
|---|---|
| child unit address | nexus `#address-cells` |
| child interrupt specifier | nexus `#interrupt-cells` |
| parent unit address | parent interrupt controller `#address-cells` |
| parent interrupt specifier | parent interrupt controller `#interrupt-cells` |

The Arm Juno PCIe host bridge is a real nexus for legacy INTx (`arch/arm64/boot/dts/arm/juno-base.dtsi`, trimmed):

```dts
gic: interrupt-controller@2c010000 {
        #address-cells = <1>;
        #interrupt-cells = <3>;
        /* ... */
};

pcie_ctlr: pcie@40000000 {
        #address-cells = <3>;
        #interrupt-cells = <1>;
        interrupt-map-mask = <0 0 0 7>;
        interrupt-map = <0 0 0 1 &gic 0 GIC_SPI 136 IRQ_TYPE_LEVEL_HIGH>,
                        <0 0 0 2 &gic 0 GIC_SPI 137 IRQ_TYPE_LEVEL_HIGH>,
                        <0 0 0 3 &gic 0 GIC_SPI 138 IRQ_TYPE_LEVEL_HIGH>,
                        <0 0 0 4 &gic 0 GIC_SPI 139 IRQ_TYPE_LEVEL_HIGH>;
        /* ... */
};
```

Each row is 3 child-address cells (PCI), 1 child interrupt cell (INTA–INTD as 1–4), the GIC phandle, 1 parent address cell (`0`, because the GIC node has `#address-cells = <1>`), and 3 GIC specifier cells: nine cells after expansion.

The mask has the combined width of the child unit address and child interrupt specifier. Matching applies the mask to the candidate and row key. Here `<0 0 0 7>` ignores the device address completely and matches only the pin, so every device's INTA goes to SPI 136; there is no per-slot swizzling. A mask that ignores address bits makes one mapping apply to several children; an overly broad mask can silently route an interrupt to the wrong parent input.

Never split rows by visual grouping alone. Walk the flat cell stream using the governing counts, especially when different rows target parents with different widths.

## `dma-coherent`

`dma-coherent` is an empty boolean property declaring that the device's DMA transactions are coherent with CPU caches under the platform's coherency model. The same SDHCI IP shows why it is an integration fact. On AM65x it is marked coherent (`k3-am65-main.dtsi`):

```dts
sdhci1: mmc@4fa0000 {
        compatible = "ti,am654-sdhci-5.1";
        /* ... */
        dma-coherent;
};
```

while the AM64x `sdhci0` and `sdhci1` nodes in `k3-am64-main.dtsi` have no `dma-coherent`, and AM65x also places `dma-coherent` on the whole `main_navss` bus node.

This is a hardware fact, not a performance toggle. Adding it to hide missing cache maintenance can cause data corruption. Omitting it on coherent hardware can impose unnecessary synchronization and restrict optimizations.

Some bus bindings allow coherency to be inherited from a parent. Check the architecture and bus binding before relying on inheritance. Coherency also does not remove the need for the DMA API: address translation, barriers, ownership transitions, and device-specific ordering still matter.

## `iommus`, `phys`, And `phy-names`

These are ordinary phandle arrays decoded through `#iommu-cells` and `#phy-cells`; see [DMA, IOMMU, PHY, And Regulator Dependencies](../provider-consumer-relationships/dma-iommu-phy-and-regulator-dependencies.md). Two standard-level cautions apply:

- An IOMMU relationship does not prove that isolation is enabled at boot, that the correct domain is attached, or that all DMA paths pass through that IOMMU. Validate firmware bypass state, requester identifiers, grouping, and runtime domain attachment. Bus-level `iommu-map` translations belong with the bus binding; do not replace them casually with per-device `iommus`.
- A PHY relationship describes a logical hardware resource. It does not replace pinctrl, clocks, resets, regulators, connector graph links, or protocol configuration when those are separately required.

## A Unified Review Method

For each cross-cutting property, decode every entry in the final DTB with the [decoding worksheet](../provider-consumer-relationships/specifier-decoding-and-resource-names.md#a-mechanical-decoding-worksheet), then:

1. Check nexus translations along the actual path.
2. Verify that firmware and kernel drivers enable the described hardware behavior.
3. Inspect runtime subsystem state rather than equating DT presence with enforcement.

## Failure Patterns Worth Recognizing

- Treating a mixed-provider array as fixed-width tuples.
- Reading interrupt flag values using the wrong controller binding.
- Forgetting the parent unit-address portion of an `interrupt-map` row.
- Using a mask whose width does not match the child key.
- Adding `dma-coherent` as a software workaround.
- Assuming an IOMMU phandle guarantees security isolation.
- Reordering `phys` without reordering `phy-names`.
- Copying provider argument values between unrelated controllers.

## Authoritative References

- [Devicetree Specification: interrupt mappings and standard properties](https://devicetree-specification.readthedocs.io/en/stable/devicetree-basics.html)
- [Linux IRQ domain documentation](https://docs.kernel.org/core-api/irq/irq-domain.html)
- [Linux DMA API HOWTO](https://docs.kernel.org/core-api/dma-api-howto.html)
- [Linux generic PHY framework](https://docs.kernel.org/driver-api/phy/phy.html)
- [Linux DeviceTree kernel API](https://docs.kernel.org/devicetree/kernel-api.html)

## Next Step

Apply the model in the [Standard Platform Tree Lab](standard-platform-tree-lab.md).

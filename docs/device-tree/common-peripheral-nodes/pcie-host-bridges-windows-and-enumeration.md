---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# PCIe Host Bridges, Windows, And Enumeration

A PCIe host-bridge node describes the root complex's CPU-facing resources and the translation into PCI address spaces. Downstream functions generally enumerate through PCI configuration space; DT is not a static inventory of every plug-in endpoint.

## Host Bridge Skeleton

```dts
pcie@80000000 {
        compatible = "vendor,soc-pcie";
        device_type = "pci";
        reg = <0x0 0x80000000 0x0 0x100000>;
        bus-range = <0x00 0xff>;
        #address-cells = <3>;
        #size-cells = <2>;
        ranges = < /* PCI space -> CPU space windows */ >;
        dma-ranges = < /* PCI DMA -> system memory */ >;
        msi-parent = <&its>;
        status = "okay";
};
```

The three-cell PCI child address includes flags plus bus/device/function and register information. `ranges` describes outbound CPU access to PCI I/O or memory space. `dma-ranges` describes inbound DMA translation. Their encodings are not ordinary simple-bus tuples; review them against the generic host-bridge schema and platform address map.

## Root-Complex Resources

Real controllers commonly add:

- DBI, configuration, application, and ATU register regions
- core/auxiliary/reference clocks and resets
- PHYs and lane counts
- regulator supplies
- PERST#, CLKREQ#, WAKE#, and presence signals
- legacy INTx mapping and MSI/MSI-X controller relationships
- IOMMU stream/requester-ID maps

Property names and reset polarity come from the controller binding. PERST# may be owned by the host driver, a slot controller, firmware, or a GPIO-backed reset property. Duplicate control produces timing races.

## Link Training Before Enumeration

Enumeration starts only after power, reference clock, reset timing, PHY initialization, and link training succeed. If no endpoint appears, inspect link state before debugging PCI IDs or endpoint drivers.

Validate lane mapping, bifurcation, generation limit, reference-clock architecture, signal integrity, and endpoint power sequencing. For removable slots, test empty boot, insertion if supported, surprise removal, and power faults.

Properties such as `num-lanes` and `max-link-speed` should reflect routed/supported hardware constraints. Reducing link speed is a useful diagnostic but not a substitute for resolving marginal layout or equalization.

## Interrupts And Requester IDs

MSI/MSI-X messages must reach the correct interrupt domain. `msi-parent` suits simple relationships; `msi-map` translates PCI requester IDs where required. Similarly, `iommu-map` translates requester IDs to IOMMU stream IDs. Masks and ranges must cover every possible BDF created by the topology.

An endpoint can enumerate and perform MMIO while DMA fails because the IOMMU map is wrong. MSI failure can fall back to legacy INTx on some devices and conceal the mapping error. Inspect interrupt mode and IOMMU groups explicitly.

## Fixed Downstream Devices

DT child nodes under a PCI host or bridge are reserved for cases needing firmware-described properties not discoverable from PCI configuration space, or for platform-specific relationships defined by bindings. The child's unit address encodes its BDF. Do not list ordinary cards merely to force driver loading.

## Runtime Diagnosis

```sh
lspci -nnvv
lspci -t
find /sys/bus/pci/devices -maxdepth 2 -name iommu_group -o -name msi_irqs
dmesg | grep -Ei 'pci|pcie|link|aer|msi|iommu'
```

Use AER counters, link capability/status, negotiated width/speed, resource assignments, and `/proc/iomem`. Compare cold and warm boots; inherited firmware link state often hides reset or clock defects.

## Authoritative References

- [Linux PCI driver API](https://docs.kernel.org/driver-api/pci/pci.html)
- [Devicetree kernel PCI mapping APIs](https://docs.kernel.org/devicetree/kernel-api.html)
- [Linux generic PCI host-bridge schema](https://github.com/torvalds/linux/blob/master/Documentation/devicetree/bindings/pci/pci-host-bridge.yaml)

## Continue

Proceed to [MMC, SD, SDIO, And eMMC](mmc-sd-sdio-and-emmc.md).

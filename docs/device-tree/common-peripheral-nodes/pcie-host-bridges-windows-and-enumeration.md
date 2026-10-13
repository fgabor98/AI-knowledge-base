---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# PCIe Host Bridges, Windows, And Enumeration

A PCIe host-bridge node describes the root complex's CPU-facing resources and the translation into PCI address spaces. Downstream functions generally enumerate through PCI configuration space; DT is not a static inventory of every plug-in endpoint.

## A Real Host Bridge

The AM64x root complex, from `k3-am64-main.dtsi`:

```dts
pcie0_rc: pcie@f102000 {
        compatible = "ti,am64-pcie-host", "ti,j721e-pcie-host";
        reg = <0x00 0x0f102000 0x00 0x1000>,
              <0x00 0x0f100000 0x00 0x400>,
              <0x00 0x0d000000 0x00 0x00800000>,
              <0x06 0x00000000 0x00 0x00001000>; /* ECAM (4 KB) */
        reg-names = "intd_cfg", "user_cfg", "reg", "cfg";
        interrupt-names = "link_state";
        interrupts = <GIC_SPI 203 IRQ_TYPE_EDGE_RISING>;
        device_type = "pci";
        ti,syscon-pcie-ctrl = <&pcie0_ctrl 0x0>;
        max-link-speed = <2>;
        num-lanes = <1>;
        power-domains = <&k3_pds 114 TI_SCI_PD_EXCLUSIVE>;
        clocks = <&k3_clks 114 0>, <&serdes0 CDNS_TORRENT_REFCLK_DRIVER>;
        clock-names = "fck", "pcie_refclk";
        #address-cells = <3>;
        #size-cells = <2>;
        bus-range = <0x0 0xff>;
        cdns,no-bar-match-nbits = <64>;
        vendor-id = <0x104c>;
        device-id = <0xb010>;
        msi-map = <0x0 &gic_its 0x0 0x10000>;
        ranges = <0x01000000 0x00 0x00001000 0x06 0x00001000 0x00 0x00100000>, /* IO (1 MB) */
                 <0x02000000 0x00 0x00101000 0x06 0x00101000 0x00 0xffeff000>; /* 32-bit Non-Prefetchable MEM (4 GB - 1 MB - 4 KB) */
        dma-ranges = <0x02000000 0x0 0x0 0x0 0x0 0x00000010 0x0>;
        status = "disabled";
};
```

The three-cell child address, `ranges`, `dma-ranges`, and the 4 KiB configuration window are decoded in [PCI Host Bridges, Address Windows, And Interrupts](../addressing-and-bus-modeling/pci-host-bridges-address-windows-and-interrupts.md). This page focuses on the controller integration around them.

## Root-Complex Resources

The SoC node above already shows most categories a real controller needs:

- four register regions, named because the driver looks them up by name
- a functional clock and the reference clock, which here comes from the SERDES (`CDNS_TORRENT_REFCLK_DRIVER`)
- a TI-SCI power domain and a syscon handle into the PCIe control MMR
- link limits: Gen2 and one lane
- an MSI relationship: `msi-map` sends requester IDs `0x0`–`0xffff` to the GIC ITS unchanged
- the link-state interrupt; there is no `interrupt-map` here, so legacy INTx is not described

The AM642 EVM adds the board half (`k3-am642-evm.dts`):

```dts
&serdes_ln_ctrl {
        idle-states = <AM64_SERDES0_LANE0_PCIE0>;
};

&serdes0 {
        serdes0_pcie_link: phy@0 {
                reg = <0>;
                cdns,num-lanes = <1>;
                #phy-cells = <0>;
                cdns,phy-type = <PHY_TYPE_PCIE>;
                resets = <&serdes_wiz0 1>;
        };
};

&pcie0_rc {
        status = "okay";
        reset-gpios = <&exp1 5 GPIO_ACTIVE_HIGH>;
        phys = <&serdes0_pcie_link>;
        phy-names = "pcie-phy";
        num-lanes = <1>;
};
```

The single SERDES lane is muxed to PCIe0; the AM642 SK muxes it to USB instead, which is why its PCIe stays disabled. PERST# comes through the I²C expander `exp1`, so the root complex also depends on `main_i2c1`.

The reset polarity needs care. PERST# is active low on the wire, yet the board says `GPIO_ACTIVE_HIGH`. `pci-j721e` requests the line `GPIOD_OUT_LOW`, waits the 100 ms `PCIE_T_PVPERL_MS`, and then sets it to logical 1 to *release* reset, the reverse of the usual reset-GPIO convention. The Kontron SA67 board file says so explicitly: “This is low active, but the driver itself is broken and already inverts the logic.” Writing `GPIO_ACTIVE_LOW` here would hold the endpoint in reset whenever the driver thinks it released it. Always check the driver as well as the binding, and never “fix” such a flag in one board without fixing the driver and every board together.

## Link Training Before Enumeration

Enumeration starts only after power, reference clock, reset timing, PHY initialization, and link training succeed. If no endpoint appears, inspect link state before debugging PCI IDs or endpoint drivers.

Validate lane mapping, bifurcation, generation limit, reference-clock architecture, signal integrity, and endpoint power sequencing. For removable slots, test empty boot, insertion if supported, surprise removal, and power faults.

Properties such as `num-lanes` and `max-link-speed` should reflect routed/supported hardware constraints. Reducing link speed is a useful diagnostic but not a substitute for resolving marginal layout or equalization.

## Interrupts And Requester IDs

`msi-map` and `iommu-map` masks and ranges must cover every BDF the topology can create. An endpoint can enumerate and perform MMIO while DMA fails because the IOMMU map is wrong, and MSI failure can fall back to legacy INTx and conceal the mapping error. Inspect interrupt mode and IOMMU groups explicitly.

Fixed DT child nodes under a host or bridge are only for firmware-described properties that PCI configuration space cannot provide. Do not list ordinary cards merely to force driver loading.

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

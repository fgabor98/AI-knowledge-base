---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# DMA, IOMMU, PHY, And Regulator Dependencies

These relationships all connect a device to external services, but their specifiers answer very different hardware questions. Decode each through its own subsystem binding.

## DMA Channels

A DMA controller declares `#dma-cells`. AM64x has two K3 DMA engines with different specifier shapes (`k3-am64-main.dtsi`):

```dts
main_pktdma: dma-controller@485c0000 {
        compatible = "ti,am64-dmss-pktdma";
        #dma-cells = <2>;
};

main_bcdma: dma-controller@485c0100 {
        compatible = "ti,am64-dmss-bcdma";
        #dma-cells = <3>;
};

main_spi0: spi@20100000 {
        dmas = <&main_pktdma 0xc300 0>, <&main_pktdma 0x4300 0>;
        dma-names = "tx0", "rx0";
};
```

The `k3-pktdma.yaml` binding defines the first cell as the PSI-L thread ID of the peripheral end—destination (TX) threads are `0x8000`–`0xffff`, source (RX) threads `0`–`0x7fff`—and the second as the ASEL address-select value. `0xc300` is therefore McSPI0's TX thread, not a Linux DMA channel number. The ADC on the AM642 EVM uses BCDMA instead, whose three cells start with a channel-type selector: `dmas = <&main_bcdma 0 0x440f 0>, <&main_bcdma 0 0x4410 0>;`.

The McSPI driver builds the names `"tx0"`/`"rx0"` and calls `dma_request_chan()`, which maps through `dma-names` to one specifier. Availability still depends on the DMA provider driver, channel registration, and TI-SCI resource-manager ranges.

## IOMMU Relationships

An IOMMU provider declares `#iommu-cells`. AM64x has no system IOMMU, so this example comes from Arm's Juno board (`arch/arm64/boot/dts/arm/juno-base.dtsi`):

```dts
smmu_hdlcd0: iommu@7fb20000 {
        compatible = "arm,mmu-401", "arm,smmu-v1";
        reg = <0x0 0x7fb20000 0x0 0x10000>;
        #iommu-cells = <1>;
        /* ... */
};

hdlcd@7ff60000 {
        compatible = "arm,hdlcd";
        iommus = <&smmu_hdlcd0 0>;
        /* ... */
};
```

For the Arm SMMU binding the argument is a stream ID. It describes how transactions from this device are recognized by the IOMMU; it is not a virtual address or page-table address. The same file gives the Juno DMA-330 nine entries, `<&smmu_dma 0>` through `<&smmu_dma 8>`, one per channel stream.

IOMMU topology can also be expressed through bus-level maps: the Juno PCIe host uses `iommu-map = <0x0 &smmu_pcie 0x0 0x1>;`. That translation model is covered with bus mappings rather than treated as a direct phandle list.

An incorrect IOMMU relationship can present as DMA faults, timeouts, inaccessible memory, or a device that probes successfully but cannot transfer data.

## PHY Relationships

A generic PHY provider advertises `#phy-cells`. Two AM64x examples show zero and one argument cells:

```dts
/* k3-am64-main.dtsi: CPSW port interface-mode selector */
phy_gmii_sel: phy@4044 {
        compatible = "ti,am654-phy-gmii-sel";
        #phy-cells = <1>;
};

cpsw_port1: port@1 {
        phys = <&phy_gmii_sel 1>;            /* argument: CPSW port number */
};

/* k3-am642-sk.dts: SERDES lane configured for USB3 */
serdes0_usb_link: phy@0 {
        #phy-cells = <0>;
        cdns,phy-type = <PHY_TYPE_USB3>;
};

&usb0 {
        phys = <&serdes0_usb_link>;
        phy-names = "cdns3,usb3-phy";
};
```

The generic PHY framework resolves the name, asks the provider to translate the specifier, and gives the consumer a PHY handle.

PHY is an overloaded hardware word. The DP83867 Ethernet PHYs on the SK are MDIO bus children (`cpsw3g_phy0: ethernet-phy@0`) referenced through `phy-handle`, not through the generic `phys` property. Follow the consumer binding and subsystem model.

## Regulator Supplies

Regulator relationships use named supply properties. The AM642 SK SD card slot (`k3-am642-sk.dts`):

```dts
vdd_mmc1: regulator-2 {
        /* TPS2051BD */
        compatible = "regulator-fixed";
        regulator-name = "vdd_mmc1";
        regulator-min-microvolt = <3300000>;
        regulator-max-microvolt = <3300000>;
        regulator-boot-on;
        enable-active-high;
        vin-supply = <&vcc_3v3_sys>;
        gpio = <&exp1 3 GPIO_ACTIVE_HIGH>;
};

&sdhci1 {
        vmmc-supply = <&vdd_mmc1>;
};
```

The MMC binding defines the supply name (`vmmc`), and the MMC core requests it with:

```c
/* drivers/mmc/core/regulator.c */
mmc->supply.vmmc = devm_regulator_get_optional(dev, "vmmc");
```

Unlike clocks or PHYs, ordinary regulator supply references do not use a generic `#regulator-cells` argument convention. The phandle identifies the supplying regulator. Voltage and current constraints belong to regulator bindings and board policy, not extra cells after the phandle.

One consumer can have several independently named rails. The TLV320AIC3106 codec on the AM62x SK (`k3-am62x-sk-common.dtsi`):

```dts
tlv320aic3106: audio-codec@1b {
        compatible = "ti,tlv320aic3106";
        AVDD-supply = <&vcc_3v3_sys>;
        IOVDD-supply = <&vcc_3v3_sys>;
        DRVDD-supply = <&vcc_3v3_sys>;
};
```

These are separate properties rather than one list plus `regulator-names`, and the stems—uppercase here—come from that codec's binding.

## Provider Chains

Providers can consume other providers. The SK SD-card supply chain is:

```text
sdhci1
  -> vmmc-supply -> vdd_mmc1 (TPS2051BD load switch)
                      -> gpio -> exp1 line 3 (PCA9538 on main_i2c1)
                      -> vin-supply -> vcc_3v3_sys (LP8733 output)
                                         -> vin-supply -> vusb_main (USB 5V input)
```

The load switch depends on an I2C GPIO expander, which depends on an I2C controller, its pins, clock, and power domain. Debugging must walk the chain until every supplier is available.

## `#*-cells` Is A Pattern, Not A Universal Schema

Many subsystems use the phandle-plus-arguments convention, but property names and semantics remain subsystem-specific. Do not invent `#foo-cells` and `foos` without a reviewed binding. Do not assume every dependency should be collapsed into one generic provider tuple.

The consumer binding may constrain a provider more narrowly than the provider's generic schema. The Cadence USB binding (`cdns,usb3.yaml`) accepts at most two `phys` entries named `cdns3,usb2-phy` and `cdns3,usb3-phy`, whatever the PHY provider exports.

## Failure Signatures

| Relationship | Typical failure evidence |
|---|---|
| DMA | channel request failure, timeout, no transfer completion |
| IOMMU | translation fault, stream-ID fault, mapping failure |
| PHY | PHY lookup or initialization failure, no link/signaling |
| regulator | supply lookup failure, deferred probe, voltage/enable error |

These are starting points, not proofs. A malformed clock can make a DMA controller unavailable; a missing regulator can make a PHY fail; the first visible error may be downstream from the root supplier.

## Debugging Method

For every link, decode the runtime entry with the [decoding worksheet](specifier-decoding-and-resource-names.md#a-mechanical-decoding-worksheet), then:

1. verify the provider's own ancestors, clocks, resets, supplies, and domains
2. confirm provider driver configuration and registration
3. inspect subsystem-specific runtime diagnostics
4. follow deferral to the earliest unavailable supplier

## Common Errors

- Treating a DMA request ID as a Linux channel index.
- Treating an IOMMU stream ID as an address.
- Using generic `phys` for a subsystem whose binding models a bus child instead.
- Adding argument cells to a regulator supply phandle.
- Looking for `regulator-names` instead of binding-defined `*-supply` properties.
- Debugging the final consumer while its provider's own regulator is missing.
- Assuming successful probe proves DMA or IOMMU traffic is correct.

## Exercises

1. Map `dma-names = "tx0", "rx0"` on `main_spi0` to its two PKTDMA entries and identify which is a destination thread.
2. Explain what evidence is needed to call an IOMMU argument a stream ID.
3. Distinguish generic PHY references from an MDIO Ethernet PHY child.
4. Translate the MMC core's request name `vqmmc` into the expected supply property.
5. Draw a provider chain for a USB controller, PHY, clock, reset, regulator, and power domain.

## References And Next Step

- [Linux DMA Engine client guide](https://docs.kernel.org/driver-api/dmaengine/client.html)
- [Linux Generic PHY Framework](https://docs.kernel.org/driver-api/phy/phy.html)
- [Linux regulator consumer interface](https://docs.kernel.org/power/regulator/consumer.html)
- [Linux device links](https://docs.kernel.org/driver-api/device_link.html)
- [Linux Devicetree bindings](https://docs.kernel.org/devicetree/bindings/index.html)

Continue with the [Provider-Consumer Tracing Lab](provider-consumer-tracing-lab.md).

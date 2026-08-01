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

A DMA controller declares `#dma-cells`:

```dts
dma0: dma-controller@d000 {
        compatible = "example,trainer-dma";
        #dma-cells = <2>;
};

serial@1000 {
        dmas = <&dma0 4 0>, <&dma0 5 0>;
        dma-names = "rx", "tx";
};
```

The provider binding may define request line, channel, direction capability, peripheral ID, or flags. Do not infer `4` and `5` to be Linux DMA channel numbers.

The driver commonly requests `dma_request_chan(dev, "rx")`, which maps through `dma-names` to one specifier. Availability still depends on the DMA provider driver, channel registration, and request routing.

## IOMMU Relationships

An IOMMU provider declares `#iommu-cells`:

```dts
iommu0: iommu@e000 {
        compatible = "example,trainer-iommu";
        #iommu-cells = <1>;
};

accelerator@4000 {
        iommus = <&iommu0 0x42>;
};
```

The argument might be a stream ID, requester ID, context selector, or another provider-defined identifier. It describes how transactions from this device are recognized by the IOMMU; it is not a virtual address or page-table address.

IOMMU topology can also be expressed through bus-level maps such as `iommu-map`. That translation model is covered with bus mappings rather than treated as a direct phandle list.

An incorrect IOMMU relationship can present as DMA faults, timeouts, inaccessible memory, or a device that probes successfully but cannot transfer data.

## PHY Relationships

A generic PHY provider advertises `#phy-cells`:

```dts
usb_phy: phy@f000 {
        compatible = "example,trainer-usb-phy";
        #phy-cells = <1>;
};

usb@5000 {
        phys = <&usb_phy 0>;
        phy-names = "usb";
};
```

The argument may select one lane, port, protocol, or instance. The generic PHY framework resolves the name, asks the provider to translate the specifier, and gives the consumer a PHY handle.

PHY is an overloaded hardware word. Ethernet PHYs on MDIO commonly use bus child nodes and are not necessarily represented through the generic `phys` property. Follow the consumer binding and subsystem model.

## Regulator Supplies

Regulator relationships use named supply properties:

```dts
reg_3v3: regulator-3v3 {
        compatible = "regulator-fixed";
        regulator-name = "board-3v3";
        regulator-min-microvolt = <3300000>;
        regulator-max-microvolt = <3300000>;
};

sensor@48 {
        vdd-supply = <&reg_3v3>;
};
```

The consumer binding defines the supply name (`vdd` here), and Linux drivers commonly request it with:

```c
vdd = devm_regulator_get(dev, "vdd");
```

Unlike clocks or PHYs, ordinary regulator supply references do not use a generic `#regulator-cells` argument convention. The phandle identifies the supplying regulator. Voltage and current constraints belong to regulator bindings and board policy, not extra cells after the phandle.

One consumer can have several independently named rails:

```dts
avdd-supply = <&reg_1v8_analog>;
dvdd-supply = <&reg_1v2_core>;
iovdd-supply = <&reg_3v3_io>;
```

These are separate properties rather than one list plus `regulator-names`.

## Provider Chains

Providers can consume other providers:

```text
sensor
  -> vdd-supply -> load-switch regulator
                      -> vin-supply -> PMIC regulator
                                           -> parent PMIC supply
```

Similarly, a PHY can consume clocks, resets, and regulators; a DMA controller can belong to a power domain and use an IOMMU. Debugging must walk the chain until every supplier is available.

## `#*-cells` Is A Pattern, Not A Universal Schema

Many subsystems use the phandle-plus-arguments convention, but property names and semantics remain subsystem-specific. Do not invent `#foo-cells` and `foos` without a reviewed binding. Do not assume every dependency should be collapsed into one generic provider tuple.

The consumer binding may constrain a provider more narrowly than the provider's generic schema. For example, one USB controller can require exactly two PHY entries named `usb2` and `usb3`, even though the PHY provider exports many ports.

## Failure Signatures

| Relationship | Typical failure evidence |
|---|---|
| DMA | channel request failure, timeout, no transfer completion |
| IOMMU | translation fault, stream-ID fault, mapping failure |
| PHY | PHY lookup or initialization failure, no link/signaling |
| regulator | supply lookup failure, deferred probe, voltage/enable error |

These are starting points, not proofs. A malformed clock can make a DMA controller unavailable; a missing regulator can make a PHY fail; the first visible error may be downstream from the root supplier.

## Debugging Method

For every link:

1. verify the consumer property in the runtime tree
2. read the consumer binding for names and cardinality
3. resolve the provider phandle
4. read the provider binding and `#*-cells` where applicable
5. decode arguments using binding constants
6. verify provider ancestors, clocks, resets, supplies, and domains
7. confirm provider driver configuration and registration
8. inspect subsystem-specific runtime diagnostics
9. follow deferral to the earliest unavailable supplier

## Common Errors

- Treating a DMA request ID as a Linux channel index.
- Treating an IOMMU stream ID as an address.
- Using generic `phys` for a subsystem whose binding models a bus child instead.
- Adding argument cells to a regulator supply phandle.
- Looking for `regulator-names` instead of binding-defined `*-supply` properties.
- Debugging the final consumer while its provider's own regulator is missing.
- Assuming successful probe proves DMA or IOMMU traffic is correct.

## Exercises

1. Map `dma-names = "rx", "tx"` to two variable-width DMA entries.
2. Explain what evidence is needed to call an IOMMU argument a stream ID.
3. Distinguish generic PHY references from an MDIO Ethernet PHY child.
4. Translate driver request name `iovdd` into the expected supply property.
5. Draw a provider chain for a USB controller, PHY, clock, reset, regulator, and power domain.

## References And Next Step

- [Linux DMA Engine client guide](https://docs.kernel.org/driver-api/dmaengine/client.html)
- [Linux Generic PHY Framework](https://docs.kernel.org/driver-api/phy/phy.html)
- [Linux regulator consumer interface](https://docs.kernel.org/power/regulator/consumer.html)
- [Linux device links](https://docs.kernel.org/driver-api/device_link.html)
- [Linux Devicetree bindings](https://docs.kernel.org/devicetree/bindings/index.html)

Continue with the [Provider-Consumer Tracing Lab](provider-consumer-tracing-lab.md).

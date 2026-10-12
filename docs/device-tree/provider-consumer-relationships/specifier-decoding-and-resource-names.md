---
status: draft
reviewed: false
domain: device-tree
difficulty: intermediate
last_reviewed: null
---

# Specifier Decoding And Resource Names

## A Phandle Is Only The Start Of An Entry

Each entry in a phandle array begins with a phandle, then carries the number of argument cells declared by the referenced provider. The AM64x SERDES wrapper takes clocks from two very different providers (`k3-am64-main.dtsi`):

```dts
/ {
        serdes_refclk: clock-cmnrefclk {
                #clock-cells = <0>;
                compatible = "fixed-clock";
                clock-frequency = <0>;
        };
};

k3_clks: clock-controller {                 /* child of the TI-SCI node */
        compatible = "ti,k2g-sci-clk";
        #clock-cells = <2>;
};

serdes_wiz0: wiz@f000000 {
        compatible = "ti,am64-wiz-10g";
        clocks = <&k3_clks 162 0>, <&k3_clks 162 1>, <&serdes_refclk>;
        clock-names = "fck", "core_ref_clk", "ext_ref_clk";
        /* ... */
};
```

The effective entries are:

```text
&k3_clks 162 0      provider + 2 arguments
&k3_clks 162 1      provider + 2 arguments
&serdes_refclk      provider + 0 arguments
```

A parser therefore advances by a different length for each entry. The commas are source formatting only; a raw cell dump of this property is seven cells, so resolve each phandle before advancing. Never parse a compiled array as fixed tuples unless every permitted provider binding proves that shape.

The provider binding, not the consumer and not `dtc`, defines what the arguments mean. For `ti,k2g-sci-clk`, the first cell is the TI-SCI device ID (162, the SERDES) and the second the clock ID within that device.

## What `#*-cells` Means

A provider advertises the number of argument cells following its phandle for one resource class. Real values from AM64x and Arm Juno:

```dts
#clock-cells = <2>;          /* k3_clks: TI-SCI device ID, clock ID */
#reset-cells = <2>;          /* k3_reset: TI-SCI device ID, reset mask */
#power-domain-cells = <2>;   /* k3_pds: TI-SCI device ID, exclusive/shared flag */
#gpio-cells = <2>;           /* main_gpio0: line offset, flags */
#interrupt-cells = <3>;      /* gic500: type, number, trigger flags */
#dma-cells = <2>;            /* main_pktdma: PSI-L thread ID, ASEL */
#phy-cells = <1>;            /* phy_gmii_sel: CPSW port number */
#iommu-cells = <1>;          /* Juno smmu_dma: stream ID */
```

The leading `#` is part of the property name. The value is a cell count, not the number of resources exported by the provider.

On K3 SoCs one firmware-backed node exposes several provider classes through child nodes, each with its own count and namespace (`k3-am64-main.dtsi`):

```dts
dmsc: system-controller@44043000 {
        compatible = "ti,k2g-sci";
        /* ... */

        k3_pds: power-controller {
                compatible = "ti,sci-pm-domain";
                #power-domain-cells = <2>;
        };

        k3_clks: clock-controller {
                compatible = "ti,k2g-sci-clk";
                #clock-cells = <2>;
        };

        k3_reset: reset-controller {
                compatible = "ti,sci-reset";
                #reset-cells = <2>;
        };
};

main_r5fss0_core0: r5f@78000000 {
        resets = <&k3_reset 121 1>;
        /* power domain is on the parent r5fss node: <&k3_pds 119 TI_SCI_PD_EXCLUSIVE> */
};
```

All three use device IDs, but the second cell means a clock ID, a reset mask, or an ownership flag depending on the provider. Same firmware does not imply a shared argument meaning.

## Zero-Cell Providers

A zero-cell provider such as `serdes_refclk` needs no selector because the phandle uniquely identifies the resource. The phandle itself is still present; adding an invented `0`, as in `<&serdes_refclk 0>`, creates an extra cell that the parser reads as the start of the next entry.

## Consumer Binding And Provider Binding Have Different Jobs

The consumer binding defines:

- whether the property is required
- how many resource entries are allowed
- their semantic order
- whether a names property is required
- constraints specific to this consumer

The provider binding defines:

- its `#*-cells` count
- the meaning and allowed values of argument cells
- relationships among exported resources

Correct decoding needs both.

## Resource Names

Many list properties have a companion string list. The AM64x 8-bit SDHCI and MCAN nodes (`k3-am64-main.dtsi`):

```dts
sdhci0: mmc@fa10000 {
        clocks = <&k3_clks 57 0>, <&k3_clks 57 1>;
        clock-names = "clk_ahb", "clk_xin";
};

main_mcan0: can@20701000 {
        reg = <0x00 0x20701000 0x00 0x200>,
              <0x00 0x20708000 0x00 0x8000>;
        reg-names = "m_can", "message_ram";
        clocks = <&k3_clks 98 5>, <&k3_clks 98 0>;
        clock-names = "hclk", "cclk";
        interrupts = <GIC_SPI 155 IRQ_TYPE_LEVEL_HIGH>,
                     <GIC_SPI 156 IRQ_TYPE_LEVEL_HIGH>;
        interrupt-names = "int0", "int1";
};
```

Names map positionally:

| Index | `sdhci0` clock entry | Clock name |
|---:|---|---|
| 0 | `<&k3_clks 57 0>` | `clk_ahb` |
| 1 | `<&k3_clks 57 1>` | `clk_xin` |

The name describes the resource's function at the consumer. It does not rename the provider output: `clk_xin` is TI-SCI clock 1 of device 57.

Bindings guidance recommends explicit ordering and matching constraints for phandle lists and their names. Reordering the resources without the names silently changes which resource a driver obtains.

## Names And Linux Lookup

Driver APIs request the consumer-facing name. In mainline Linux:

```c
/* drivers/mmc/host/sdhci_am654.c */
clk_xin = devm_clk_get(dev, "clk_xin");

/* drivers/spi/spi-omap2-mcspi.c builds "rx0"/"tx0" ... and calls */
mcspi_dma->dma_rx = dma_request_chan(mcspi->dev, mcspi_dma->dma_rx_ch_name);
```

which match `clock-names = "clk_ahb", "clk_xin";` on `sdhci0` and `dma-names = "tx0", "rx0";` on `main_spi0`. The framework maps the name to the matching index in `clock-names`, `reset-names`, `dma-names`, or `phy-names`, then decodes the corresponding provider entry.

Names are ABI. Renaming `"clk_xin"` in the DTS without changing the binding and every driver that reads it breaks resource lookup.

## Optional And Required Resources

Optionality comes from the consumer binding and driver API contract, not from phandle syntax. An absent optional property differs from:

- a present but malformed property
- a phandle to a disabled provider
- a valid provider that has not probed
- an argument value rejected by the provider

Optional getter APIs often return `NULL` for a genuinely absent resource but propagate errors for broken dependencies. Treating every error as “not fitted” hides board-description defects.

## A Mechanical Decoding Worksheet

For each consumer property, record:

| Field | Question |
|---|---|
| consumer binding | What entries and names are expected? |
| property index | Which logical function is this? |
| provider path | Where does the phandle resolve? |
| provider compatible | Which binding applies? |
| `#*-cells` | How many argument cells follow? |
| raw arguments | What exact cells are present? |
| decoded meaning | What does the provider binding call each cell? |
| provider state | Is the node available and its driver registered? |

This worksheet scales from a fixed clock to a multi-level IOMMU or interrupt nexus.

## Common Errors

- Treating `#clock-cells` as the number of clocks exported.
- Adding `<&serdes_refclk 0>` to a zero-cell provider.
- Assuming every provider of one resource class uses the same argument count.
- Decoding argument values from a header constant without reading the provider binding.
- Allowing `*-names` count to drift from the resource list.
- Reordering phandles but not names.
- Calling a missing required resource optional in the DTS instead of fixing the binding mismatch.

## Exercises

1. Split `<&a &b 3 &c 4 5>` when `a`, `b`, and `c` declare 0, 1, and 2 cells.
2. Explain why `#gpio-cells = <2>` says nothing by itself about which cell contains flags, and how the `#power-domain-cells = <2>` of `k3_pds` differs from the `#clock-cells = <2>` of `k3_clks`.
3. Map three clocks to `clock-names` and show the effect of swapping two names.
4. List the consumer-binding and provider-binding questions needed to decode one entry.
5. Explain the difference between absent optional resource and unavailable provider.

## References And Next Step

- [Devicetree Specification](https://devicetree-specification.readthedocs.io/en/stable/devicetree-basics.html)
- [Linux binding design guidance](https://docs.kernel.org/devicetree/bindings/writing-bindings.html)
- [Linux DeviceTree kernel API](https://docs.kernel.org/devicetree/kernel-api.html)

Continue with [Clocks, Resets, And Power Domains](clocks-resets-and-power-domains.md).

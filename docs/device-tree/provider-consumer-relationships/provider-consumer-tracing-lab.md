---
status: draft
reviewed: false
domain: device-tree
difficulty: intermediate
last_reviewed: null
---

# Provider-Consumer Tracing Lab

## Goal

Decode real consumers in the TI AM642 SK DTB, prove the effective entries from raw cells, trace them to the Linux lookups that consume them, and diagnose deliberate count, name, and supplier faults injected into a copy of the blob.

## Inputs

- `k3-am642-sk.dtb` built from mainline Linux as in the [First Device Tree Lab](../foundations/first-device-tree-lab.md#step-10-build-the-real-board-dtb)
- the sources `arch/arm64/boot/dts/ti/k3-am64-main.dtsi` and `k3-am642-sk.dts`
- `dtc`, `fdtget`, and `fdtput`

Phandle numbers below were observed with a v6.19-rc1 build. Yours may differ; the method does not.

## Step 1: Capture Identity And A Phandle Index

```sh
D=k3-am642-sk.dtb
sha256sum "$D"
dtc -I dtb -O dts -o sk.dts "$D"
grep -n 'phandle = ' sk.dts | wc -l
```

To resolve a numeric phandle, search the decompiled tree:

```sh
grep -n -B20 'phandle = <0x02>;' sk.dts | grep -E '^[0-9]+-\s+[a-z].*\{' | tail -1
```

With the observed build, phandle `0x2` is `k3_clks` (`clock-controller` under `system-controller@44043000`), `0x3` is `k3_pds`, `0x31` is `serdes_refclk`, and `0x16` is `main_gpio0`.

## Step 2: Complete A Decoding Table

Without looking at the reference answers, fill this table from raw cells:

| Consumer node | Property | Raw cells (`fdtget -tx`) | Provider | `#*-cells` | Decoded arguments | Name |
|---|---|---|---|---|---|---|
| `wiz@f000000` | `clocks` | | | | | |
| `spi@20100000` | `dmas` | | | | | |
| `ethernet@8000000/ethernet-ports/port@1` | `phys` | | | | | n/a |
| `cdns-usb@f900000/usb@f400000` | `phys` | | | | | |
| `serial@2800000` | `power-domains` | | | | | n/a |
| `mmc@fa10000/wlcore@2` | `interrupts` | | via `interrupt-parent` | | | n/a |
| `/regulator-4` (`wlan_en`) | `gpio` | | | | | n/a |
| `mmc@fa00000` | `vmmc-supply` | | | none | none | `vmmc` |

Useful commands:

```sh
fdtget -tx $D /bus@f4000/wiz@f000000 clocks
fdtget     $D /bus@f4000/wiz@f000000 clock-names
fdtget -tx $D /bus@f4000/spi@20100000 dmas
fdtget -tx $D /bus@f4000/ethernet@8000000/ethernet-ports/port@1 phys
fdtget -tx $D /bus@f4000/cdns-usb@f900000/usb@f400000 phys
fdtget -tx $D /bus@f4000/serial@2800000 power-domains
fdtget -tx $D /bus@f4000/mmc@fa10000/wlcore@2 interrupt-parent
fdtget -tx $D /bus@f4000/mmc@fa10000/wlcore@2 interrupts
fdtget -tx $D /regulator-4 gpio
fdtget -tx $D /bus@f4000/mmc@fa00000 vmmc-supply
```

Raw numeric output does not preserve tuple boundaries. Resolve each phandle and read its `#*-cells` before counting arguments.

## Step 3: Trace Consumer Names To Driver Lookups

Find the call that consumes each named entry in the kernel source:

```sh
grep -n '"clk_xin"' drivers/mmc/host/sdhci_am654.c
grep -n 'dma_request_chan\|"rx%d"\|"tx%d"' drivers/spi/spi-omap2-mcspi.c
grep -n '"vmmc"' drivers/mmc/core/regulator.c
grep -n 'devm_phy_optional_get' drivers/usb/cdns3/cdns3-plat.c
```

For each, record the property, names list if any, selected index, provider, and arguments.

## Step 4: Inject A Stray Cell After A Zero-Cell Provider

Work only on a copy:

```sh
cp $D z.dtb
fdtput -t x z.dtb /bus@f4000/wiz@f000000 clocks 2 a2 0 2 a2 1 31 0
dtc -I dtb -O dts z.dtb >/dev/null
```

`dtc` reports nothing: a zero phandle is a legal empty entry in a phandle array, so the stray `0` parses as a fourth, empty clock. The WIZ binding (`ti,phy-j721e-wiz.yaml`) even permits a fourth clock, `core_ref1_clk`, so four entries are not obviously invalid; the mismatch with the three-entry `clock-names` list is visible only to a careful reviewer, a stricter schema, or the driver. Explain why this is worse than a loud error.

## Step 5: Break A Names List

```sh
cp $D n.dtb
fdtput -t s n.dtb /bus@f4000/mmc@fa10000 clock-names clk_xin clk_ahb
fdtget n.dtb /bus@f4000/mmc@fa10000 clock-names
```

The cells and phandles are structurally valid, but `devm_clk_get(dev, "clk_xin")` now returns TI-SCI clock 0 of device 57 instead of clock 1. Explain why only binding schemas with ordered `clock-names` items, or hardware behavior, reveal this.

## Step 6: Break A Provider Count

```sh
cp $D c.dtb
fdtput -t u c.dtb /bus@f4000/system-controller@44043000/clock-controller '#clock-cells' 1
dtc -I dtb -O dts c.dtb 2>&1 >/dev/null | head
```

`dtc`'s `clocks_property` check now splits every `<&k3_clks dev clk>` entry after one argument and reads the leftover clock ID as the next phandle. Clock ID `1` is the phandle of `gic500`, so the warnings claim that timers reference `interrupt-controller@1800000` without `#clock-cells`. Explain why a provider-side change breaks dozens of consumers at once, and why the warning text names the wrong node.

## Step 7: Model Supplier Unavailability

```sh
cp $D s.dtb
fdtput -t s s.dtb /bus@f4000/i2c@20010000/gpio@70 status disabled
```

The SD-card switch `vdd_mmc1` still resolves its `gpio = <&exp1 3 ...>` phandle and the blob is valid, but at runtime the PCA9538 expander is never created. Explain the runtime difference among:

- phandle cannot resolve
- provider node is disabled (this case)
- provider driver is not configured
- provider driver defers
- provider registers but rejects the specifier

These cases need different fixes despite similar consumer symptoms.

## Step 8: Draw The Dependency Graph

Draw `sdhci1` and all its suppliers down to the root: the `vmmc-supply` regulator chain, the expander and its I2C controller, and every clock and power domain on the way. Mark the earliest supplier whose failure would cause several downstream deferrals.

## Reference Answers

| Consumer | Raw cells | Decoded |
|---|---|---|
| `wiz@f000000` `clocks` | `2 a2 0 2 a2 1 31` | `k3_clks` dev 162 clk 0 (`fck`); `k3_clks` dev 162 clk 1 (`core_ref_clk`); `serdes_refclk`, zero cells (`ext_ref_clk`) |
| `spi@20100000` `dmas` | `c c300 0 c 4300 0` | `main_pktdma` PSI-L thread `0xc300` ASEL 0 (`tx0`); thread `0x4300` ASEL 0 (`rx0`) |
| CPSW `port@1` `phys` | `1b 1` | `phy_gmii_sel`, CPSW port 1 |
| `usb@f400000` `phys` | `21` | `serdes0_usb_link`, zero cells (`cdns3,usb3-phy`) |
| `serial@2800000` `power-domains` | `3 92 1` | `k3_pds` dev 146, `TI_SCI_PD_EXCLUSIVE` |
| `wlcore@2` `interrupts` | `2e 2` | parent `main_gpio0`: line 46, `IRQ_TYPE_EDGE_FALLING` |
| `wlan_en` `gpio` | `16 30 0` | `main_gpio0` line 48, `GPIO_ACTIVE_HIGH` |
| `mmc@fa00000` `vmmc-supply` | `17` | `vdd_mmc1` regulator |

## Review Questions

1. Which properties embed a provider phandle in every entry?
2. Why does `interrupts` require a different first decoding step from `interrupts-extended`?
3. Which relationship in the table has no `#*-cells` property?
4. Why can `main_gpio0` use two different cell-count properties?
5. What evidence distinguishes a wrong consumer name from a missing provider?
6. Why is property order not a resource sequencing language?

## Completion Checklist

- [ ] I decoded a mixed-width provider list from raw cells.
- [ ] I handled a zero-cell provider correctly.
- [ ] I mapped `*-names` entries to real driver lookups.
- [ ] I distinguished GPIO and interrupt roles on one provider.
- [ ] I traced DMA, PHY, power-domain, GPIO, and regulator suppliers.
- [ ] I demonstrated faults that `dtc` misreports or does not report.
- [ ] I can distinguish malformed, disabled, unconfigured, deferred, and rejected suppliers.
- [ ] I can draw a multi-level dependency graph and choose the earliest useful debugging point.

## References And Next Steps

- [Linux Devicetree bindings guidance](https://docs.kernel.org/devicetree/bindings/writing-bindings.html)
- [Linux DeviceTree kernel API](https://docs.kernel.org/devicetree/kernel-api.html)
- [Linux probe failure debugging](../../linux-kernel/debugging/probe-failure-debugging.md)
- [Device Tree binding validation](../../build-systems/advanced/linux-kernel/device-tree-binding-validation.md)

Return to [Provider-Consumer Relationships](../provider-consumer-relationships.md), or continue with [Standard Nodes And Properties](../standard-nodes-and-properties.md).

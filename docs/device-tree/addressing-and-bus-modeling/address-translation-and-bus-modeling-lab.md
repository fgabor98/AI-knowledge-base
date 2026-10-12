---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# Address Translation And Bus Modeling Lab

## Goal

Decode CPU, DMA, and interrupt paths in two real DTBs—the TI AM642 SK and the Raspberry Pi 4 Model B—then inspect a PCI host window. Deliberate failures injected into copies test cell grouping, window containment, inheritance, and namespace separation.

## Inputs

Build both DTBs from mainline Linux (see the [First Device Tree Lab](../foundations/first-device-tree-lab.md#step-10-build-the-real-board-dtb) for the manual `cpp` + `dtc` pipeline if Kbuild is unavailable):

```sh
make ARCH=arm64 ti/k3-am642-sk.dtb ti/k3-am642-evm.dtb broadcom/bcm2711-rpi-4-b.dtb
```

Sources to keep open: `arch/arm64/boot/dts/ti/k3-am64.dtsi`, `k3-am64-main.dtsi`, `k3-am642-sk.dts`, and `arch/arm/boot/dts/broadcom/bcm2711.dtsi`.

## Step 1: Build And Preserve Evidence

```sh
SK=k3-am642-sk.dtb
EVM=k3-am642-evm.dtb
RPI=bcm2711-rpi-4-b.dtb
sha256sum $SK $EVM $RPI
dtc --version
```

If you reproduce the build by hand, `dtc` prints `unit_address_vs_reg` warnings for the Pi's `/soc` node. Kbuild suppresses that check by default (`-Wno-unit_address_vs_reg` in `scripts/Makefile.dtbs`); note which flags produced your artifacts.

## Step 2: Build A Cell-Contract Table

| Node | Parent | Parent `#address-cells` | Parent `#size-cells` | Raw `reg` |
|---|---|---:|---:|---|
| RPi `/soc/serial@7e201000` | `/soc` | | | |
| SK `/bus@f4000/serial@2800000` | `/bus@f4000` | | | |
| SK `/bus@f4000/icssg@30000000/pru@34000` | `icssg@30000000` | | | |
| SK `/bus@f4000/syscon@43000000/phy@4044` | `syscon@43000000` | | | |

```sh
fdtget -tx $RPI /soc/serial@7e201000 reg
fdtget -tx $SK /bus@f4000/serial@2800000 reg
fdtget -tx $SK /bus@f4000/icssg@30000000/pru@34000 reg
fdtget -tx $SK /bus@f4000/syscon@43000000/phy@4044 reg
fdtget -tu $SK /bus@f4000/icssg@30000000 '#address-cells'
```

## Step 3: Translate CPU Resources

Read each `ranges` and translate outward one boundary at a time:

```sh
fdtget -tx $RPI / '#address-cells'
fdtget -tx $RPI /soc ranges
fdtget -tx $SK /bus@f4000/icssg@30000000 ranges
fdtget -tx $SK /bus@f4000/syscon@43000000 ranges
```

Expected results:

| Child | Path | CPU interval |
|---|---|---|
| RPi UART0 `0x7e201000 0x200` | `/soc` window `0x7e000000 → 0xfe000000` | `[0xfe201000, 0xfe201200)` |
| AM64x PRU0_0 IRAM `0x34000 0x3000` | `icssg0` window `0x0 → 0x30000000`, then identity `ICSSG0/1` window | `[0x30034000, 0x30037000)` |
| AM64x `phy_gmii_sel` `0x4044 0x8` | `main_conf` window `0x0 → 0x43000000`, then identity `CTRL_MMR0` window | `[0x43004044, 0x4300404c)` |

For each, confirm the whole interval lies inside both windows, not just its first byte.

## Step 4: Translate The DMA Aperture

```sh
fdtget -tx $RPI /soc dma-ranges
fdtget -tx $SK /bus@f4000/bus@48000000 dma-ranges
fdtget -tx $SK /bus@f4000/pcie@f102000 dma-ranges
```

Answer:

1. Which bus address does a legacy Raspberry Pi DMA master program for a buffer at CPU physical `0x08000000`?
2. Can that master reach a buffer at `0x50000000`? What does Linux do instead?
3. What does the empty `dma-ranges` on the AM64x DMSS bus assert?
4. How large is the AM64x PCIe inbound window, and is it translated?

## Step 5: Resolve Interrupt Inheritance

```sh
fdtget -tx $SK / interrupt-parent
fdtget -tx $SK /bus@f4000/interrupt-controller@1800000 phandle
fdtget -tx $SK /bus@f4000/serial@2800000 interrupts
fdtget -tx $SK /bus@f4000/mmc@fa10000/wlcore@2 interrupt-parent
fdtget -tx $SK /bus@f4000/gpio@600000 interrupt-parent
fdtget -tx $SK /bus@f4000/interrupt-controller@a00000 interrupt-parent
```

Draw the paths:

```text
serial@2800000  --inherits from / -->  gic500 (SPI 178, level high)
wlcore@2        --explicit -->  main_gpio0 line 46
                                  --> main_gpio_intr (router input 190..195)
                                  --> gic500
```

## Step 6: Decode PCI Windows

The SK repurposes the SERDES lane for USB, so use the EVM, whose PCIe root complex is enabled:

```sh
fdtget -tx $EVM /bus@f4000/pcie@f102000 ranges
fdtget -tx $EVM /bus@f4000/pcie@f102000 reg
fdtget $EVM /bus@f4000/pcie@f102000 status
```

Split each `ranges` entry into space code, PCI address, CPU address, and size. Identify the I/O and 32-bit memory windows and the 4 KiB configuration window in `reg`.

## Step 7: Break A `reg` Width

```sh
cp $SK w.dtb
fdtput -t x w.dtb /bus@f4000/serial@2800000 reg 2800000 100
dtc -I dtb -O dts w.dtb >/dev/null
```

Record the `reg_format` warning. Split the two cells under the parent's two-address/two-size contract and explain the result.

## Step 8: Cross A Window Boundary

```sh
cp $SK x.dtb
fdtput -t x x.dtb /bus@f4000/icssg@30000000/pru@34000 reg 7e000 3000 22000 100 22400 100
dtc -I dtb -O dts x.dtb >/dev/null
```

`dtc` is silent. Compute the IRAM interval in ICSSG-local space and show that it crosses the end of the `[0x0, 0x80000)` window. Explain why checking only the first byte of a resource misses this.

## Step 9: Remove Versus Empty `ranges`

```sh
cp $SK r.dtb
fdtput -d r.dtb /bus@f4000/icssg@30000000 ranges
dtc -I dtb -O dts r.dtb >/dev/null
```

`dtc` is silent again, but every ICSSG child now has no defined CPU address. Explain why an absent `ranges` is not identity, and relate it to the real `main_gpio_intr` node whose `reg` lies outside every `bus@f4000` window (see [Multiple And Disjoint Windows](simple-bus-ranges-and-nested-translation.md#multiple-and-disjoint-windows)).

## Step 10: Break Interrupt Inheritance

```sh
cp $SK i.dtb
fdtput -d i.dtb / interrupt-parent
dtc -I dtb -O dts i.dtb 2>&1 >/dev/null | head
dtc -I dtb -O dts i.dtb 2>&1 >/dev/null | wc -l
```

Dozens of `interrupts_property: ... Missing interrupt-parent` warnings appear, because most AM64x nodes rely on the root's inherited parent. Explain why `wlcore@2` and `main_gpio0` are unaffected.

## Step 11: Compare With Linux Runtime

On an AM642 SK running the corresponding kernel, collect:

```sh
grep -i -E '2800000|30034000|f102000' /proc/iomem
cat /proc/interrupts | head -40
readlink /sys/bus/platform/devices/*/of_node | head
```

Compare `/proc/iomem` intervals with your Step 3 calculations, and `/proc/interrupts` with Step 5. Linux IRQ numbers in `/proc/interrupts` are not the DT specifier cells.

## Completion Criteria

The lab is complete when you have:

- identities and build flags for all three DTBs
- a cell-contract table with every width source named
- CPU intervals for three nested or translated resources
- a DMA address for a Raspberry Pi buffer and an explanation of the 1 GiB limit
- interrupt-parent paths for inherited and explicit cases
- decoded AM64x PCIe windows
- explanations for each injected failure and whether `dtc` reported it

## Authoritative References

- [Devicetree Specification: addressing and interrupts](https://devicetree-specification.readthedocs.io/en/stable/devicetree-basics.html)
- [Linux DeviceTree address translation APIs](https://docs.kernel.org/devicetree/kernel-api.html)
- [Linux DMA API HOWTO](https://docs.kernel.org/core-api/dma-api-howto.html)
- [Linux IRQ-domain documentation](https://docs.kernel.org/core-api/irq/irq-domain.html)

## Continue

Proceed to [Driver Matching](../driver-matching.md).

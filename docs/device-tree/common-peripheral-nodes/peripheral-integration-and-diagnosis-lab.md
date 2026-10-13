---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# Peripheral Integration And Diagnosis Lab

This lab reviews the TI AM642 EVM as an integrated peripheral board: an I²C GPIO expander that seven other devices depend on, an MDIO bus split by a GPIO-controlled mux, two RGMII PHYs, an octal SPI NOR boot flash, CAN transceivers, and a PCIe root port. It emphasizes address-space boundaries, cross-subsystem dependencies, and evidence. Most steps need only the DTB; Steps 5 and 6 need the board.

## Inputs

- `k3-am642-evm.dtb` and `arch/arm64/boot/dts/ti/k3-am642-evm.dts` from mainline Linux
- `k3-am64-main.dtsi` for the SoC halves of each node
- for Steps 5 and 6, an AM642 EVM running the matching kernel

```sh
E=k3-am642-evm.dtb
```

## Step 1: Read The Shared Expander

From `k3-am642-evm.dts` (trimmed):

```dts
&main_i2c1 {
        status = "okay";
        clock-frequency = <400000>;

        exp1: gpio@22 {
                compatible = "ti,tca6424";
                reg = <0x22>;
                gpio-controller;
                #gpio-cells = <2>;
                gpio-line-names = "GPIO_eMMC_RSTn", "CAN_MUX_SEL",
                                  "GPIO_CPSW1_RST", "GPIO_RGMII1_RST",
                                  "GPIO_RGMII2_RST", "GPIO_PCIe_RST_OUT",
                                  "MMC1_SD_EN", "FSI_FET_SEL",
                                  "MCAN0_STB_3V3", "MCAN1_STB_3V3",
                                  "CPSW_FET_SEL", "CPSW_FET2_SEL",
                                  "PRG1_RGMII2_FET_SEL", "TEST_GPIO2",
                                  "GPIO_OLED_RESETn", "VPP_LDO_EN",
                                  "TEST_LED1", /* ... */;
        };
};
```

Find every consumer from the DTB rather than from memory:

```sh
X=$(fdtget -tx $E /bus@f4000/i2c@20010000/gpio@22 phandle)
dtc -I dtb -O dts $E 2>/dev/null | grep -n -E "gpios? = <0x$X "
```

Expected: seven references, on lines 5 (`pcie@f102000` `reset-gpios`), 6 (`vdd_mmc1` `gpio`), 8 and 9 (`can-phy0`/`can-phy1` `standby-gpios`), 12 (`mux-controller` `mux-gpios`), 14 (`display@3c` `reset-gpios`), and 16 (the heartbeat LED). Map each to its node and line name. The `grep` is a quick filter; confirm each hit against the source because a phandle value can also appear as an ordinary integer.

## Step 2: Decode Every Address Space

Build this table from the DTB:

| Node | Parent address space | `reg` meaning |
|---|---|---|
| `i2c@20010000/gpio@22` | I²C bus | 7-bit target address `0x22` |
| `i2c@20000000/eeprom@50` | I²C bus | target address `0x50` (AT24CM01) |
| `mdio@f00/ethernet-phy@0` | MDIO bus | PHY address 0 |
| `mdio-mux-1/mdio@1` | MDIO mux channels | channel 1 (`exp1` line 12 high) |
| `mdio@1/ethernet-phy@3` | MDIO bus behind channel 1 | PHY address 3 |
| `spi@fc40000/flash@0` | OSPI chip selects | chip select 0 |
| `spi@20100000/eeprom@0` | McSPI chip selects | chip select 0 |
| partition nodes | flash byte space | offset and size |
| `pcie@f102000` | CPU physical space | four named register windows |

The repeated small integers (0, 1, 3) are unrelated across domains. That is the central review test for nested peripheral nodes. Note that `mdio-mux-1` is a root-level node: it reaches the CPSW MDIO bus through `mdio-parent-bus = <&cpsw3g_mdio>`, not through tree position.

## Step 3: Validate Flash Arithmetic

```sh
F=/bus@f4000/bus@fc00000/spi@fc40000/flash@0/partitions
for n in $(fdtget -l $E $F); do echo "$n $(fdtget $E $F/$n label) $(fdtget -tx $E $F/$n reg)"; done
```

Convert each boundary to an interval. Expected:

```text
ospi.tiboot3     0x0000000 .. 0x00fffff   1 MiB
ospi.tispl       0x0100000 .. 0x02fffff   2 MiB
ospi.u-boot      0x0300000 .. 0x06fffff   4 MiB
ospi.env         0x0700000 .. 0x073ffff   256 KiB
ospi.env.backup  0x0740000 .. 0x077ffff   256 KiB
(unpartitioned)  0x0780000 .. 0x07fffff   512 KiB
ospi.rootfs      0x0800000 .. 0x3fbffff   55.75 MiB
ospi.phypattern  0x3fc0000 .. 0x3ffffff   256 KiB
```

The layout ends at 64 MiB. Check `ospi.tispl` and `ospi.u-boot` against `CONFIG_SYS_SPI_U_BOOT_OFFS` in TI U-Boot's `am64x_evm_r5_defconfig` and `am64x_evm_a53_defconfig`. Then compare erase sizes and the detected flash capacity. Arithmetic alone does not validate an update design.

## Step 4: Trace RGMII Delay Ownership

```sh
fdtget $E /bus@f4000/ethernet@8000000/ethernet-ports/port@1 phy-mode
fdtget -tx $E /bus@f4000/ethernet@8000000/mdio@f00/ethernet-phy@0 ti,rx-internal-delay
```

Explain each delay. `rgmii-id` says the PCB adds no skew. `am65-cpsw-nuss` always adds TX delay in the MAC, so it connects the PHY as `rgmii-rxid`, and the DP83867 adds the RX delay (`DP83867_RGMIIDCTL_2_00_NS`). Port 2's PHY at address 3 has no `ti,rx-internal-delay`. Identify that PHY from its ID registers at runtime, and find out from its driver what RX delay it applies when the property is absent.

## Step 5: Collect Runtime Evidence

On the EVM:

```sh
gpioinfo | grep -A24 'gpiochip.*22'
ls /sys/class/devlink/ | grep -i 'i2c:1-0022'
ip -details link show
ethtool eth0; ethtool eth1
cat /proc/mtd
ip -details link show can0
ls /sys/class/leds
lspci -tv
dmesg | grep -Ei 'tca6424|pca953x|mdio|dp83867|rgmii|spi-nor|m_can|pcie|link'
```

Map runtime instance numbers through OF paths. Do not assume the first I²C adapter, network interface, MTD index, or GPIO chip number is stable. The device links to `1-0022` should match the seven consumers from Step 1.

## Step 6: Exercise Lifecycle And Boundaries

Test:

1. cold boot and warm reboot, comparing PHY detection, PCIe link, and LED state
2. Ethernet traffic on both ports at all advertised speeds
3. CAN traffic with a known-good peer, then with the interface down (transceiver in standby)
4. read-only flash identification and partition-boundary checks
5. suspend and resume, rechecking PHYs behind the MDIO mux

Do not erase or write the boot or environment partitions. Use a disposable region and a recovery plan if destructive validation is authorized later.

## Step 7: Diagnose Deliberate Faults

Inject each fault into a copy of the DTB and run `dtc -I dtb -O dts <copy> >/dev/null`; record which ones it reports.

### Fault A: The Expander Is Disabled

```sh
cp $E a.dtb
fdtput -t s a.dtb /bus@f4000/i2c@20010000/gpio@22 status disabled
```

Predict all seven consequences: the SD supply, both CAN transceivers, the MDIO mux controller, PCIe, the OLED, and the LED all defer waiting for their GPIO. The CPSW itself still probes, because `phy-handle` is not one of the properties `fw_devlink` treats as a supplier link, and `am65-cpsw-nuss` connects the PHY only in `ndo_open`. So `eth1` exists but fails when brought up, because the mux never registered the bus that holds PHY 3. Explain why that visible symptom is three providers away from the cause, and how `/sys/kernel/debug/devices_deferred` leads back to it.

### Fault B: The MDIO Mux Channel Is Wrong

```sh
cp $E b.dtb
fdtput -t x b.dtb /mdio-mux-1/mdio@1 reg 0
```

The mux now drives `exp1` line 12 low when accessing the child bus. Unless a PHY also answers at address 3 on channel 0, no PHY device is created for `ethernet-phy@3`, and `eth1` fails to connect its PHY when brought up. Port 1 is unaffected because its PHY is on the parent bus. Note the unit-address/`reg` mismatch and whether `dtc` flags it.

### Fault C: The Board Claims PCB Delays

```sh
cp $E c.dtb
fdtput -t s c.dtb /bus@f4000/ethernet@8000000/ethernet-ports/port@1 phy-mode rgmii
```

Predict the boot warning `RGMII mode without internal TX delay unsupported; please fix your Device Tree`. The MAC still adds TX delay, but the DP83867 now receives `rgmii` and enables no RX delay. Predict link-up with receive CRC errors or no received traffic. Explain why the property describes the PCB, not which chip adds delay.

### Fault D: Two Boot Partitions Overlap

```sh
cp $E d.dtb
fdtput -t x d.dtb $F/partition@100000 reg 100000 300000
```

`ospi.tispl` now overlaps the first MiB of `ospi.u-boot`. `dtc` accepts it and the fixed-partitions parser registers both partitions. Detect the overlap from interval arithmetic before boot testing, and explain what a U-Boot update through `/dev/mtdN` would now destroy.

### Fault E: PERST# Polarity Is “Corrected”

```sh
cp $E e.dtb
fdtput -t x e.dtb /bus@f4000/pcie@f102000 reset-gpios 13 5 1
```

(`13` is the observed phandle of `exp1`; confirm yours.) The flag now says active low, which matches the PERST# signal name, yet `pci-j721e` sets logical 1 to release reset. Predict that the endpoint stays in reset, the link never trains, and `lspci` shows only the root port. Explain why this flag must follow the driver's convention ([PCIe page](pcie-host-bridges-windows-and-enumeration.md#root-complex-resources)).

## Exit Review

The lab is complete when you can provide:

- the seven-consumer map for `exp1` with runtime device-link evidence
- a parent-address-space decode for every `reg` in Step 2
- the flash interval table, cross-checked against U-Boot offsets
- RGMII delay ownership for both ports
- stable subsystem-based identification of every runtime device
- cold/warm boot and suspend/resume results
- predicted and observed outcomes for each fault, including which ones `dtc` reported

## Authoritative References

- [Linux I2C sysfs topology](https://docs.kernel.org/i2c/i2c-sysfs.html)
- [Linux NVMEM subsystem](https://docs.kernel.org/driver-api/nvmem.html)
- [Linux PHY and phylink documentation](https://docs.kernel.org/networking/sfp-phylink.html)
- [Linux SPI NOR framework](https://docs.kernel.org/driver-api/mtd/spi-nor.html)
- [Linux subsystem drivers using GPIO](https://docs.kernel.org/driver-api/gpio/drivers-on-gpio.html)

## Continue

Proceed to [Graph Bindings And Complex Data Paths](../graph-bindings-and-complex-data-paths.md).

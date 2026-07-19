---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# Peripheral Integration And Diagnosis Lab

This lab integrates an I²C mux and EEPROM-backed MAC address, an Ethernet PHY, SPI NOR partitions, a status LED, and a wake button. It emphasizes address-space boundaries, cross-subsystem dependencies, and evidence. Compatible strings and controller labels are illustrative; real hardware must use its exact schemas.

## Board Contract

Assume the board has:

- an I²C mux at parent-bus address `0x70`
- a 64-Kbit EEPROM at `0x50` on mux channel 1
- the Ethernet MAC address in EEPROM bytes `0x10..0x15`
- an RGMII PHY at MDIO address 1, with PHY-provided RX and TX delays
- a 16 MiB SPI NOR on chip select 0
- 1 MiB bootloader, 7 MiB slot A, 7 MiB slot B, and 1 MiB environment regions
- an active-high green status LED
- an active-low, interrupt-capable user button that may wake the system

## Step 1: Review The Description

```dts
#include <dt-bindings/gpio/gpio.h>
#include <dt-bindings/input/input.h>
#include <dt-bindings/leds/common.h>

&i2c1 {
        clock-frequency = <400000>;
        status = "okay";

        i2c-mux@70 {
                compatible = "nxp,pca9544";
                reg = <0x70>;
                #address-cells = <1>;
                #size-cells = <0>;

                i2c@1 {
                        reg = <1>;
                        #address-cells = <1>;
                        #size-cells = <0>;

                        board_eeprom: eeprom@50 {
                                compatible = "atmel,24c64";
                                reg = <0x50>;
                                #address-cells = <1>;
                                #size-cells = <1>;

                                mac_address: mac-address@10 {
                                        reg = <0x10 0x6>;
                                };
                        };
                };
        };
};

&ethernet0 {
        phy-mode = "rgmii-id";
        phy-handle = <&ethernet_phy>;
        nvmem-cells = <&mac_address>;
        nvmem-cell-names = "mac-address";
        status = "okay";

        mdio {
                #address-cells = <1>;
                #size-cells = <0>;

                ethernet_phy: ethernet-phy@1 {
                        compatible = "ethernet-phy-ieee802.3-c22";
                        reg = <1>;
                        reset-gpios = <&gpio3 4 GPIO_ACTIVE_LOW>;
                        reset-assert-us = <10000>;
                        reset-deassert-us = <30000>;
                };
        };
};

&spi0 {
        status = "okay";

        flash@0 {
                compatible = "jedec,spi-nor";
                reg = <0>;
                spi-max-frequency = <50000000>;

                partitions {
                        compatible = "fixed-partitions";
                        #address-cells = <1>;
                        #size-cells = <1>;

                        partition@0 {
                                label = "bootloader";
                                reg = <0x0000000 0x0100000>;
                                read-only;
                        };

                        partition@100000 {
                                label = "firmware-a";
                                reg = <0x0100000 0x0700000>;
                        };

                        partition@800000 {
                                label = "firmware-b";
                                reg = <0x0800000 0x0700000>;
                        };

                        partition@f00000 {
                                label = "environment";
                                reg = <0x0f00000 0x0100000>;
                        };
                };
        };
};

leds {
        compatible = "gpio-leds";

        led-status {
                function = LED_FUNCTION_STATUS;
                color = <LED_COLOR_ID_GREEN>;
                gpios = <&gpio2 12 GPIO_ACTIVE_HIGH>;
                default-state = "off";
        };
};

keys {
        compatible = "gpio-keys";

        key-user {
                label = "User button";
                linux,code = <KEY_PROG1>;
                gpios = <&gpio2 13 GPIO_ACTIVE_LOW>;
                debounce-interval = <20>;
                wakeup-source;
        };
};
```

This fragment assumes the SoC layer defines controllers, pins, clocks, resets, supplies, and interrupt topology. Verify those inherited resources in the final DTB.

## Step 2: Decode Every `reg`

Build this table before touching hardware:

| Node | Parent address space | Meaning |
|---|---|---|
| `i2c-mux@70` | I²C bus | 7-bit target address `0x70` |
| `i2c@1` | mux channels | channel 1 |
| `eeprom@50` | downstream I²C segment | target address `0x50` |
| `mac-address@10` | EEPROM NVMEM space | offset `0x10`, length 6 |
| `ethernet-phy@1` | MDIO bus | PHY address 1 |
| `flash@0` | SPI bus | chip select 0 |
| partition nodes | flash address space | byte offset and size |

The repeated integers are unrelated across domains. This is the central review test for nested peripheral nodes.

## Step 3: Validate Flash Arithmetic

Convert each boundary:

```text
bootloader  0x0000000 .. 0x00fffff   1 MiB
firmware-a  0x0100000 .. 0x07fffff   7 MiB
firmware-b  0x0800000 .. 0x0efffff   7 MiB
environment 0x0f00000 .. 0x0ffffff   1 MiB
```

The regions are contiguous and end at 16 MiB. Next compare erase sizes, boot ROM offsets, bootloader configuration, update tooling, and the actual detected flash capacity. Arithmetic alone does not validate the product update design.

## Step 4: Trace MAC-Address Provisioning

Expected path:

```text
I2C controller -> mux address 0x70 -> channel 1 -> EEPROM 0x50
     -> NVMEM bytes 0x10..0x15 -> Ethernet MAC consumer
```

Prove that the mux creates its child adapter, the EEPROM binds, the NVMEM cell is available, and the Ethernet driver reads it. Validate that the resulting address is a unicast address and unique for the unit. Do not “fix” a missing NVMEM supplier by adding a shared fallback address.

## Step 5: Collect Runtime Evidence

```sh
i2cdetect -l
find /sys/bus/i2c/devices -maxdepth 2 -type l -o -type d
find /sys/bus/nvmem/devices -maxdepth 2 -type f
ip -details link show
ethtool eth0
cat /proc/mtd
mtdinfo -a
ls -l /sys/class/leds /sys/class/input
cat /proc/bus/input/devices
dmesg | grep -Ei 'i2c|eeprom|nvmem|ethernet|mdio|phy|spi-nor|mtd|gpio-keys|led'
```

Map runtime instance numbers through OF paths. Avoid assuming the first I²C adapter, network interface, MTD index, or input event number is stable.

## Step 6: Exercise Lifecycle And Boundaries

Test:

1. cold boot with every peripheral left in reset/off by firmware
2. warm reboot to expose inherited-state dependencies
3. Ethernet traffic at all advertised speeds and temperature corners
4. repeated suspend/wake using the button
5. LED behavior through probe and suspend
6. read-only flash identification and partition-boundary checks
7. I²C access after mux idle/suspend cycles
8. failure rollback when EEPROM or PHY supply is unavailable

Do not perform flash erase/write tests on boot or environment regions. Use a dedicated disposable test region and recovery plan if destructive validation is authorized later.

## Step 7: Diagnose Deliberate Faults

### Fault A: EEPROM Is Placed Directly Under `i2c1`

Linux probes address `0x50` on the upstream segment rather than mux channel 1. An upstream device with the same address could respond and mislead the diagnosis. Restore physical hierarchy; do not change the EEPROM address to make the node unique globally.

### Fault B: NVMEM Cell Uses `reg = <0x10>`

The cell provider requires offset and size because it declares one address and one size cell. Schema should reject the tuple. Add the six-byte size rather than modifying parent cell counts.

### Fault C: PHY Uses `phy-mode = "rgmii"`

Link may come up but traffic errors rise because the board relies on PHY-added delays. Compare schematic timing, PHY binding, negotiated link, and CRC counters. Restore `rgmii-id` only after proving neither MAC nor PCB supplies those delays.

### Fault D: `firmware-b` Starts At `0x0700000`

It overlaps the final MiB of slot A. Detect this from interval arithmetic before boot testing. Never rely on partition-parser ordering to make overlap safe.

### Fault E: Wake Button Works While Awake But Not In Suspend

Trace input event generation, IRQ wake enablement, GPIO controller power domain, sleep pinctrl, and physical edge. `wakeup-source` states capability; it cannot keep an unmodeled controller or rail alive.

## Exit Review

The lab is complete when you can provide:

- a parent-address-space decode for every `reg`
- the complete I²C mux/NVMEM/MAC dependency graph
- non-overlapping flash interval proof and erase-boundary review
- RGMII delay ownership and runtime error evidence
- stable subsystem-based identification of every runtime device
- cold/warm boot and suspend/wake results
- root-cause evidence for each deliberate fault

## Authoritative References

- [Linux I2C sysfs topology](https://docs.kernel.org/i2c/i2c-sysfs.html)
- [Linux NVMEM subsystem](https://docs.kernel.org/driver-api/nvmem.html)
- [Linux PHY and phylink documentation](https://docs.kernel.org/networking/sfp-phylink.html)
- [Linux SPI NOR framework](https://docs.kernel.org/driver-api/mtd/spi-nor.html)
- [Linux subsystem drivers using GPIO](https://docs.kernel.org/driver-api/gpio/drivers-on-gpio.html)

## Continue

Proceed to [Graph Bindings And Complex Data Paths](../graph-bindings-and-complex-data-paths.md).

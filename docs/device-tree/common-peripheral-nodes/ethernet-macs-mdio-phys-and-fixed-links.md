---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# Ethernet MACs, MDIO, PHYs, And Fixed Links

An Ethernet interface crosses several contracts: the MAC moves frames, a PCS may encode them, the MAC–PHY interface has timing/electrical rules, MDIO manages an external PHY, and the PHY drives the cable medium. DT must describe the path without treating “Ethernet” as one device.

## External MDIO PHY

The AM642 SK connects both CPSW3G ports to TI DP83867 PHYs. From `k3-am642-sk.dts` (trimmed):

```dts
#include <dt-bindings/net/ti-dp83867.h>

&cpsw_port1 {
        phy-mode = "rgmii-id";
        phy-handle = <&cpsw3g_phy0>;
        status = "okay";
};

&cpsw3g_mdio {
        status = "okay";
        pinctrl-names = "default";
        pinctrl-0 = <&mdio1_pins_default>;

        cpsw3g_phy0: ethernet-phy@0 {
                reg = <0>;
                ti,rx-internal-delay = <DP83867_RGMIIDCTL_2_00_NS>;
                ti,fifo-depth = <DP83867_PHYCR_FIFO_DEPTH_4_B_NIB>;
        };
        /* ethernet-phy@1 is identical */
};
```

The PHY `reg` is its MDIO address, usually set by strap pins sampled during reset. There is no `compatible`: the generic `ethernet-phy-ieee802.3-c22` is the default, and the PHY driver is chosen from the ID registers read over MDIO. The `ti,*` properties are still valid because the DP83867 driver reads them from the node once it binds.

Where the board controls PHY reset, the PHY node owns the line. From `k3-am625-beagleplay.dts`:

```dts
cpsw3g_phy0: ethernet-phy@0 {
        reg = <0>;
        reset-gpios = <&main_gpio0 15 GPIO_ACTIVE_LOW>;
        reset-assert-us = <10000>;
        reset-deassert-us = <50000>;
};
```

Verify strap pulls, reset timing, and whether strap pins change function after sampling. An address that differs between cold and warm boot strongly suggests a strap, reset, or power problem.

## `phy-mode` Is A PCB Contract

`phy-mode` (or `phy-connection-type`) names the MAC-to-PHY interface. For RGMII, `ethernet-controller.yaml` defines the suffix in terms of the PCB: RGMII needs about 2 ns of clock-to-data skew in each direction, and the mode says which delays the board traces provide.

- `rgmii` — the PCB provides both delays
- `rgmii-id` — the PCB provides neither; the MAC and PHY together must add both. The binding calls this the most frequent case.
- `rgmii-rxid` — the PCB provides the TX delay; MAC or PHY adds RX
- `rgmii-txid` — the PCB provides the RX delay; MAC or PHY adds TX

The MAC and PHY drivers split the internal work between themselves. On the AM642 SK, `am65-cpsw-nuss` knows the CPSW always adds the TX delay. It turns `rgmii-id` into `rgmii-rxid` before connecting the PHY, so the DP83867 adds only the RX delay (2.00 ns via `ti,rx-internal-delay`). A DT that says `rgmii` or `rgmii-rxid` triggers `RGMII mode without internal TX delay unsupported; please fix your Device Tree`. Do not cycle through values until packets pass: a doubled delay can work at one temperature and fail at another.

## Fixed Links And In-Band Status

A MAC connected to something without a manageable PHY can use a fixed link. From the J784S4 EVM overlay `k3-j784s4-evm-usxgmii-exp1-exp2.dtso` (trimmed):

```dts
&main_cpsw0_port1 {
        phy-mode = "usxgmii";
        phys = <&cpsw0_phy_gmii_sel 1>, <&serdes2_usxgmii_link>;
        phy-names = "mac", "serdes";
        status = "okay";

        fixed-link {
                speed = <5000>;
                full-duplex;
        };
};
```

Use a fixed link only when link parameters really are fixed and no MDIO-managed PHY exists. Serial interfaces such as SGMII may instead use `managed = "in-band-status"` so link state comes from the PCS control word. Fixed and in-band modes are distinct phylink modes.

## MAC Addresses

The AM64x SoC file gives each CPSW port two address sources:

```dts
cpsw_port1: port@1 {
        reg = <1>;
        ti,mac-only;
        label = "port1";
        phys = <&phy_gmii_sel 1>;
        mac-address = [00 00 00 00 00 00];
        ti,syscon-efuse = <&main_conf 0x200>;
        status = "disabled";
};
```

The all-zero `mac-address` is a placeholder for the bootloader to fill. `am65-cpsw-nuss` first calls `of_get_mac_address()`, which rejects an invalid (all-zero) address and then tries NVMEM. Next it reads the factory MAC from the eFuse block at `main_conf` offset `0x200`. Only if that is also invalid does it log `Use random MAC address`. [MAC Addresses, Serial Numbers, And Board Identity](../boot-time-mutation-and-ownership/mac-addresses-serial-numbers-and-board-identity.md) covers precedence and validation. Do not commit one globally administered address into a shared board DTS.

When the address comes from an NVMEM cell, its byte order and post-processing must match the layout binding. A six-byte value existing in sysfs does not prove the network stack interpreted it correctly.

## Runtime Diagnosis

```sh
ip -details link show
ethtool eth0
ethtool -i eth0
ethtool --show-eee eth0
cat /sys/class/net/eth0/phydev/phy_id 2>/dev/null
dmesg | grep -Ei 'ethernet|mdio|phy|rgmii|link'
```

Separate MAC registration, MDIO discovery, PHY state, negotiation, PCS state, carrier, and packet integrity. Link-up proves negotiation, not error-free timing. Track CRC/alignment errors, retransmissions, negotiated mode, and traffic at temperature and cable extremes.

## Failure Patterns

- PHY address straps are sampled before supplies/reset stabilize.
- MDIO responds but `phy-handle` points to another address.
- RGMII delay is applied by both PCB and PHY—or by neither.
- A fixed link hides a real negotiable PCS relationship.
- MAC address provisioning produces duplicates or reversed bytes.
- PHY reset is shared but modeled as private GPIO ownership.
- Wake-on-LAN is enabled while PHY/IRQ supplies disappear in suspend.

## Authoritative References

- [Linux PHY and phylink documentation](https://docs.kernel.org/networking/sfp-phylink.html)
- [Linux Ethernet controller schema](https://github.com/torvalds/linux/blob/master/Documentation/devicetree/bindings/net/ethernet-controller.yaml)
- [Linux MDIO schema](https://github.com/torvalds/linux/blob/master/Documentation/devicetree/bindings/net/mdio.yaml)
- [Linux Ethernet PHY schema](https://github.com/torvalds/linux/blob/master/Documentation/devicetree/bindings/net/ethernet-phy.yaml)

## Continue

Proceed to [Board Services: LEDs, Keys, Watchdogs, RTC, Hwmon, And NVMEM](board-services-leds-keys-watchdogs-rtc-hwmon-and-nvmem.md).

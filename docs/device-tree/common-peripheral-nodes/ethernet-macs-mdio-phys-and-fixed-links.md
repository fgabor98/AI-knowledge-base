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

```dts
&ethernet0 {
        phy-mode = "rgmii-id";
        phy-handle = <&phy0>;
        nvmem-cells = <&mac_address>;
        nvmem-cell-names = "mac-address";
        status = "okay";

        mdio {
                #address-cells = <1>;
                #size-cells = <0>;

                phy0: ethernet-phy@1 {
                        compatible = "ethernet-phy-ieee802.3-c22";
                        reg = <1>;
                        reset-gpios = <&gpio1 10 GPIO_ACTIVE_LOW>;
                        reset-assert-us = <10000>;
                        reset-deassert-us = <30000>;
                };
        };
};
```

The PHY `reg` is its MDIO address, often selected by strap pins sampled during reset. Verify strap pulls, reset timing, and whether shared pins change function after sampling. An address that differs between cold and warm boot strongly suggests strap/reset/power behavior.

The generic PHY compatible permits PHY-ID discovery where appropriate. Use a device-specific compatible when its binding requires board properties or discovery is unreliable.

## `phy-mode` Is An Electrical Contract

`phy-mode`/`phy-connection-type` describes the MAC-to-PHY interface: MII, RMII, RGMII variants, SGMII, 1000BASE-X, and others. RGMII requires clock/data skew. Suffixes communicate where internal delays are enabled:

- `rgmii` — the PHY adds neither internal delay; the PCB/MAC side must supply the required skew
- `rgmii-id` — PHY adds both RX and TX internal delay
- `rgmii-rxid` — PHY adds RX delay
- `rgmii-txid` — PHY adds TX delay

Board traces and MAC capabilities must supply whatever the selected mode does not. Do not cycle through values until packets pass; double delay can work at one temperature and fail later.

## Fixed Links And In-Band Status

A MAC connected directly to a switch/FPGA port can use a fixed link:

```dts
fixed-link {
        speed = <1000>;
        full-duplex;
        pause;
};
```

Use a fixed link only when link parameters really are fixed and no MDIO-managed PHY exists. Serial interfaces such as SGMII may instead use `managed = "in-band-status"` so link state comes from the PCS control word. Fixed and in-band modes are distinct phylink modes.

## MAC Addresses

A production MAC address may come from an NVMEM cell, dedicated EEPROM, OTP, firmware, or a standard address property. Establish precedence in the driver/binding and ensure unique provisioning. Do not commit one globally administered address into a shared board DTS.

The NVMEM cell's byte order and post-processing must match its layout binding. A six-byte value existing in sysfs does not prove the network stack interpreted it correctly.

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

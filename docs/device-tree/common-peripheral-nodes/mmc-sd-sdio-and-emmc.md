---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# MMC, SD, SDIO, And eMMC

One host-controller binding covers several physically different deployments: removable SD cards, soldered eMMC, and SDIO peripherals. Properties must describe the actual wiring, voltage capabilities, and power sequence—not a desired benchmark mode.

## Removable SD

```dts
&sdhci0 {
        bus-width = <4>;
        cd-gpios = <&gpio2 7 GPIO_ACTIVE_LOW>;
        vmmc-supply = <&reg_sd_card>;
        vqmmc-supply = <&reg_sd_io>;
        pinctrl-names = "default", "state_100mhz", "sleep";
        pinctrl-0 = <&sd_default>;
        pinctrl-1 = <&sd_uhs>;
        pinctrl-2 = <&sd_sleep>;
        status = "okay";
};
```

`vmmc` powers the card; `vqmmc` supplies the I/O signaling domain when the host binding uses those generic supplies. UHS voltage switching requires both host support and a rail that can safely switch. Card detect may be GPIO, native, broken, or absent according to standard properties and wiring.

## Soldered eMMC

```dts
&sdhci1 {
        bus-width = <8>;
        non-removable;
        mmc-hs200-1_8v;
        cap-mmc-highspeed;
        vmmc-supply = <&reg_emmc>;
        vqmmc-supply = <&reg_1v8>;
        status = "okay";
};
```

`non-removable` describes physical attachment. High-speed capability booleans assert that the board, controller, device class, voltage, pin state, and timing support those modes. Do not enable HS200/HS400 because another board using the same SoC does.

eMMC reset, strobe, tuning, and enhanced-strobe properties are binding-dependent. Validate trace topology, signal voltage, drive strength, and tuning across process, voltage, and temperature.

## SDIO Devices

A soldered Wi-Fi module may appear as a child of the MMC host when its binding needs non-discoverable resources. It can require an out-of-band wake IRQ, supplies, clocks, and a power sequence. SDIO function numbers are enumerated by the card; child `reg` encoding follows MMC binding rules, not I²C addressing.

Some platforms use `mmc-pwrseq` providers for legacy sequencing; newer hardware may integrate with the generic power-sequencing subsystem. Follow the host and device binding supported by the target kernel. Do not also request the same reset GPIO in the Wi-Fi driver and sequencer.

## Stable Naming And Boot Media

Aliases such as `mmc0` can influence controller numbering on some platforms, but `/dev/mmcblkN` is not a robust filesystem identity. Boot arguments and mounts should use PARTUUID, filesystem UUID, or another stable identifier.

eMMC hardware boot partitions (`boot0`, `boot1`) are not DT fixed partitions. They are device-defined hardware regions exposed separately by the MMC subsystem. Treat writes as boot-critical operations.

## Runtime Diagnosis

```sh
dmesg | grep -Ei 'mmc|sdhci|sdio|tuning'
ls -l /sys/class/mmc_host
find /sys/bus/mmc/devices -maxdepth 2 -type f
lsblk -o NAME,MAJ:MIN,SIZE,RO,TYPE,FSTYPE,PARTUUID,MOUNTPOINTS
```

Separate card detection, power, command exchange, identification, voltage switching, tuning, and block I/O failures. A card that works only at reduced frequency points toward timing/power/signal integrity but does not identify which one.

## Authoritative References

- [Linux generic MMC controller schema](https://github.com/torvalds/linux/blob/master/Documentation/devicetree/bindings/mmc/mmc-controller.yaml)
- [Linux MMC subsystem documentation](https://docs.kernel.org/driver-api/mmc/index.html)
- [Linux SD/MMC device partitions](https://docs.kernel.org/driver-api/mmc/mmc-dev-parts.html)
- [Linux power-sequencing API](https://docs.kernel.org/driver-api/pwrseq.html)

## Continue

Proceed to [Ethernet MACs, MDIO, PHYs, And Fixed Links](ethernet-macs-mdio-phys-and-fixed-links.md).

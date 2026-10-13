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

From `k3-am625-beagleplay.dts` (trimmed):

```dts
&sdhci1 {
        /* SD/MMC */
        pinctrl-names = "default";
        pinctrl-0 = <&sd_pins_default>;

        vmmc-supply = <&vdd_3v3_sd>;
        vqmmc-supply = <&vdd_sd_dv>;
        disable-wp;
        cd-gpios = <&main_gpio1 48 GPIO_ACTIVE_LOW>;
        cd-debounce-delay-ms = <100>;
        ti,fails-without-test-cd;
        status = "okay";
};
```

`vmmc` powers the card; `vqmmc` supplies the I/O signaling domain, and UHS needs a `vqmmc` rail that can switch to 1.8 V. `disable-wp` says no write-protect switch is wired. Card detect is a GPIO here, so the controller's native CD input is not connected. `ti,fails-without-test-cd` makes `sdhci_am654` set the SDHCI test-CD bits after every reset, so the controller always believes a card is present while the GPIO does the real detection. The SoC description already supplies the AM62x tap delays, `bus-width = <4>`, clocks, and power domain.

## Soldered eMMC

The AM642 EVM enables `sdhci0` with three board lines:

```dts
/* eMMC */
&sdhci0 {
        status = "okay";
        non-removable;
        ti,driver-strength-ohm = <50>;
};
```

The speed capabilities come from the SoC file, `k3-am64-main.dtsi` (trimmed):

```dts
sdhci0: mmc@fa10000 {
        compatible = "ti,am64-sdhci-8bit";
        bus-width = <8>;
        mmc-ddr-1_8v;
        mmc-hs200-1_8v;
        ti,otap-del-sel-hs200 = <0x7>;
        ti,itap-del-sel-ddr52 = <0x3>;
        /* ... */
        status = "disabled";
};
```

`non-removable` describes physical attachment. The high-speed booleans assert that board, controller, device, voltage, pins, and timing support those modes, and the per-mode tap delays are silicon timing data. Because the SoC file sets them, every AM64x board inherits HS200; a board whose eMMC routing cannot meet it must delete the property rather than leave it enabled because the EVM supports it.

Validate trace topology, signal voltage, drive strength, and tuning across process, voltage, and temperature.

## SDIO Devices

BeaglePlay's WL1807 Wi-Fi module sits on `sdhci2`:

```dts
sdio_pwrseq: sdio-pwrseq {
        compatible = "mmc-pwrseq-simple";
        pinctrl-names = "default";
        pinctrl-0 = <&wifi_en_pins_default>;
        /* Internal power on time(Figure 8-3) * 2 */
        post-power-on-delay-ms = <10>;
        /* Re-enable time(Figure 8-2) + 20uS */
        power-off-delay-us = <80>;
        reset-gpios = <&main_gpio0 38 GPIO_ACTIVE_LOW>;
};

&sdhci2 {
        non-removable;
        ti,fails-without-test-cd;
        cap-power-off-card;
        keep-power-in-suspend;
        mmc-pwrseq = <&sdio_pwrseq>;
        #address-cells = <1>;
        #size-cells = <0>;
        status = "okay";

        wlcore: wlcore@2 {
                compatible = "ti,wl1807";
                reg = <2>;
                pinctrl-names = "default";
                pinctrl-0 = <&wifi_wlirq_pins_default>;
                interrupt-parent = <&main_gpio0>;
                interrupts = <41 IRQ_TYPE_EDGE_FALLING>;
        };
};
```

The child exists only because the card cannot describe its out-of-band IRQ. `reg = <2>` is the SDIO function number, not a bus address. The power sequencer owns the enable line (`reset-gpios`, active low); the delays cite the WL18xx datasheet figures they came from. Do not also request the same GPIO in the Wi-Fi driver. Compare the AM642 SK, which models the same kind of enable as the fixed regulator `wlan_en` used as `vmmc-supply` ([Pin, GPIO, And Interrupt Bring-Up Lab](../pinctrl-gpios-and-interrupts/pin-gpio-and-interrupt-bring-up-lab.md)). Both are valid; the binding and driver choose which one a board uses.

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

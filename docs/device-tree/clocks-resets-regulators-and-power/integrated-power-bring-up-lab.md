---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# Integrated Power Bring-Up Lab

This lab reviews two real power paths on the TI AM62x LP SK (`k3-am62-lp-sk.dts`): the SD card slot, which depends on a GPIO-switched supply chain, TI-SCI clocks, and a TI-SCI power domain, and the Cortex-A53 cluster, whose operating points are filtered by eFuse speed grade and throttled by on-die temperature sensors. Five deliberate faults are injected into copies of the DTB.

## Objectives

By the end, you should be able to:

- draw a multi-framework supplier graph from a real DTB
- explain who owns rate, voltage, and domain state on a firmware-managed SoC
- predict which OPPs a given speed grade exposes
- connect VTM sensor channels to zones, trips, and CPU cooling devices
- diagnose five deliberate faults, including ones no build tool reports

## Inputs

- `k3-am62-lp-sk.dtb` built from mainline Linux (`make ARCH=arm64 ti/k3-am62-lp-sk.dtb`, or the manual `cpp` + `dtc` pipeline from the [First Device Tree Lab](../foundations/first-device-tree-lab.md#step-10-build-the-real-board-dtb))
- sources: `k3-am62-lp-sk.dts`, `k3-am62x-sk-common.dtsi`, `k3-am625.dtsi`, `k3-am62-main.dtsi`, `k3-am62-wakeup.dtsi`, `k3-am62-thermal.dtsi`
- drivers: `drivers/cpufreq/ti-cpufreq.c`, `drivers/thermal/k3_j72xx_bandgap.c`, `drivers/regulator/core.c`
- for Steps 5 and 6, an AM62x LP SK running the matching kernel

The arm64 `defconfig` builds `CONFIG_K3_THERMAL` as a module; the TI-SCI clock and power-domain drivers, `CONFIG_ARM_TI_CPUFREQ`, `cpufreq-dt`, `regulator-gpio`, the TPS65219 driver, the PCA953x expander driver, and `sdhci_am654` are built in.

## Step 1: Read The SD Supply Chain

From `k3-am62-lp-sk.dts` (trimmed):

```dts
vcc_3v3_sys: regulator-2 {
        /* output of LM61460-Q1 */
        compatible = "regulator-fixed";
        regulator-name = "vcc_3v3_sys";
        regulator-min-microvolt = <3300000>;
        regulator-max-microvolt = <3300000>;
        vin-supply = <&vmain_pd>;
        regulator-always-on;
        regulator-boot-on;
};

vdd_mmc1: regulator-3 {
        /* TPS22918DBVR */
        compatible = "regulator-fixed";
        regulator-name = "vdd_mmc1";
        regulator-min-microvolt = <3300000>;
        regulator-max-microvolt = <3300000>;
        regulator-boot-on;
        enable-active-high;
        vin-supply = <&vcc_3v3_sys>;
        gpio = <&exp1 3 GPIO_ACTIVE_HIGH>;
};

vddshv_sdio: regulator-4 {
        compatible = "regulator-gpio";
        regulator-name = "vddshv_sdio";
        pinctrl-names = "default";
        pinctrl-0 = <&vddshv_sdio_pins_default>;
        regulator-min-microvolt = <1800000>;
        regulator-max-microvolt = <3300000>;
        regulator-boot-on;
        vin-supply = <&ldo1_reg>;
        gpios = <&main_gpio0 31 GPIO_ACTIVE_HIGH>;
        states = <1800000 0x0>,
                 <3300000 0x1>;
};

&main_i2c0 {
        tps65219: pmic@30 {
                compatible = "ti,tps65219";
                reg = <0x30>;
                ldo1-supply = <&vcc_3v3_sys>;
                /* ... */
                regulators {
                        ldo1_reg: ldo1 {
                                regulator-name = "VDDSHV_SDIO";
                                regulator-min-microvolt = <3300000>;
                                regulator-max-microvolt = <3300000>;
                        };
                };
        };
};

&sdhci1 {
        vmmc-supply = <&vdd_mmc1>;
        vqmmc-supply = <&vddshv_sdio>;
};
```

`exp1` is a TCA6424 at `0x22` on `main_i2c1`; its line 3 is named `MMC1_SD_EN`. The SoC node from `k3-am62-main.dtsi` adds:

```dts
sdhci1: mmc@fa00000 {
        compatible = "ti,am62-sdhci";
        power-domains = <&k3_pds 58 TI_SCI_PD_EXCLUSIVE>;
        clocks = <&k3_clks 58 5>, <&k3_clks 58 6>;
        clock-names = "clk_ahb", "clk_xin";
        /* ... */
};
```

`vmmc` powers the card; `vqmmc` powers the I/O signaling and is switched to 1.8 V by `sdhci_am654_start_signal_voltage_switch()` for UHS modes.

## Step 2: Read The CPU Performance Path

From `k3-am625.dtsi` (trimmed):

```dts
cpu0: cpu@0 {
        compatible = "arm,cortex-a53";
        operating-points-v2 = <&a53_opp_table>;
        clocks = <&k3_clks 135 0>;
        #cooling-cells = <2>;
};

a53_opp_table: opp-table {
        compatible = "operating-points-v2-ti-cpu";
        opp-shared;
        syscon = <&opp_efuse_table>;

        opp-800000000 {
                opp-hz = /bits/ 64 <800000000>;
                opp-supported-hw = <0x01 0x0007>;
                clock-latency-ns = <6000000>;
        };

        opp-1000000000 {
                opp-hz = /bits/ 64 <1000000000>;
                opp-supported-hw = <0x01 0x0006>;
                clock-latency-ns = <6000000>;
        };

        opp-1250000000 {
                opp-hz = /bits/ 64 <1250000000>;
                opp-supported-hw = <0x01 0x0004>;
                clock-latency-ns = <6000000>;
                opp-suspend;
        };
        /* 200, 400, and 600 MHz also use <0x01 0x0007> */
};
```

`opp_efuse_table` is `syscon@18` (`ti,am62-opp-efuse-table`, a single 4-byte register) in the wakeup `CTRL_MMR`. From `k3-am62-thermal.dtsi`, two zones read `wkup_vtm0` channels 0 and 1; each has a 95 °C passive trip mapped to `cpu0`–`cpu3` and a 105 °C critical trip.

## Step 3: Draw The Graph

Derive the edges from the DTB, not the source:

```sh
L=k3-am62-lp-sk.dtb
M=/bus@f0000/mmc@fa00000
for p in vmmc-supply vqmmc-supply clocks power-domains; do
        echo "$p: $(fdtget -tx $L $M $p)"
done
fdtget -tx $L /regulator-4 vin-supply gpios
fdtget -tx $L /opp-table syscon
fdtget -tx $L /thermal-zones/main0-thermal thermal-sensors
fdtget -tx $L /thermal-zones/main0-thermal/cooling-maps/map0 cooling-device
```

Resolve every phandle to a path, then complete:

```text
vmain_pd -> vcc_3v3_sys -> vdd_mmc1 (enable: exp1 line 3)       -> sdhci1 vmmc
vcc_3v3_sys -> TPS65219 ldo1 -> vddshv_sdio (select: GPIO0_31)  -> sdhci1 vqmmc
k3_clks[58, 5] clk_ahb, k3_clks[58, 6] clk_xin                  -> sdhci1
k3_pds[58, exclusive]                                           -> sdhci1
k3_clks[135, 0]                                                 -> cpu0..cpu3 (one shared policy)
opp_efuse_table -> ti-cpufreq -> a53_opp_table                  -> cpufreq-dt
wkup_vtm0[0], wkup_vtm0[1] -> main0/main1 zones -> cpu0..cpu3 cooling devices
```

For each edge, record the provider's `#*-cells`, what each cell means, the owning Linux framework, and the runtime evidence you expect. Note what is absent: the OPPs have no `opp-microvolt` and the CPUs have no `cpu-supply`. On the LP SK, buck1 (`VDD_CORE`) is fixed at 0.75 V, so A53 DVFS here is frequency scaling through TI-SCI, not voltage scaling.

## Step 4: Predict The Speed-Grade Filter

`ti-cpufreq` matches the root compatible `ti,am625`, reads the eFuse field (mask `0x07c0`, shift 6), and translates it with `am625_efuse_xlate()`: K (11) sets bit 0, S (19) bits 0–1, T (20) bits 0–2. An unknown value falls back to bit 0. For AM62x the revision cell is hard-coded to `0x1` through a `soc_device_match()` on the `AM62X` family.

| Speed grade | Version mask | 200–800 MHz | 1.0 GHz | 1.25 GHz |
|---|---|---|---|---|
| K | `0x1` | | | |
| S | `0x3` | | | |
| T | `0x7` | | | |

Fill in the table. An OPP is enabled when both of its `opp-supported-hw` cells share a bit with the corresponding hardware values. The `syscon` node is a single register, so the driver's first read at offset `0x18` fails with `-EIO` and `TI_QUIRK_SYSCON_IS_SINGLE_REG` retries at offset `0x0`.

## Step 5: Collect Runtime Evidence

On the LP SK, with paths adapted to your kernel:

```sh
cat /sys/devices/system/cpu/cpufreq/policy0/scaling_available_frequencies
cat /sys/devices/system/cpu/cpufreq/policy0/related_cpus
cat /sys/devices/system/cpu/cpufreq/policy0/scaling_driver
cat /sys/kernel/debug/regulator/regulator_summary
grep -E 'VDDSHV_SDIO|vddshv_sdio|vdd_mmc1' /sys/kernel/debug/regulator/regulator_summary
cat /sys/kernel/debug/mmc1/ios
for z in /sys/class/thermal/thermal_zone*; do
        echo "$z $(cat $z/type) $(cat $z/temp)"
done
ls /sys/class/thermal/ | grep cooling_device
dmesg | grep -i -E 'cpufreq|syscon|thermal|regulator|mmc1'
```

Expected evidence:

- `scaling_driver` is `cpufreq-dt`, `related_cpus` lists 0–3, and the available frequencies match your speed-grade row
- `vdd_mmc1` and `vddshv_sdio` list `fa00000.mmc` as a consumer; `VDDSHV_SDIO` (ldo1) lists `vddshv_sdio` as its consumer
- with a UHS card, `ios` reports `signal voltage: 1 (1.80 V)` and `regulator_summary` shows `vddshv_sdio` at 1.8 V; with a high-speed-only card, 3.3 V
- `main0-thermal` and `main1-thermal` report plausible millidegree readings
- one `cpufreq-cpu0` cooling device exists, because `opp-shared` gives the four CPUs a single policy and `cpufreq-dt` registers one cooling device per policy; the trip points under each zone match 95 °C and 105 °C

`regulator_summary` shows what the kernel requested. It does not prove the pad voltage; measure the `VDDSHV_SDIO` rail once to anchor the table.

## Step 6: Exercise The Lifecycle

Test:

1. cold boot with an SD card inserted, then hot insertion after boot
2. a UHS card and a high-speed-only card, recording the signaling voltage for each
3. sustained CPU load at each available frequency
4. a passive-trip crossing, if you can heat the package safely, and recovery through the 2 °C hysteresis
5. system suspend and resume, noting that `opp-suspend` marks 1.25 GHz as the suspend frequency on T-grade parts

## Step 7: Diagnose Deliberate Faults

Work on copies of the DTB. The phandles below are from a mainline build (`cpu0` = `0x49`, `wkup_vtm0` = `0x47`, `ldo1_reg` = `0x59`); confirm yours with `fdtget -tx $L <path> phandle`. Run `dtc -I dtb -O dts <copy> >/dev/null` on each copy and note that it accepts all five.

### Fault A: A K-Grade Part Exposes 1.25 GHz

```sh
cp $L a.dtb
fdtput -t x a.dtb /opp-table/opp-1250000000 opp-supported-hw 1 7
```

Nothing fails: the kernel enables 1.25 GHz on K, S, and T parts. Only a comparison of `scaling_available_frequencies` against the device's speed grade (from its eFuse value or part marking) catches this. Explain why neither `dtc` nor the schema can validate the semantic meaning of a version mask, and why an unvalidated OPP is an electrical-safety bug even if the board boots.

### Fault B: The eFuse Syscon Is Removed

```sh
cp $L b.dtb
fdtput -d b.dtb /opp-table syscon
```

`ti_cpufreq_setup_syscon_register()` logs `"syscon" is missing, cannot use OPPv2 table.` and returns before it registers the `cpufreq-dt` device. Predict the result: no `/sys/devices/system/cpu/cpufreq/policy0`, and the CPUs stay at whatever rate firmware left them. Also predict that thermal throttling has no CPU cooling device to bind to.

### Fault C: The Main0 Zone Reads The Wrong Channel

```sh
cp $L c.dtb
fdtput -t x c.dtb /thermal-zones/main0-thermal thermal-sensors 47 1
```

Both zones now name channel 1, and no zone names channel 0. `k3_j72xx_bandgap` registers a zone for each sensor the VTM reports, starting with id 0. Registration for id 0 finds no zone, the driver logs `thermal zone device is NULL`, and the probe fails. Predict the result: no thermal zones at all, so no throttling and no critical shutdown. Explain why a mistake in one zone removes protection from the whole SoC, and why a plausible reading is weaker evidence than tracing every channel to a zone.

### Fault D: The I/O Voltage States Are Swapped

```sh
cp $L d.dtb
fdtput -t x d.dtb /regulator-4 states 1b7740 1 325aa0 0
```

The kernel still reports 1.8 V when it requests 1.8 V, because the reported value comes from the `states` table. Physically, the GPIO now drives the opposite level for every request. Predict the result for card initialization at 3.3 V signaling and for a UHS voltage switch, and explain why only a meter or scope on the rail can catch this. Restore the original states before testing with a valuable card.

### Fault E: The I/O Rail Loses Its Parent

```sh
cp $L e.dtb
fdtput -d e.dtb /regulator-4 vin-supply
```

The GPIO regulator still probes, but `ldo1` now has no consumers. It is neither `regulator-always-on` nor `regulator-boot-on`. About 30 seconds after boot, `regulator_late_cleanup()` disables enabled regulators with no users. If LDO1 was on at handoff, expect `VDDSHV_SDIO: disabling` in the log, followed by SD I/O failures that look like a card or signal-integrity fault. Explain why `vin-supply` is a power-sequencing edge, not documentation, and why `regulator_ignore_unused` only hides the bug.

## Exit Review

You have completed the lab when you can present:

- the resolved supplier graph with every phandle mapped to a path
- a decode of every clock, power-domain, and supply specifier for `sdhci1`
- the speed-grade OPP table with runtime confirmation for your part
- signaling-voltage evidence from both kernel state and a rail measurement
- zone, trip, and cooling-device evidence for both VTM channels
- the predicted and observed outcome for each fault, including which ones `dtc` accepted

## Authoritative References

- [Linux Common Clock Framework](https://docs.kernel.org/driver-api/clk.html)
- [Linux regulator API](https://docs.kernel.org/driver-api/regulator.html)
- [Linux runtime power management](https://docs.kernel.org/power/runtime_pm.html)
- [Linux OPP library](https://docs.kernel.org/power/opp.html)
- [Linux thermal sysfs API](https://docs.kernel.org/driver-api/thermal/sysfs-api.html)
- [TI SCI protocol documentation](https://software-dl.ti.com/tisci/esd/latest/)

## Continue

Proceed to [Common Peripheral Nodes](../common-peripheral-nodes.md).

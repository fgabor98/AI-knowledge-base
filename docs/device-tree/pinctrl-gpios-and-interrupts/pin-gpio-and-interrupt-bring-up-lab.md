---
status: draft
reviewed: false
domain: device-tree
difficulty: intermediate
last_reviewed: null
---

# Pin, GPIO, And Interrupt Bring-Up Lab

## Goal

Trace the AM642 SK Wi-Fi module's enable and interrupt nets through pinctrl, GPIO, regulator, and IRQ domains; predict physical levels; inspect runtime ownership; and diagnose deliberate polarity, trigger, hog, and cascade failures.

## Inputs

- `k3-am642-sk.dtb` and `arch/arm64/boot/dts/ti/k3-am642-sk.dts` from mainline Linux
- `k3-am64-main.dtsi` and `k3-pinctrl.h`
- for Steps 5 and 11, an AM642 SK running the matching kernel
- for Step 9, optionally an AM62x LP SK

## The Real Description

From `k3-am642-sk.dts` (trimmed):

```dts
com8_ls_en: regulator-3 {
        compatible = "regulator-fixed";
        regulator-name = "com8_ls_en";
        regulator-always-on;
        regulator-boot-on;
        pinctrl-0 = <&main_com8_ls_en_pins_default>;
        pinctrl-names = "default";
        gpio = <&main_gpio0 62 GPIO_ACTIVE_LOW>;
};

wlan_en: regulator-4 {
        /* output of SN74AVC4T245RSVR */
        compatible = "regulator-fixed";
        regulator-name = "wlan_en";
        enable-active-high;
        pinctrl-0 = <&main_wlan_en_pins_default>;
        pinctrl-names = "default";
        vin-supply = <&com8_ls_en>;
        gpio = <&main_gpio0 48 GPIO_ACTIVE_HIGH>;
};

&main_pmx0 {
        main_wlan_en_pins_default: main-wlan-en-default-pins {
                pinctrl-single,pins = <
                        AM64X_IOPAD(0x00c4, PIN_OUTPUT_PULLUP, 7) /* (V8) GPIO0_48 */
                >;
        };

        main_com8_ls_en_pins_default: main-com8-ls-en-default-pins {
                pinctrl-single,pins = <
                        AM64X_IOPAD(0x00fc, PIN_OUTPUT, 7) /* (U7) PRG1_PRU0_GPO17.GPIO0_62 */
                >;
        };

        main_wlan_pins_default: main-wlan-default-pins {
                pinctrl-single,pins = <
                        AM64X_IOPAD(0x00bc, PIN_INPUT, 7) /* (U8) GPIO0_46 */
                >;
        };
};

&sdhci0 {
        vmmc-supply = <&wlan_en>;
        /* ... */
        wlcore: wlcore@2 {
                compatible = "ti,wl1837";
                reg = <2>;
                pinctrl-0 = <&main_wlan_pins_default>;
                pinctrl-names = "default";
                interrupt-parent = <&main_gpio0>;
                interrupts = <46 IRQ_TYPE_EDGE_FALLING>;
        };
};
```

`com8_ls_en` enables an on-board level shifter; `wlan_en` drives the WL1837's enable through that shifter and is the SDIO slot's `vmmc` supply.

## Step 1: Capture The Artifact

Record the kernel revision, preprocessor and `dtc` versions, the build command and warnings, the DTB hash, and a decompiled copy. Compilation checks syntax and structure, not the schematic.

## Step 2: Build A Signal Ownership Table

Decode the pad cells from the DTB before booting:

```sh
D=k3-am642-sk.dtb
P=/bus@f4000/pinctrl@f4000
for n in main-wlan-default-pins main-wlan-en-default-pins main-com8-ls-en-default-pins; do
        echo "$n: $(fdtget -tx $D $P/$n pinctrl-single,pins)"
done
```

Expected: `bc 54007`, `c4 20007`, `fc 10007`. The low three bits are mux mode 7 (GPIO); `0x54000` is `PIN_INPUT` (input enable, Schmitt trigger, no pull), `0x20000` is `PIN_OUTPUT_PULLUP`, and `0x10000` is `PIN_OUTPUT` (pull disabled).

| Net | Ball | Pad offset | Pad config | GPIO provider/line | Logical polarity | IRQ provider/line | Trigger |
|---|---|---|---|---|---|---|---|
| WLAN IRQ | U8 | | | | n/a | | |
| WLAN_EN | V8 | | | | | n/a | n/a |
| COM8 level-shifter enable | U7 | | | | | n/a | n/a |

Explain why GPIO line 46 and interrupt line 46 refer to the same controller-local line of `main_gpio0` but use different flag namespaces.

## Step 3: Predict Enable Levels

Fill the truth table:

| Regulator | Logical state | Physical level on the GPIO |
|---|---|---|
| `wlan_en` | enabled | |
| `wlan_en` | disabled | |
| `com8_ls_en` | enabled | |
| `com8_ls_en` | disabled | |

For `regulator-fixed`, polarity comes **only** from `enable-active-high`. `drivers/gpio/gpiolib-of.c` has a quirk table that ignores the GPIO specifier's flag for this compatible. So `wlan_en` drives high when enabled, and `com8_ls_en`, which lacks `enable-active-high`, drives low when enabled; its `GPIO_ACTIVE_LOW` flag merely agrees.

## Step 4: Trace The Interrupt Cascade

Write the complete route:

```text
WL1837 IRQ output
→ pad U8 (offset 0x0bc, GPIO mux, input enabled)
→ main_gpio0 line 46, falling edge
→ main_gpio0 bank interrupt (16 lines per bank: line 46 is in bank 2 → router input 192)
→ main_gpio_intr (TI-SCI interrupt router)
→ one of 16 router outputs mapped by ti,interrupt-ranges = <0 32 16> onto GIC inputs
→ Linux virtual IRQ
→ wlcore handler
```

TI-SCI firmware programs which router output a bank uses at runtime. Note the two independent trigger descriptions: the WLAN child is falling-edge at the GPIO line, while `main_gpio_intr` declares `ti,intr-trigger-type = <1>` (edge, per `ti,sci-intr.yaml`) for every GIC input it drives. Explain why each controller input needs its own trigger type.

## Step 5: Inspect Runtime State

On the AM642 SK, collect read-only evidence:

```sh
gpioinfo | grep -E 'line +(46|48|62):'
grep -i -E 'wl|gpio' /proc/interrupts
cat /sys/kernel/debug/pinctrl/f4000.pinctrl-pinctrl-single/pinmux-pins | grep -E 'pin (47|49|63) '
cat /sys/kernel/debug/regulator/regulator_summary | grep -E 'wlan_en|com8'
dmesg | grep -i -E 'wlcore|wl18|enforce active low'
```

Pad names and debugfs paths vary by kernel; locate them with `ls /sys/kernel/debug/pinctrl/`. Map line names, consumer labels, GPIO offsets, hardware IRQs, and Linux IRQs into separate columns. Expect the boot log to contain `/regulator-3 enforce active low on GPIO handle`, because `com8_ls_en` has no `enable-active-high`.

Do not use `gpioset` on kernel-owned lines.

## Step 6: Break Polarity Two Ways

On copies of the DTB:

```sh
cp $D p1.dtb
fdtput -t x p1.dtb /regulator-4 gpio 16 30 1      # GPIO_ACTIVE_LOW on wlan_en
cp $D p2.dtb
fdtput -d p2.dtb /regulator-4 enable-active-high  # remove the polarity property
```

`16` is the observed phandle of `main_gpio0`; confirm yours first. Predict the result of booting each:

- `p1.dtb`: no behavioral change; the kernel logs `... GPIO handle specifies active low - ignored`.
- `p2.dtb`: the enable really inverts. The regulator now drives low to “enable,” the WL1837 stays powered down, and SDIO enumeration of `wlcore@2` fails.

Explain why reading the GPIO flag alone is not enough to know the electrical polarity for this compatible.

## Step 7: Substitute The Wrong Flag Namespace

In a scratch copy of `k3-am642-sk.dts`, change:

```dts
interrupts = <46 GPIO_ACTIVE_LOW>;
```

After preprocessing this is `<46 1>`, and in the IRQ namespace `1` is `IRQ_TYPE_EDGE_RISING`, not “active low.” Predict the missed or late interrupts. Restore `IRQ_TYPE_EDGE_FALLING`.

## Step 8: Create A Hog Conflict

In a scratch copy, add a hog on the enable line:

```dts
&main_gpio0 {
        wlan-en-hog {
                gpio-hog;
                gpios = <48 GPIO_ACTIVE_HIGH>;
                output-low;
                line-name = "WLAN_EN_HOG";
        };
};
```

The hog and `wlan_en` now compete for one line. Record compile/schema diagnostics, then predict that the regulator's GPIO request fails with `-EBUSY` and its probe fails. Explain why no compiler can generally prove such runtime conflicts.

## Step 9: Break A Wake Path

AM64x SK does not use pad wake-up, so use the AM62x LP SK's MCU CAN wake state (`k3-am62-lp-sk.dts`):

```dts
mcu_mcan0_rx_pins_wakeup: mcu-mcan0-rx-wakeup-pins {
        pinctrl-single,pins = <
                AM62X_IOPAD(0x038, PIN_INPUT | PIN_WKUP_EN, 0) /* (B3) MCU_MCAN0_RX */
        >;
};
```

Remove `| PIN_WKUP_EN` in a scratch copy. The `wakeup-source` property and the `wakeup` pinctrl state still exist, so everything looks configured, but the pad no longer arms its wake detector. On the hardware, test CAN traffic before, during, and after suspend entry, and record wake reason and IRQ counters.

## Step 10: Break The Cascade Parent

```sh
cp $D c.dtb
fdtput -d c.dtb /bus@f4000/gpio@600000 interrupts
dtc -I dtb -O dts c.dtb >/dev/null
```

`dtc` reports nothing. Predict which stages still succeed: pinctrl selection, GPIO controller registration, the `wlan_en` GPIO request, child IRQ-domain creation, parent IRQ acquisition, and WLAN IRQ delivery.

## Step 11: Test Trigger And Acknowledge Behavior

With the WL1837 active (for example during a Wi-Fi scan), record:

1. inactive voltage on the IRQ pad
2. assertion edge
3. GPIO bank pending state, if exposed
4. GPIO bank parent count in `/proc/interrupts`
5. `wlcore` Linux IRQ count
6. device status before and after the handler
7. deassertion timing

Classify no-event, one-shot, duplicate, and storm failures without changing flags by trial and error.

## Completion Criteria

The lab is complete when you can provide:

- reproducible DTB identity and final source
- a complete signal ownership table with decoded pad values
- a logical-to-physical truth table that accounts for the `regulator-fixed` quirk
- the GPIO → router → GIC interrupt route with trigger explanation
- runtime pinctrl, GPIO, regulator, and IRQ evidence
- predicted and observed outcomes for each deliberate failure
- proof of safe restoration

## Authoritative References

- [Linux pin control subsystem](https://docs.kernel.org/driver-api/pin-control.html)
- [Linux GPIO consumer interface](https://docs.kernel.org/driver-api/gpio/consumer.html)
- [Linux GPIO irqchip provider interface](https://docs.kernel.org/driver-api/gpio/driver.html)
- [Linux IRQ-domain documentation](https://docs.kernel.org/core-api/irq/irq-domain.html)

## Continue

Proceed to [Clocks, Resets, Regulators, And Power](../clocks-resets-regulators-and-power.md).

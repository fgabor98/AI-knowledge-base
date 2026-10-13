---
status: draft
reviewed: false
domain: device-tree
difficulty: intermediate
last_reviewed: null
---

# Board Services: LEDs, Keys, Watchdogs, RTC, Hwmon, And NVMEM

Small board-service devices deserve the same rigor as high-speed peripherals. Standard subsystem bindings provide stable kernel abstractions and prevent raw GPIO, register, or EEPROM access from leaking into applications.

## LEDs

From `k3-am625-beagleplay.dts` (trimmed):

```dts
leds {
        compatible = "gpio-leds";

        led-0 {
                gpios = <&main_gpio0 3 GPIO_ACTIVE_HIGH>;
                linux,default-trigger = "heartbeat";
                function = LED_FUNCTION_HEARTBEAT;
                default-state = "off";
        };

        led-1 {
                gpios = <&main_gpio0 4 GPIO_ACTIVE_HIGH>;
                linux,default-trigger = "disk-activity";
                function = LED_FUNCTION_DISK_ACTIVITY;
                default-state = "keep";
        };

        led-2 {
                gpios = <&main_gpio0 5 GPIO_ACTIVE_HIGH>;
                function = LED_FUNCTION_CPU;
        };
        /* led-3: LED_FUNCTION_LAN, led-4: LED_FUNCTION_WLAN */
};
```

`color` and `function` build the LED class name as `<color>:<function>`. BeaglePlay sets no `color`, so `led_compose_name()` produces names such as `:heartbeat` under `/sys/class/leds/`. `default-state = "keep"` on `led-1` keeps whatever the bootloader left, which is intentional because U-Boot also drives these LEDs (all carry `bootph-all`). `linux,default-trigger` is Linux policy: BeaglePlay chose a heartbeat and disk activity, but a product could equally set triggers from user space.

For PWM or multicolor hardware, use the matching LED binding rather than approximating it with GPIO nodes.

## Keys And Buttons

```dts
gpio_keys: gpio-keys {
        compatible = "gpio-keys";
        autorepeat;
        pinctrl-names = "default";
        pinctrl-0 = <&usr_button_pins_default>;

        usr: button-usr {
                label = "User Key";
                linux,code = <BTN_0>;
                gpios = <&main_gpio0 18 GPIO_ACTIVE_LOW>;
        };
};
```

`linux,code` selects the input event code; `GPIO_ACTIVE_LOW` says the button pulls the line low when pressed. With no `debounce-interval`, `gpio-keys` uses its 5 ms default. There is no `wakeup-source`, so this button does not wake the board. Adding one is valid only if the GPIO IRQ and its power path remain functional during sleep; on AM62x that also needs a pad configured with `PIN_WKUP_EN` (see [Sleep States, Wakeup, And Ownership](../pinctrl-gpios-and-interrupts/sleep-states-wakeup-and-ownership.md)).

Do not export the same line as a userspace GPIO. The input driver owns it and provides events, debouncing, and wake integration.

## Watchdogs

The watchdog node lives in the SoC description. From `k3-am62-main.dtsi`:

```dts
main_rti0: watchdog@e000000 {
        compatible = "ti,j7-rti-wdt";
        reg = <0x00 0x0e000000 0x00 0x100>;
        clocks = <&k3_clks 125 0>;
        power-domains = <&k3_pds 125 TI_SCI_PD_EXCLUSIVE>;
        assigned-clocks = <&k3_clks 125 0>;
        assigned-clock-parents = <&k3_clks 125 2>;
};
```

The node has no `status`, so it is enabled on every AM62x board. Two properties of the RTI hardware shape policy, and neither appears in DT:

- it is windowed. `rti_wdt` selects a 50% open window, so a ping earlier than about 52% of the timeout is rejected as too early, not just a late one
- `rti_wdt_ops` has no `.stop`. Once started, by Linux or inherited from the bootloader (`WDOG_HW_RUNNING`), it cannot be disabled

Validate cold boot, bootloader-to-kernel handoff, probe failure, suspend, shutdown, and deliberate userspace failure. A bootloader-started watchdog must be serviced before its inherited timeout expires.

```sh
wdctl
cat /sys/class/watchdog/watchdog0/status 2>/dev/null
```

## RTCs

Boards can expose several clocks. AM62x has an on-chip RTC, and BeaglePlay also has an external I²C RTC on `main_i2c0`:

```dts
rtc: rtc@68 {
        compatible = "ti,bq32000";
        reg = <0x68>;
        interrupt-parent = <&main_gpio0>;
        interrupts = <2 IRQ_TYPE_EDGE_FALLING>;
};
```

Marking a node available does not choose the authoritative wall clock. Verify backup supply, oscillator, alarm IRQ, wake routing, century handling, and userspace synchronization. Use `/sys/class/rtc/rtc*/name` and `hwclock` to map runtime instances; numbering can change. Test time retention with main power removed, not only reboot.

## Hardware Monitoring

Hwmon devices report voltages, currents, temperatures, and fans through standardized sysfs units. The AM642 SK's two TMP100 sensors (`tmp100@48`, `tmp100@49` on `main_i2c1`) need only `compatible` and `reg`. Current monitors such as the INA2xx family also need `shunt-resistor` in micro-ohms, which is board calibration data.

```sh
for path in /sys/class/hwmon/hwmon*; do
        cat "$path/name"
done
sensors
```

A wrong shunt value yields consistent but wrong current readings. Validate against instruments across the useful range. Hwmon exposure is not thermal control: the TMP100s report temperatures but take part in no thermal zone, unlike the VTM sensors in [Thermal Zones](../clocks-resets-regulators-and-power/thermal-zones-trips-and-cooling-maps.md).

## NVMEM Providers And Cells

Kontron's SA67 board stores its MAC address in the OTP area of its SPI NOR flash. From `k3-am67a-kontron-sa67-base.dts` (trimmed):

```dts
otp-1 {
        compatible = "user-otp";

        nvmem-layout {
                compatible = "kontron,sa67-vpd", "kontron,sl28-vpd";

                serial_number: serial-number {
                };

                base_mac_address: base-mac-address {
                        #nvmem-cell-cells = <1>;
                };
        };
};

&cpsw_port1 {
        phy-connection-type = "rgmii-id";
        phy-handle = <&phy0>;
        nvmem-cells = <&base_mac_address 0>;
        nvmem-cell-names = "mac-address";
};
```

NVMEM cells expose semantic fields without making consumers know the storage device or offset. Here the cells have no `reg`. The `sl28vpd` layout driver knows the offsets, checks a CRC, and rejects an invalid Ethernet address. `#nvmem-cell-cells = <1>` lets each consumer pass an index: `sl28vpd_mac_address_pp()` adds it to the base address, so port 1 uses index 0 and a second port could use index 1. With a fixed layout, cells use `reg = <offset size>` in the provider's byte space, not the I²C or SPI address.

Protect calibration, identity, and keys. A writable provider sysfs file is not automatically an acceptable production provisioning interface. Define write protection, lifecycle state, access controls, redundancy, and atomic update behavior.

## Authoritative References

- [Linux subsystem drivers using GPIO](https://docs.kernel.org/driver-api/gpio/drivers-on-gpio.html)
- [Linux LED class documentation](https://docs.kernel.org/leds/leds-class.html)
- [Linux watchdog API](https://docs.kernel.org/watchdog/watchdog-api.html)
- [Linux hwmon sysfs conventions](https://docs.kernel.org/hwmon/sysfs-interface.html)
- [Linux NVMEM subsystem](https://docs.kernel.org/driver-api/nvmem.html)

## Continue

Proceed to [MTD, SPI NOR, And Fixed Partitions](mtd-spi-nor-and-fixed-partitions.md).

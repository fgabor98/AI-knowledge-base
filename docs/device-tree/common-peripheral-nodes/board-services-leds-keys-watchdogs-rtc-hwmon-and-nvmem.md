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

```dts
#include <dt-bindings/leds/common.h>

leds {
        compatible = "gpio-leds";

        led-status {
                function = LED_FUNCTION_STATUS;
                color = <LED_COLOR_ID_GREEN>;
                gpios = <&gpio2 12 GPIO_ACTIVE_HIGH>;
                default-state = "off";
        };
};
```

Function and color create meaningful LED class names. `default-state` describes safe initial behavior; `keep` should be used only when inherited state is intentional. A `linux,default-trigger` is Linux policy and should be justified—many products are better configured from user space.

For PWM or multicolor hardware, use the appropriate LED binding rather than approximating it with GPIO nodes.

## Keys And Buttons

```dts
#include <dt-bindings/input/input.h>

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

`linux,code` selects the input event code, while polarity describes electrical assertion. Debounce must suit switch behavior and driver capability. `wakeup-source` is valid only if the GPIO IRQ and power path remain functional during sleep.

Do not export the same line as a userspace GPIO. The input driver owns it and provides events, debouncing, and wake integration.

## Watchdogs

A watchdog node usually lives in the SoC description with its registers, clock, reset, and interrupt. Board properties may define a hardware timeout or external enable according to its binding. DT describes watchdog hardware; user-space daemon policy and `nowayout` behavior are separate.

Validate cold boot, bootloader-to-kernel handoff, probe failure, suspend, shutdown, and deliberate userspace failure. A bootloader-started watchdog must be serviced or safely reconfigured before its inherited timeout expires.

```sh
wdctl
cat /sys/class/watchdog/watchdog0/status 2>/dev/null
```

## RTCs

Boards can expose several clocks (SoC RTC, PMIC RTC, external I²C RTC). Marking a node available does not choose the authoritative wall clock universally. Verify backup supply, oscillator, alarm IRQ, wake routing, century handling, and userspace synchronization.

Use `/sys/class/rtc/rtc*/name` and `hwclock` to map runtime instances; numbering can change. Test time retention with main power removed, not only reboot.

## Hardware Monitoring

Hwmon devices report voltages, currents, temperatures, and fans through standardized sysfs units. Device-specific bindings may require shunt resistance, divider values, labels, or channel configuration. These are board calibration facts.

```sh
for path in /sys/class/hwmon/hwmon*; do
        cat "$path/name"
done
sensors
```

A wrong shunt value yields consistent but wrong current readings. Validate against instruments across the useful range. Do not confuse hwmon sensor exposure with thermal-zone control; a sensor participates in thermal policy only when the relevant framework relationship exists.

## NVMEM Providers And Cells

```dts
eeprom@50 {
        compatible = "atmel,24c64";
        reg = <0x50>;
        #address-cells = <1>;
        #size-cells = <1>;

        mac_address: mac-address@10 {
                reg = <0x10 0x6>;
        };
};

&ethernet0 {
        nvmem-cells = <&mac_address>;
        nvmem-cell-names = "mac-address";
};
```

NVMEM cells expose semantic fields without making consumers know the storage device or offset. Cell `reg` is offset and size in the provider's NVMEM address space, not the I²C address. Layout drivers can parse dynamic formats when fixed offsets are insufficient.

Protect calibration, identity, and keys. A writable provider sysfs file is not automatically an acceptable production provisioning interface. Define write protection, lifecycle state, access controls, redundancy, and atomic update behavior.

## Authoritative References

- [Linux subsystem drivers using GPIO](https://docs.kernel.org/driver-api/gpio/drivers-on-gpio.html)
- [Linux LED class documentation](https://docs.kernel.org/leds/leds-class.html)
- [Linux watchdog API](https://docs.kernel.org/watchdog/watchdog-api.html)
- [Linux hwmon sysfs conventions](https://docs.kernel.org/hwmon/sysfs-interface.html)
- [Linux NVMEM subsystem](https://docs.kernel.org/driver-api/nvmem.html)

## Continue

Proceed to [MTD, SPI NOR, And Fixed Partitions](mtd-spi-nor-and-fixed-partitions.md).

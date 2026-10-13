---
status: draft
reviewed: false
domain: device-tree
difficulty: intermediate
last_reviewed: null
---

# GPIO Controllers, Ranges, Line Names, And Hogs

A GPIO controller provides a local line-offset namespace. Its binding defines each GPIO specifier, its relationship to physical pins, which offsets exist, and whether lines can also provide interrupts.

## Provider Contract

```dts
/* k3-am62-main.dtsi */
main_gpio0: gpio@600000 {
        compatible = "ti,am64-gpio", "ti,keystone-gpio";
        reg = <0x0 0x00600000 0x0 0x100>;
        gpio-ranges = <&main_pmx0  0  0 32>,
                      <&main_pmx0 32 33 38>,
                      <&main_pmx0 70 72 22>;
        gpio-controller;
        #gpio-cells = <2>;
        ti,ngpio = <92>;
        ti,davinci-gpio-unbanked = <0>;
        /* interrupts, power-domains, clocks ... */
};
```

`gpio-controller` and `#gpio-cells` were introduced in [GPIO And Interrupt Relationships](../provider-consumer-relationships/gpio-and-interrupt-relationships.md); this page covers the rest of the provider contract.

The generic `ngpios` property limits valid offsets when a bank is smaller than its register layout. TI's DaVinci/Keystone GPIO binding (`gpio-davinci.yaml`) predates it and requires the vendor property `ti,ngpio` instead; with `additionalProperties: false`, generic `ngpios` is not even accepted there. The same SoC IP reports 87 lines on AM64x `main_gpio0` and 92 on AM62x, so the count is integration data in the SoC DTSI. Gaps can be described with `gpio-reserved-ranges` where the binding permits it. Consumers must not target fused-off, unavailable, or package-unbonded lines.

There is no stable global GPIO number encoded here. Linux GPIO chip numbering can change with probe order. Use provider identity plus controller-local offset, line names, and consumer relationships.

## Line Names

`gpio-line-names` assigns board-facing names by offset. The AM62x LP SK TCA6424 expander (`k3-am62-lp-sk.dts`, trimmed):

```dts
exp1: gpio@22 {
        compatible = "ti,tca6424";
        reg = <0x22>;
        gpio-controller;
        #gpio-cells = <2>;
        gpio-line-names = "GPIO_CPSW2_RST", "GPIO_CPSW1_RST",
                          "PRU_DETECT", "MMC1_SD_EN",
                          /* ... */
                          "MCASP1_FET_SEL", "UART1_FET_SEL",
                          "", "IO_EXP_TEST_LED";
};
```

Empty strings preserve positional gaps: line 22 is unnamed so that `IO_EXP_TEST_LED` stays at offset 23. Names should identify the schematic net, as these do, not the current Linux consumer. A line named `MMC1_SD_EN` can later show a consumer label such as `vdd_mmc1`; those are different fields.

Line names improve `gpioinfo` and diagnostics but do not reserve lines, create userspace ABI, or replace consumer phandles. Treat name changes as an operations/tooling consideration even though drivers should not locate fixed board wiring by arbitrary string search.

## GPIO-To-Pin Ranges

When pinctrl and GPIO number spaces differ, `gpio-ranges` maps them:

The AM62x `main_gpio0` above uses three entries of the form `<&pinctrl gpio-offset pin-offset count>`: GPIO 0–31 map to pads 0–31, GPIO 32–69 to pads 33–70, and GPIO 70–91 to pads 72–93. The skipped pads (32 and 71) have no GPIO function. The exact tuple is defined by the common GPIO and provider bindings; named range forms also exist.

This link allows gpiolib to coordinate GPIO requests with pinmux ownership. Without it, a descriptor may be requested successfully while the pad remains muxed to another peripheral, or conflicts may not be detected.

## GPIO Hogs

A hog is a child of the GPIO controller that claims and configures a line when the controller registers. The BeaglePlay OV5640 camera overlay drives a connector GPIO high so the module is enabled (`k3-am625-beagleplay-csi2-ov5640.dtso`):

```dts
&main_gpio0 {
        p11-hog {
                /* P11 - CSI2_CAMERA_GPIO1 */
                gpio-hog;
                gpios = <11 GPIO_ACTIVE_HIGH>;
                output-high;
                line-name = "CSI2_CAMERA_GPIO1";
        };
};
```

The `gpios` property is provider-local because the hog is already below its controller; it does not contain a phandle. The binding selects one direction/state such as `input`, `output-low`, or `output-high`.

Good hog uses include immutable straps, safe board defaults with no owning device driver, and lines that must be claimed as soon as the controller appears. Poor uses include:

- reset or enable sequencing owned by a device driver
- lines that later need dynamic control
- working around a missing regulator, reset, LED, or power-sequencing binding
- claiming a pin merely to stop userspace from touching it

A hog and a normal consumer cannot own the same line simultaneously. Such conflicts often appear only at runtime.

## Combined GPIO And IRQ Providers

Many GPIO blocks also provide an interrupt domain with a separate `#interrupt-cells` contract ([A GPIO Controller Can Also Be An Interrupt Controller](../provider-consumer-relationships/gpio-and-interrupt-relationships.md#a-gpio-controller-can-also-be-an-interrupt-controller)). The controller may also consume parent interrupts while providing child interrupts; that cascade is covered in [GPIO Interrupts, Cascades, And Shared Lines](gpio-interrupts-cascades-and-shared-lines.md).

## Expanders And Sleepable Controllers

GPIO expanders on I2C or SPI register after their transport controller and can probe late. Their consumers may defer. Access can sleep, so drivers must use `_cansleep` GPIO APIs where required; the descriptor API handles this distinction through appropriate calls.

An expander losing power during suspend also loses output and interrupt state unless hardware retains it or the driver restores it. Pin-level behavior therefore depends on regulator, reset, bus, and PM ordering as well as its DT GPIO flags.

## Runtime Inspection

Read-only GPIO character-device tooling is preferable for inventory:

```sh
gpiodetect
gpioinfo
```

The output can show chip labels, line offsets, line names, consumers, direction, and active-low state. Tool syntax and fields vary by libgpiod version.

Do not request a line with `gpioset` merely to inspect it: line requests change ownership and may change direction/value. Kernel-owned reset, enable, power, or security lines must not be taken from userspace.

Debugfs may expose additional GPIO state, but it is not a stable ABI and often reports requested rather than measured electrical behavior.

## Review Checklist

- Is `#gpio-cells` decoded using the exact provider binding?
- Do `ngpios` and reserved ranges match package availability?
- Do line-name positions include explicit empty gaps?
- Does every GPIO-to-pin range map the intended offsets and pins?
- Are hogs limited to genuinely controller-owned static policy?
- Can the same line be claimed by a hog, consumer, LED, or IRQ path twice?
- Can an expander be accessed from the driver's execution context?
- Does runtime inventory agree with the schematic and final DTB?

## Authoritative References

- [Linux GPIO provider interface](https://docs.kernel.org/driver-api/gpio/driver.html)
- [Linux GPIO subsystem documentation](https://docs.kernel.org/driver-api/gpio/index.html)
- [Linux pinctrl and GPIO ranges](https://docs.kernel.org/driver-api/pin-control.html)
- [Linux GPIO userspace character-device API](https://docs.kernel.org/userspace-api/gpio/chardev.html)

## Next Step

Continue with [GPIO Consumers, Polarity, And Reset Sequencing](gpio-consumers-polarity-and-reset-sequencing.md).

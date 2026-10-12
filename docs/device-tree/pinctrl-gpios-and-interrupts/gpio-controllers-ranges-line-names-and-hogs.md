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
gpio0: gpio-controller@1000 {
        compatible = "example,trainer-gpio";
        reg = <0x1000 0x100>;
        gpio-controller;
        #gpio-cells = <2>;
        ngpios = <16>;
};
```

`gpio-controller` and `#gpio-cells` were introduced in [GPIO And Interrupt Relationships](../provider-consumer-relationships/gpio-and-interrupt-relationships.md); this page covers the rest of the provider contract.

`ngpios` limits valid offsets when the hardware bank is smaller than its register layout's natural width. Gaps can be described with `gpio-reserved-ranges` where the common binding permits it. Consumers must not target fused-off, unavailable, or package-unbonded lines.

There is no stable global GPIO number encoded here. Linux GPIO chip numbering can change with probe order. Use provider identity plus controller-local offset, line names, and consumer relationships.

## Line Names

`gpio-line-names` assigns board-facing names by offset:

```dts
gpio-line-names =
        "status-led",       /* 0 */
        "module-reset",     /* 1 */
        "",                 /* 2 */
        "service-button";   /* 3 */
```

Empty strings preserve positional gaps. Names should identify the schematic net or board function, not the current Linux consumer. A line named `module-reset` can later show a consumer label such as `reset`; those are different fields.

Line names improve `gpioinfo` and diagnostics but do not reserve lines, create userspace ABI, or replace consumer phandles. Treat name changes as an operations/tooling consideration even though drivers should not locate fixed board wiring by arbitrary string search.

## GPIO-To-Pin Ranges

When pinctrl and GPIO number spaces differ, `gpio-ranges` maps them:

```dts
gpio-ranges = <&pinctrl 0 32 16>;
```

In the common linear form, this maps GPIO offsets 0–15 to pinctrl pins 32–47. The exact tuple is defined by the common GPIO and provider bindings; named or sparse range forms may also exist.

This link allows gpiolib to coordinate GPIO requests with pinmux ownership. Without it, a descriptor may be requested successfully while the pad remains muxed to another peripheral, or conflicts may not be detected.

## GPIO Hogs

A hog is a child of the GPIO controller that claims and configures a line when the controller registers:

```dts
safe_enable_hog: safe-enable-hog {
        gpio-hog;
        gpios = <12 GPIO_ACTIVE_HIGH>;
        output-low;
        line-name = "expansion-enable";
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

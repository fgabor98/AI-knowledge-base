---
status: draft
reviewed: false
domain: device-tree
difficulty: intermediate
last_reviewed: null
---

# UARTs, Consoles, And Serial Children

A UART node describes the controller and its fixed wiring. Console selection, tty naming, baud configuration, pin multiplexing, and attached serial devices are related but distinct contracts.

## Controller Resources

SoC controller nodes normally come from the SoC `.dtsi`; a board enables and wires them. From `k3-am625-beagleplay.dts`:

```dts
&main_uart5 {
        pinctrl-names = "default";
        pinctrl-0 = <&mikrobus_uart_pins_default>;
        status = "okay";
};

&main_uart1 {
        /* Main UART1 is used by TIFS firmware */
        status = "reserved";
};
```

`status = "reserved"` means the hardware exists and is in use, but not by this operating system. Linux skips it like a disabled node, while tools and other software can still see that it is taken. Use it when firmware or another core owns a controller, rather than deleting the node or silently disabling it.

The inherited node may already define `compatible`, `reg`, interrupts, clocks, resets, DMA channels, and quirks. Do not copy these into the board file unless board wiring changes a binding-defined property. Review the final merged node, not the amendment alone.

## Console Selection

`/chosen/stdout-path` and its `serial0` alias are covered in [Chosen And Boot Handoff](../standard-nodes-and-properties/chosen-and-boot-handoff.md). The alias does not universally guarantee `/dev/ttyS0`; driver type, probe order, command-line console parameters, and alias handling determine tty naming, so confirm the boot log's actual console registration.

Bootloader and kernel must agree on pins, clock, format, and ownership during handoff. Garbled early output often indicates a source clock or divisor mismatch, not a bad baud string. Silence after early console can mean the full driver changed pinctrl, reset, or clock state.

## Hardware Flow Control

Properties such as `uart-has-rtscts` are binding-specific declarations that RTS/CTS are wired. They do not create missing pins. From `k3-am642-hummingboard-t.dts`:

```dts
main_uart3_default_pins: main-uart3-default-pins {
        pinctrl-single,pins = <
                AM64X_IOPAD(0x016c, PIN_INPUT, 10) /* PRG0_PRU0_GPO3.UART3_CTSn */
                AM64X_IOPAD(0x0170, PIN_OUTPUT, 10) /* PRG0_PRU0_GPO4.UART3_TXD */
                AM64X_IOPAD(0x0174, PIN_OUTPUT, 10) /* PRG0_PRU0_GPO5.UART3_RTSn */
                AM64X_IOPAD(0x01ac, PIN_INPUT, 10) /* PRG0_PRU0_GPO19.UART3_RXD */
        >;
};

&main_uart3 {
        pinctrl-names = "default";
        pinctrl-0 = <&main_uart3_default_pins>;
        uart-has-rtscts;
        linux,rs485-enabled-at-boot-time;
        status = "okay";
};
```

The pin group muxes all four signals, so the claim is backed by pinctrl. If RTS or CTS is repurposed as GPIO, do not claim hardware flow control. A user-space choice to enable flow control is policy; whether the lines physically exist is hardware description.

## Devices Attached To A UART

Bluetooth, GNSS, radio co-processors, and other fixed serial peripherals may be child nodes when their binding defines the relationship. BeaglePlay's CC1352P7 wireless MCU sits on `main_uart6`:

```dts
&main_uart6 {
        pinctrl-names = "default";
        pinctrl-0 = <&wifi_debug_uart_pins_default>;
        status = "okay";

        mcu {
                compatible = "ti,cc1352p7";
                bootloader-backdoor-gpios = <&main_gpio0 13 GPIO_ACTIVE_HIGH>;
                reset-gpios = <&main_gpio0 14 GPIO_ACTIVE_HIGH>;
                vdds-supply = <&vdd_3v3>;
        };
};
```

The child makes the UART a serdev bus, so no `/dev/ttyS*` appears for it; the `gb-beagleplay` driver owns the port instead. The child also carries resources the UART knows nothing about: a supply, a reset line, and a boot-mode pin.

Check the GPIO flags against the driver, not just the binding. To enter the CC1352 ROM bootloader, `gb-beagleplay` drives `reset` to logical 0, waits, then sets it to 1 to release the chip. Logical 1 therefore means “running,” which is why BeaglePlay marks the reset line `GPIO_ACTIVE_HIGH`, even though the binding's example uses `GPIO_ACTIVE_LOW`. The [PCIe page](pcie-host-bridges-windows-and-enumeration.md#root-complex-resources) shows the same inversion.

Bluetooth bindings commonly add `max-speed`, which constrains the protocol driver's baud rate; it does not set a console baud.

## RS-232 And RS-485

The SoC UART exposes logic-level signals. An external RS-232 level shifter or RS-485 transceiver may introduce enable, termination, polarity, turnaround, and suspend requirements. Generic serial bindings include several RS-485 properties, but support depends on the controller driver and binding.

Do not model software turnaround delays as GPIO toggles in DT when the serial core/controller supports RS-485. Conversely, declaring RS-485 mode cannot compensate for an unmodeled transceiver supply or direction-control connection.

## Runtime Diagnosis

```sh
cat /proc/consoles
cat /proc/tty/driver/serial
dmesg | grep -Ei 'tty|serial|uart|console'
ls -l /sys/class/tty
```

For electrical faults, observe TX/RX at both the SoC and transceiver sides. Check idle voltage, baud period, stop/parity framing, and contention. A tty device appearing proves driver registration, not correct pinmux or signal integrity.

## Failure Patterns

- `stdout-path` references an alias absent from the final tree.
- Kernel command line selects a different console than `/chosen`.
- The board enables a UART but omits its pinctrl state.
- TX and RX are swapped, at the wrong voltage, or driven by two owners.
- DMA fails while interrupt-driven console traffic appears healthy.
- Runtime PM gates the UART while an attached wake-capable device expects it active.
- A serial child is placed at the root, losing its transport relationship.

## Authoritative References

- [Devicetree Specification: `/chosen` and aliases](https://devicetree-specification.readthedocs.io/en/stable/devicenodes.html)
- [Linux serial binding schema](https://github.com/torvalds/linux/blob/master/Documentation/devicetree/bindings/serial/serial.yaml)
- [Linux console driver documentation](https://docs.kernel.org/driver-api/tty/console.html)

## Continue

Proceed to [I2C Controllers, Devices, And Muxes](i2c-controllers-devices-and-muxes.md).

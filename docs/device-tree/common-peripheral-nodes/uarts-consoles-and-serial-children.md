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

SoC controller nodes normally come from the SoC `.dtsi`; a board enables and wires them:

```dts
&uart2 {
        pinctrl-names = "default", "sleep";
        pinctrl-0 = <&uart2_default>;
        pinctrl-1 = <&uart2_sleep>;
        status = "okay";
};
```

The inherited node may already define `compatible`, `reg`, interrupts, clocks, resets, DMA channels, and quirks. Do not copy these into the board file unless board wiring changes a binding-defined property. Review the final merged node, not the amendment alone.

## Console Selection

`/chosen/stdout-path` selects the firmware console path, optionally with serial options:

```dts
aliases {
        serial0 = &uart2;
};

chosen {
        stdout-path = "serial0:115200n8";
};
```

The alias offers a stable firmware path; it does not universally guarantee `/dev/ttyS0`. Linux driver type, probe order, command-line console parameters, and alias handling determine tty naming. Confirm the boot log's actual console registration.

Bootloader and kernel must agree on pins, clock, format, and ownership during handoff. Garbled early output often indicates a source clock or divisor mismatch, not a bad baud string. Silence after early console can mean the full driver changed pinctrl, reset, or clock state.

## Hardware Flow Control

Properties such as `uart-has-rtscts` are binding-specific declarations that RTS/CTS are wired. They do not create missing pins. Ensure the pinctrl state includes the signals and the remote device uses compatible voltage levels and polarity.

If RTS or CTS is repurposed as GPIO, do not claim hardware flow control. A user-space choice to enable flow control is policy; whether the lines physically exist is hardware description.

## Devices Attached To A UART

Bluetooth, GNSS, and other fixed serial peripherals may be child nodes when their binding defines the relationship:

```dts
&uart3 {
        status = "okay";

        bluetooth {
                compatible = "vendor,radio-chip";
                shutdown-gpios = <&gpio2 3 GPIO_ACTIVE_HIGH>;
                vbat-supply = <&reg_radio>;
                max-speed = <3000000>;
        };
};
```

Use the exact child binding. `max-speed` describes a capability/constraint used by the protocol driver; it does not set a console baud. A child may require clocks, reset GPIOs, wake interrupts, and a power sequence beyond the UART controller itself.

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

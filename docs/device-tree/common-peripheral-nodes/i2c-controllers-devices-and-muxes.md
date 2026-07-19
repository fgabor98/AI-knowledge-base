---
status: draft
reviewed: false
domain: device-tree
difficulty: intermediate
last_reviewed: null
---

# I2C Controllers, Devices, And Muxes

I²C does not enumerate ordinary targets. DT must identify every fixed device, its address, the physical bus segment it occupies, and board-specific resources that its binding requires.

## Controller And Child Addressing

```dts
&i2c1 {
        clock-frequency = <400000>;
        pinctrl-names = "default";
        pinctrl-0 = <&i2c1_default>;
        status = "okay";

        temperature-sensor@48 {
                compatible = "vendor,temp-sensor";
                reg = <0x48>;
                interrupt-parent = <&gpio3>;
                interrupts = <5 IRQ_TYPE_LEVEL_LOW>;
                vdd-supply = <&reg_3v3>;
        };
};
```

For a normal 7-bit target, `reg` is the unshifted address shown in the binding/datasheet. Do not encode the address byte with the R/W bit. Ten-bit addresses require the generic binding's flag encoding and controller support.

`clock-frequency` is a bus limit/request, not proof every child tolerates it. The slowest target, rise time, pull-ups, capacitance, clock stretching, controller limitations, and level shifters determine a safe rate.

## No Blind Scanning As Description

Tools such as `i2cdetect` can disturb devices because not every address responds safely to generic probe transactions. Use the schematic and binding to declare known devices. Scanning is a controlled diagnostic technique, not enumeration or a substitute for DT.

An ACK proves that something responded at an address. It does not prove compatible identity, register protocol, voltage state, or correct bus segment.

## Muxes And Switches Create New Segments

An I²C mux has a parent-side address and child bus nodes:

```dts
i2c-mux@70 {
        compatible = "nxp,pca9544";
        reg = <0x70>;
        #address-cells = <1>;
        #size-cells = <0>;

        i2c@0 {
                reg = <0>;
                #address-cells = <1>;
                #size-cells = <0>;

                eeprom@50 {
                        compatible = "atmel,24c64";
                        reg = <0x50>;
                };
        };
};
```

Here `i2c-mux@70/reg` is an I²C address, `i2c@0/reg` is a mux channel, and `eeprom@50/reg` is an address on that downstream segment. Identical target addresses are legal on isolated channels. Placing both devices under the parent bus creates a false collision.

Mux bindings vary in whether an `i2c-mux` container is present. Follow the device schema exactly. Consider idle channel, disconnect, power, and concurrent access behavior.

## Interrupts, GPIOs, And Alert Lines

An I²C device's IRQ is usually routed separately through a GPIO or interrupt controller. The I²C address does not imply interrupt routing. Shared open-drain alert lines require a compatible trigger, pulls, and drivers that drain all pending sources.

GPIO expanders on I²C can themselves provide GPIO and IRQ domains. That creates a supplier chain: controller → expander → downstream consumer. If the expander is powered off, every consumer GPIO/IRQ disappears with it.

## Bus Recovery And Electrical Reality

A target can hold SDA low after reset or interrupted transfer. Some controller bindings support recovery GPIOs or pinctrl states; some drivers use hardware recovery. Validate that changing pins to GPIO does not conflict with another owner and that the pulses reach the affected segment through any mux.

Rise time is set by pull-up resistance and bus capacitance. Symptoms of marginal timing include address-dependent errors, failures only at temperature, or success at 100 kHz but not 400 kHz. Logic-analyzer decoding is useful only after verifying actual voltage thresholds and waveform shape.

## Runtime Diagnosis

```sh
i2cdetect -l
find /sys/bus/i2c/devices -maxdepth 2 -type l -o -type d
dmesg | grep -Ei 'i2c|smbus|eeprom'
```

Bus numbers are runtime identifiers and may change with aliases, mux creation, or probe order. Map adapters through sysfs names and OF paths rather than hard-coding `i2c-1` in production logic.

Use `i2cget`/`i2cset` only when no kernel driver owns the device and the datasheet makes the transaction safe. Competing with a bound driver can corrupt state.

## Authoritative References

- [Linux I2C device instantiation](https://docs.kernel.org/i2c/instantiating-devices.html)
- [Linux I2C sysfs topology](https://docs.kernel.org/i2c/i2c-sysfs.html)
- [Linux generic I2C controller schema](https://github.com/torvalds/linux/blob/master/Documentation/devicetree/bindings/i2c/i2c-controller.yaml)
- [Linux I2C mux documentation](https://docs.kernel.org/i2c/i2c-topology.html)

## Continue

Proceed to [SPI Controllers, Chip Selects, And Peripherals](spi-controllers-chip-selects-and-peripherals.md).

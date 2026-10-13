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

From `k3-am62-lp-sk.dts` (trimmed); `k3-am62x-sk-common.dtsi` enables `main_i2c1` at 100 kHz:

```dts
&main_i2c1 {
        exp1: gpio@22 {
                compatible = "ti,tca6424";
                reg = <0x22>;
                gpio-controller;
                #gpio-cells = <2>;
                gpio-line-names = "GPIO_CPSW2_RST", "GPIO_CPSW1_RST",
                                  "PRU_DETECT", "MMC1_SD_EN", /* ... */;

                interrupt-parent = <&main_gpio1>;
                interrupts = <23 IRQ_TYPE_EDGE_FALLING>;
                interrupt-controller;
                #interrupt-cells = <2>;

                pinctrl-names = "default";
                pinctrl-0 = <&main_gpio1_ioexp_intr_pins_default>;
        };
};
```

The TCA6424 is an I²C target, an interrupt consumer (its `INT` output on `main_gpio1` line 23), and a GPIO and interrupt provider for other nodes, such as the SD card supply on line 3.

For a normal 7-bit target, `reg` is the unshifted address without the R/W bit; ten-bit addresses need the generic binding's flag encoding ([I2C Children](../addressing-and-bus-modeling/bus-specific-child-addressing.md#i2c-children)).

`clock-frequency` is a bus limit/request, not proof every child tolerates it. The slowest target, rise time, pull-ups, capacitance, clock stretching, controller limitations, and level shifters determine a safe rate.

## No Blind Scanning As Description

Tools such as `i2cdetect` can disturb devices because not every address responds safely to generic probe transactions. Use the schematic and binding to declare known devices. Scanning is a controlled diagnostic technique, not enumeration or a substitute for DT.

An ACK proves that something responded at an address. It does not prove compatible identity, register protocol, voltage state, or correct bus segment.

## Muxes And Switches Create New Segments

An I²C mux has a parent-side address and child bus nodes. The J721E SK places a PCA9543 on `main_i2c3` to separate its two camera connectors. From `k3-j721e-sk.dts`:

```dts
i2c-mux@70 {
        compatible = "nxp,pca9543";
        #address-cells = <1>;
        #size-cells = <0>;
        reg = <0x70>;

        /* CSI0 I2C */
        cam0_i2c: i2c@0 {
                #address-cells = <1>;
                #size-cells = <0>;
                reg = <0>;
        };

        /* CSI1 I2C */
        cam1_i2c: i2c@1 {
                #address-cells = <1>;
                #size-cells = <0>;
                reg = <1>;
        };
};
```

The overlay `k3-j721e-sk-csi2-dual-imx219.dtso` adds one Sony IMX219 to each channel, both at address `0x10`:

```dts
&cam0_i2c {
        imx219_0: imx219-0@10 {
                compatible = "sony,imx219";
                reg = <0x10>;
                /* clocks, supplies, port */
        };
};

&cam1_i2c {
        imx219_1: imx219-1@10 {
                compatible = "sony,imx219";
                reg = <0x10>;
                /* ... */
        };
};
```

Here `i2c-mux@70/reg` is an I²C address, `i2c@0/reg` is a mux channel, and each `imx219-*@10/reg` is an address on its own downstream segment. Identical addresses are legal on isolated channels; placing both sensors directly under `main_i2c3` would create a real collision. The base DTS leaves the channels empty, so the mux and its adapters exist even without cameras.

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

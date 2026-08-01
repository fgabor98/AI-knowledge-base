---
status: draft
reviewed: false
domain: device-tree
difficulty: intermediate
last_reviewed: null
---

# SPI Controllers, Chip Selects, And Peripherals

SPI shares clock and data wires while selecting targets individually. It provides no standard device enumeration, so each populated target and its board-level transfer limits must be described explicitly.

## Target Nodes

```dts
&spi2 {
        pinctrl-names = "default";
        pinctrl-0 = <&spi2_default>;
        status = "okay";

        adc@0 {
                compatible = "vendor,precision-adc";
                reg = <0>;
                spi-max-frequency = <12000000>;
                spi-cpol;
                spi-cpha;
                vref-supply = <&reg_2v5>;
                interrupts-extended = <&gpio2 11 IRQ_TYPE_EDGE_FALLING>;
        };
};
```

For an ordinary SPI target, `reg` is the controller-local chip-select index. It is not a memory address. `spi-max-frequency` is the maximum safe bus clock for this target on this board; the driver may use a lower rate.

`spi-cpol` and `spi-cpha` select the clock mode. Absence means both false for mode 0. These are protocol facts, not tuning knobs. A wrong mode can return data that looks structured but is bit-shifted or sampled on unstable edges.

## Native And GPIO Chip Selects

Controllers may provide native chip selects or use `cs-gpios`. The mapping between logical chip-select indices and native/GPIO lines is controller-binding specific. Review inactive polarity, boot state, and whether a GPIO glitches while pinctrl or the controller probes.

No two enabled targets may respond to the same physical select unless a binding explicitly models a coordinated multi-device arrangement. Also verify that unselected targets truly tri-state shared MISO; a powered-down target can clamp the bus through its I/O pins.

## Transfer Width And Lanes

Standard properties can describe dual, quad, or octal data connectivity:

```dts
spi-rx-bus-width = <4>;
spi-tx-bus-width = <1>;
```

They describe wired lanes and supported protocol use, not a request to force every transfer into that width. Controller, target, and protocol driver must all support the selected operation. Validate pinctrl for every data lane.

Bits per word, maximum transfer size, DMA alignment, and chip-select timing are usually driver/controller capabilities rather than generic DT properties. Add vendor properties only when a reviewed binding defines real hardware constraints.

## Timing Beyond Frequency

Many devices specify minimum chip-select setup/hold, inter-word delay, or wake time. Modern generic SPI bindings expose selected timing properties, but controller support varies. Do not reduce all timing requirements to a lower `spi-max-frequency`; frequency does not guarantee chip-select setup or inter-transfer delay.

A level shifter, isolator, long trace, or connector can lower the board limit below the silicon limit. Validate signal integrity at the target pins under worst-case load and voltage.

## `spidev` Is Not A Hardware Identity

Production DT should use a real compatible that identifies the peripheral protocol. A generic `spidev` compatible is not a substitute for a kernel binding and is rejected or discouraged in mainline workflows. If user space owns a custom protocol, define an appropriate binding and access/security model.

## Runtime Diagnosis

```sh
ls -l /sys/bus/spi/devices
dmesg | grep -Ei 'spi|chip.?select'
```

Runtime device names such as `spi2.0` combine a Linux bus number and chip-select index. Bus numbers can vary; use sysfs topology and OF links to correlate them.

With a scope or logic analyzer, check selection polarity, mode, frequency, word boundaries, MISO contention, and error behavior. Decode captures using the actual protocol phase—an analyzer configured to the same wrong mode as the DTS can mislead.

## Failure Patterns

- `reg` is copied as a register address rather than chip select.
- Mode or bit order is wrong but reads appear non-random.
- `spi-max-frequency` copies the chip limit despite board-level degradation.
- Native/GPIO chip-select mapping does not match pin routing.
- A sleeping or unpowered target drives shared MISO.
- Quad mode is declared without four routed data pins.
- A user-space SPI transaction races a bound kernel driver.

## Authoritative References

- [Linux SPI overview](https://docs.kernel.org/spi/spi-summary.html)
- [Linux SPI driver API](https://docs.kernel.org/driver-api/spi.html)
- [Linux generic SPI controller schema](https://github.com/torvalds/linux/blob/master/Documentation/devicetree/bindings/spi/spi-controller.yaml)
- [Linux SPI multiple-data-lane guidance](https://docs.kernel.org/spi/multiple-data-lanes.html)

## Continue

Proceed to [CAN Controllers, Transceivers, And Bit Timing](can-controllers-transceivers-and-bit-timing.md).

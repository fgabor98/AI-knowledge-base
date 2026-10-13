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

The AM642 EVM puts a Microwire EEPROM on McSPI `main_spi0`. From `k3-am642-evm.dts`:

```dts
&main_spi0 {
        status = "okay";
        pinctrl-names = "default";
        pinctrl-0 = <&main_spi0_pins_default>;
        ti,pindir-d0-out-d1-in;
        eeprom@0 {
                compatible = "microchip,93lc46b";
                reg = <0>;
                spi-max-frequency = <1000000>;
                spi-cs-high;
                data-size = <16>;
        };
};
```

For an ordinary SPI target, `reg` is the controller-local chip-select index, not a memory address. `spi-max-frequency` is the maximum safe bus clock for this target on this board; the driver may use a lower rate.

Every other property here records a hardware fact:

- `ti,pindir-d0-out-d1-in` is a controller property. McSPI data pins have no fixed MOSI/MISO role; the default is D0 input and D1 output, and this board wires them the other way.
- `spi-cs-high` says the chip select is active high, as the 93xx46 Microwire protocol requires. Without it, the EEPROM is selected exactly when it should be idle.
- `data-size = <16>` is required by `microchip,93lc46b.yaml`. The 93LC46B is the 16-bit-word variant (the 93LC46A uses 8-bit words), so the value must match the part actually fitted.
- no `spi-cpol` or `spi-cpha` means SPI mode 0

`spi-cpol` and `spi-cpha` are protocol facts, not tuning knobs. A wrong mode can return data that looks structured but is bit-shifted or sampled on unstable edges.

## Native And GPIO Chip Selects

Controllers may provide native chip selects or use `cs-gpios`. The mapping between logical chip-select indices and native/GPIO lines is controller-binding specific. Review inactive polarity, boot state, and whether a GPIO glitches while pinctrl or the controller probes.

No two enabled targets may respond to the same physical select unless a binding explicitly models a coordinated multi-device arrangement. Also verify that unselected targets truly tri-state shared MISO; a powered-down target can clamp the bus through its I/O pins.

## Transfer Width And Lanes

Standard properties can describe dual, quad, or octal data connectivity. The AM642 EVM's OSPI flash sets `spi-tx-bus-width = <8>` and `spi-rx-bus-width = <8>` ([MTD, SPI NOR, And Fixed Partitions](mtd-spi-nor-and-fixed-partitions.md#spi-nor-device)). These properties describe wired lanes and supported protocol use, not a request to force every transfer into that width. Controller, target, and protocol driver must all support the selected operation. Validate pinctrl for every data lane.

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

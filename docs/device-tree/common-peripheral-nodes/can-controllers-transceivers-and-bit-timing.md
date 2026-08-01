---
status: draft
reviewed: false
domain: device-tree
difficulty: intermediate
last_reviewed: null
---

# CAN Controllers, Transceivers, And Bit Timing

A CAN controller implements framing and arbitration; a transceiver converts controller logic signals to the differential CAN bus. Successful controller probe proves neither that the transceiver is awake nor that bit timing and termination are correct.

## Controller Description

An integrated controller commonly needs registers, clocks, resets, pins, interrupts, and a transceiver relationship:

```dts
can_transceiver: can-phy {
        compatible = "ti,tcan1043";
        #phy-cells = <0>;
        standby-gpios = <&gpio3 8 GPIO_ACTIVE_HIGH>;
        max-bitrate = <5000000>;
};

&can0 {
        pinctrl-names = "default", "sleep";
        pinctrl-0 = <&can0_default>;
        pinctrl-1 = <&can0_sleep>;
        phys = <&can_transceiver>;
        status = "okay";
};
```

The exact transceiver binding and property names vary. Some use `xceiver-supply`, enable or standby GPIOs, and `max-bitrate`; some controllers use a generic PHY handle. Follow both schemas rather than inventing a `transceiver-gpios` convention.

External SPI CAN controllers are SPI children and also CAN network devices. Their node must satisfy both bindings: chip select/mode/rate plus oscillator, interrupt, reset, and transceiver requirements.

## Clock And Bit Timing

The controller clock feeds bit-timing calculation. An incorrect oscillator or parent rate can produce a requested nominal bitrate in software while the wire timing is wrong. CAN FD adds a data-phase bitrate that can exceed the arbitration rate.

Bit timing is normally configured at runtime through the network interface:

```sh
ip link set can0 type can bitrate 500000 sample-point 0.875
ip link set can0 up
```

DT should describe fixed hardware inputs and limits, not deployment-specific network bitrate, unless a particular binding explicitly defines a hardware constraint. All nodes on the physical bus must use compatible arbitration timing.

## Physical Network Contract

Validate:

- exactly two effective terminations at the ends of the main bus
- characteristic impedance and stub length
- transceiver voltage and I/O voltage
- common-mode range and ground/reference strategy
- standby/silent pin polarity and default state
- CAN FD capability at the chosen data rate
- wake behavior while controller or SoC domain sleeps

A nominal 60 Ω measurement across an unpowered correctly terminated bus is a useful check, not a complete signal-integrity test.

## Runtime Evidence

```sh
ip -details -statistics link show can0
ethtool -i can0
ip link set can0 type can bitrate 500000 loopback on
```

Internal loopback verifies much of the controller/driver path but bypasses the transceiver and cable. External loopback or traffic with a known-good node exercises the physical path. Inspect error counters, bus state, restart behavior, and captured differential waveforms.

An interface entering error-passive or bus-off is evidence, not a reason to raise restart frequency blindly. Look for bitrate mismatch, absent ACK peers, wiring, termination, or transceiver standby.

## Failure Patterns

- The CAN controller probes while the transceiver remains in standby.
- The controller clock rate differs from the binding or driver assumption.
- An external SPI controller has the wrong interrupt trigger.
- Internal loopback passes but the physical bus is open or unterminated.
- One node uses CAN FD data timing on a classic-only transceiver.
- Wake is requested even though the transceiver or IRQ path loses power.
- Termination is modeled as software policy when it is fixed board hardware—or omitted when a controllable terminator binding exists.

## Authoritative References

- [Linux SocketCAN documentation](https://docs.kernel.org/networking/can.html)
- [Linux CAN controller common schema](https://github.com/torvalds/linux/blob/master/Documentation/devicetree/bindings/net/can/can-controller.yaml)
- [Linux TCAN104x CAN transceiver PHY schema](https://github.com/torvalds/linux/blob/master/Documentation/devicetree/bindings/phy/ti,tcan104x-can.yaml)

## Continue

Proceed to [USB Controllers, PHYs, Roles, And Connectors](usb-controllers-phys-roles-and-connectors.md).

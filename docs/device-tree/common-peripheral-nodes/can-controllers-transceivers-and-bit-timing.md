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

The AM64x SoC file describes each M_CAN instance. From `k3-am64-main.dtsi`:

```dts
main_mcan0: can@20701000 {
        compatible = "bosch,m_can";
        reg = <0x00 0x20701000 0x00 0x200>,
              <0x00 0x20708000 0x00 0x8000>;
        reg-names = "m_can", "message_ram";
        power-domains = <&k3_pds 98 TI_SCI_PD_EXCLUSIVE>;
        clocks = <&k3_clks 98 5>, <&k3_clks 98 0>;
        clock-names = "hclk", "cclk";
        interrupts = <GIC_SPI 155 IRQ_TYPE_LEVEL_HIGH>,
                     <GIC_SPI 156 IRQ_TYPE_LEVEL_HIGH>;
        interrupt-names = "int0", "int1";
        bosch,mram-cfg = <0x0 128 64 64 64 64 32 32>;
        status = "disabled";
};
```

`hclk` is the register interface clock; `cclk` feeds the CAN core and bit timing. `bosch,mram-cfg` partitions the private message RAM: offset 0, then element counts for standard and extended ID filters, Rx FIFO 0, Rx FIFO 1, Rx buffers, Tx events, and Tx buffers.

The AM642 EVM adds the transceiver and pins. From `k3-am642-evm.dts`:

```dts
transceiver1: can-phy0 {
        compatible = "ti,tcan1042";
        #phy-cells = <0>;
        max-bitrate = <5000000>;
        standby-gpios = <&exp1 8 GPIO_ACTIVE_HIGH>;
};

&main_mcan0 {
        status = "okay";
        pinctrl-names = "default";
        pinctrl-0 = <&main_mcan0_pins_default>;
        phys = <&transceiver1>;
};
```

The transceiver is a generic PHY (`phy-can-transceiver`). `m_can_open()` calls `phy_power_on()`, which drives `standby` low, so the TCAN1042 leaves standby only while the interface is up. `max-bitrate` becomes the PHY's `max_link_rate`, and `m_can_platform` copies it to `bitrate_max`, so the CAN netlink code rejects any nominal or data bitrate above 5 Mbit/s. The standby line comes from the I²C GPIO expander `exp1`, so the transceiver depends on `main_i2c1` as well.

Other transceiver bindings use `enable-gpios` or `xceiver-supply`. Follow the schemas rather than inventing a `transceiver-gpios` convention.

External SPI CAN controllers are SPI children and also CAN network devices. Their node must satisfy both bindings: chip select, mode, and rate plus oscillator, interrupt, reset, and transceiver requirements.

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

- The CAN controller probes while the transceiver remains in standby, for example because its expander-driven standby line has the wrong polarity.
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

---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# CAN Driver Integration Overview

## What Problem Does This Solve?

CAN driver integration connects controller drivers and transceivers to the Linux networking stack through SocketCAN.

CAN is different from many peripheral drivers because userspace sees a network interface:

```text
can0
```

not a character device or custom sysfs file.

## Core Concepts

- SocketCAN
- CAN controller
- CAN transceiver
- CAN network device
- bit timing
- bitrate
- sample point
- CAN FD
- error frames
- bus-off
- bus-off recovery
- termination
- transceiver standby
- `ip link`
- `can-utils`

## Mental Model

CAN appears to userspace as a network interface, but bring-up depends on controller clocks, pinmux, transceiver control, power rails, and physical bus conditions.

```text
CAN controller driver
-> netdev can0
-> SocketCAN
-> userspace sockets/can-utils
```

Board integration often includes a separate transceiver with enable, standby, or power control.

## Device Tree Shape

Controller example:

```dts
can0: can@10020000 {
    compatible = "example,can-controller";
    reg = <0x0 0x10020000 0x0 0x1000>;
    interrupts = <GIC_SPI 55 IRQ_TYPE_LEVEL_HIGH>;
    clocks = <&clkctrl 20>;
    pinctrl-names = "default", "sleep";
    pinctrl-0 = <&can0_default_pins>;
    pinctrl-1 = <&can0_sleep_pins>;
    transceiver = <&can0_transceiver>;
    status = "okay";
};

can0_transceiver: can-transceiver {
    compatible = "nxp,tja1040";
    standby-gpios = <&gpio2 3 GPIO_ACTIVE_HIGH>;
    max-bitrate = <1000000>;
};
```

Exact properties depend on controller and transceiver bindings.

## Userspace Bring-Up

Show interfaces:

```sh
ip link show
```

Configure bitrate:

```sh
sudo ip link set can0 type can bitrate 500000
sudo ip link set can0 up
```

CAN FD example:

```sh
sudo ip link set can0 type can bitrate 500000 dbitrate 2000000 fd on
sudo ip link set can0 up
```

Send/receive with `can-utils`:

```sh
candump can0
cansend can0 123#11223344
```

## Controller Driver Role

A CAN controller driver usually:

- registers a CAN network device
- configures bit timing
- starts/stops the controller
- handles TX/RX interrupts
- reports error frames
- handles bus-off state
- integrates with NAPI where appropriate
- coordinates clocks, resets, pinctrl, regulators, and transceiver

Most product work configures or debugs existing CAN drivers rather than writing one from scratch.

## Transceiver Role

The CAN controller speaks digital CAN logic. The transceiver drives the physical bus.

The transceiver may need:

- regulator supply
- standby GPIO
- enable GPIO
- max bitrate limit
- wake capability
- sleep mode

If the controller probes but the bus is silent, check transceiver state.

## Bit Timing

CAN bitrate depends on:

- controller clock
- bit timing segments
- sample point
- prescaler
- controller limits
- transceiver limits

Userspace can ask the kernel to calculate timing:

```sh
ip -details link show can0
```

If communication fails at one bitrate but not another, check clock accuracy and bit timing.

## Error Frames And Bus-Off

CAN controllers report protocol and physical-layer problems through error frames and interface state.

Inspect:

```sh
ip -details -statistics link show can0
```

Common states:

- error-active
- error-warning
- error-passive
- bus-off

Bus-off recovery may be manual or automatic depending on configuration:

```sh
sudo ip link set can0 type can restart-ms 100
```

Manual restart:

```sh
sudo ip link set can0 type can restart
```

## Physical Bus Checks

CAN needs correct physical conditions:

- CAN_H/CAN_L wiring
- common ground where required
- termination, usually 120 ohms at both ends
- correct transceiver supply
- no standby mode
- bitrate matches all nodes
- bus length and speed are compatible

A driver can be correct while the bus is physically broken.

## Debugging CAN

Kernel/network view:

```sh
ip -details -statistics link show can0
dmesg | grep -i can
cat /proc/interrupts | grep -i can
```

CAN traffic:

```sh
candump can0
cansend can0 123#DEADBEEF
```

Device Tree:

```sh
dtc -I fs -O dts /proc/device-tree > /tmp/running.dts
rg 'can|transceiver|bitrate' /tmp/running.dts
```

Hardware:

- scope CAN_TX/RX at controller side if accessible
- scope CAN_H/CAN_L differential bus
- measure termination resistance with power off
- check transceiver standby pin

## Common Failure Modes

| Symptom | Likely Cause | First Checks |
| --- | --- | --- |
| no `can0` | controller driver/DT/config issue | dmesg, DT, `.config` |
| interface up but no frames | transceiver standby, wiring, bitrate | scope, transceiver |
| bus-off | physical bus, bitrate mismatch, no ACK | `ip -details`, wiring |
| RX works, TX fails | transceiver enable, bus ACK, termination | scope, error counters |
| wrong bitrate | clock/pinctrl/bit timing issue | clock rate, `ip -details` |
| suspend wake fails | transceiver/controller wake config | wakeup policy |

## Common Mistakes

- Treating CAN as a character device.
- Debugging SocketCAN before checking the transceiver.
- Forgetting termination.
- Using the wrong bitrate or sample point.
- Ignoring controller clock accuracy.
- Leaving transceiver in standby.
- Not checking error counters.
- Assuming successful `ip link set up` proves the physical bus works.

## Practice Exercises

### Exercise 1: Bring Up CAN

Configure a CAN interface at a known bitrate and inspect details:

```sh
sudo ip link set can0 type can bitrate 500000
sudo ip link set can0 up
ip -details link show can0
```

### Exercise 2: Send And Receive

Use two CAN nodes or a loopback setup:

```sh
candump can0
cansend can0 123#11223344
```

### Exercise 3: Diagnose Bus-Off

Intentionally mismatch bitrate in a lab and observe error counters and bus-off behavior. Restore correct bitrate.

## Debugging Checklist

- Is the CAN controller node enabled?
- Did the netdev appear?
- Are clocks and pinctrl correct?
- Is the transceiver powered and out of standby?
- Is the bitrate correct on all nodes?
- Is termination correct?
- Do error counters increase?
- Does `/proc/interrupts` show controller activity?
- Is bus-off recovery configured intentionally?

## Related Topics

- [Clocks](clocks.md)
- [Regulators](regulators.md)
- [Pinctrl](pinctrl.md)
- [IRQ Handling](irq-handling.md)
- [Networking](../../networking/index.md)

## Official References

- [SocketCAN](https://docs.kernel.org/networking/can.html)
- [Network device driver API](https://docs.kernel.org/networking/netdevices.html)

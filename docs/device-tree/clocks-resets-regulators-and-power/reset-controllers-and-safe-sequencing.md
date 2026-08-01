---
status: draft
reviewed: false
domain: device-tree
difficulty: intermediate
last_reviewed: null
---

# Reset Controllers And Safe Sequencing

A reset control may expose a level, generate a pulse, or trigger a hardware-defined sequence. The DT tuple identifies that control; it does not tell a consumer that every reset operation is safe.

## Provider And Consumer Model

```dts
reset_controller: reset-controller@11000 {
        compatible = "example,soc-reset-controller";
        reg = <0x11000 0x1000>;
        #reset-cells = <1>;
};

ethernet@40000 {
        resets = <&reset_controller SOC_RST_ETH_MAC>,
                 <&reset_controller SOC_RST_ETH_BUS>;
        reset-names = "mac", "bus";
};
```

The provider binding defines each specifier. One cell may be a logical reset ID rather than a hardware bit. A multi-cell format might encode bank and offset or flags. Names and order come from the consumer binding.

## Reset Line, Control, And Controller

Keep three concepts separate:

- a **reset line** is the physical signal reaching hardware
- a **reset control** is the operation exposed to software; it may affect one or several lines
- a **reset controller** is the provider implementing those controls

One control may launch a timed sequence over several physical lines. Several consumer nodes may refer to the same control. Those facts determine whether assertion, deassertion, or pulse operations are legal.

## Exclusive, Shared, And Pulse Semantics

An exclusive reset handle lets one consumer control assertion state. It is appropriate when independently asserting the line cannot disrupt another active device.

A shared reset control coordinates users of the same hardware reset. Shared level controls rely on balanced deassert/assert use so the physical reset changes only when all users agree. Shared pulse controls have special one-shot and rearm behavior. Consumer drivers must use the API variant that matches the hardware and binding.

Do not infer “shared” because two DTS nodes contain the same tuple. That duplication may instead reveal an invalid hardware model or two functions that should be represented under one parent device.

## Reset GPIO Versus Reset Controller

An external IC often has a dedicated reset pin described with `reset-gpios`. An integrated SoC block usually uses `resets` through the reset framework. They model different provider classes and driver APIs.

Do not publish both for the same physical line unless the consumer binding explicitly defines both roles. If a board-level sequencer or reset controller owns a GPIO internally, consumers should normally reference that higher-level controller rather than contend for the GPIO.

## Sequencing Is Not Property Order

This ordering in source has no operational meaning:

```dts
clocks = <&clock_controller SOC_CLK_ENGINE>;
resets = <&reset_controller SOC_RST_ENGINE>;
vdd-supply = <&reg_engine>;
```

The driver, frameworks, or firmware sequence resources. A common—but not universal—enable path is:

1. enable the supply and wait for stability
2. power the domain and remove isolation
3. prepare and enable clocks
4. assert reset if state is unknown
5. wait the hardware-defined interval
6. deassert reset
7. access registers and confirm readiness

Some hardware requires reset asserted before its clock starts; other reset controllers cannot operate without that clock. Only the integration manual and binding can settle the order.

## Arrays Do Not Promise Ordering

Reset-control array helpers are designed for collections that can be operated without a required order. If hardware requires reset A before reset B with a delay, a consumer driver must request named controls and sequence them explicitly, or a provider must expose the sequence as one reset control.

Adding delays or reordering tuples in DTS does not encode such a requirement.

## Bootloader State Is Not Ownership

A peripheral working only with a warm boot often depends on a bootloader-deasserted reset. Linux must acquire and manage every reset required by the binding. Conversely, blindly pulsing resets during probe can destroy firmware state, interrupt a shared function, or break seamless display handoff.

For firmware-owned or boot-critical blocks, establish explicitly:

- whether Linux may reset the device
- which state firmware guarantees at handoff
- whether reset affects sibling devices or retained memory
- whether suspend, shutdown, or kexec may assert it

## Diagnosing Reset Failures

Start from the consumer:

1. verify the runtime `resets` and `reset-names` properties
2. resolve the phandle and decode the provider cells
3. confirm the provider node, ancestors, compatible, and driver
4. distinguish `-ENOENT`, malformed specifiers, and `-EPROBE_DEFER`
5. inspect reset-controller and consumer logs
6. check clock and power prerequisites for the reset controller itself
7. measure the physical line or reset-status register if accessible
8. compare cold boot, warm boot, unbind/rebind, and suspend/resume

A deassert call returning success proves only that the provider accepted the operation. It does not prove voltage, clock, isolation, pulse width, or device readiness.

## Review Traps

- Treating a reset ID as a universal register bit.
- Using a shared handle for hardware that needs independent assertion.
- Using an exclusive handle for a line shared with an active sibling.
- Calling both pulse and level APIs on the same shared control.
- Assuming an optional reset should hide every lookup error.
- Encoding required sequencing by tuple or property order.
- Resetting a firmware-initialized block without a handoff design.

## Authoritative References

- [Linux reset controller API](https://docs.kernel.org/driver-api/reset.html)
- [Linux device resource management](https://docs.kernel.org/driver-api/driver-model/devres.html)
- [Linux Devicetree binding index](https://docs.kernel.org/devicetree/bindings/index.html)

## Continue

Proceed to [Regulators, Supplies, And Board Constraints](regulators-supplies-and-board-constraints.md).

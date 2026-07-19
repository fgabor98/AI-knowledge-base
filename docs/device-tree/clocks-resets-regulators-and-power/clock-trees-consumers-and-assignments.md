---
status: draft
reviewed: false
domain: device-tree
difficulty: intermediate
last_reviewed: null
---

# Clock Trees, Consumers, And Assignments

Clocks form a directed topology: oscillators and PLLs feed muxes, dividers, and gates, which feed consumers. A consumer specifier selects a provider output. It does not describe the whole signal path or guarantee a particular rate.

## Providers And Specifiers

A provider advertises `#clock-cells`; its binding defines every argument after the phandle:

```dts
clock_controller: clock-controller@10000 {
        compatible = "example,soc-clock-controller";
        reg = <0x10000 0x1000>;
        #clock-cells = <1>;
};

video@30000 {
        clocks = <&clock_controller SOC_CLK_VIDEO_BUS>,
                 <&clock_controller SOC_CLK_VIDEO_CORE>;
        clock-names = "bus", "core";
};
```

The symbolic IDs normally come from a provider-specific header in `include/dt-bindings/clock/`. They compile to integers, but the integer is meaningful only in that provider's binding namespace. It need not equal a register offset or bit number.

With `#clock-cells = <0>`, the provider node itself identifies one output. With two or more cells, the tuple may encode a domain and an index, flags, or another provider-defined scheme. Never infer the format from a different SoC.

## Names Are Part Of The Consumer Contract

`clock-names` maps semantic roles to entries in `clocks`:

```c
bus_clk = devm_clk_get_enabled(dev, "bus");
core_clk = devm_clk_get(dev, "core");
```

Index order and names must match the consumer binding. Renaming a clock to match a schematic net may break the driver if the binding calls it `core`. Conversely, provider-internal clock names are not a replacement for `clock-names` on the consumer.

## Consumption Versus Assignment

These properties answer different questions:

| Property | Meaning |
|---|---|
| `clocks` | clock inputs consumed by this device |
| `clock-names` | semantic names for those inputs |
| `assigned-clocks` | clocks whose initial configuration is requested |
| `assigned-clock-parents` | requested parents for assigned clocks |
| `assigned-clock-rates` | requested rates for assigned clocks |

A device can consume a clock without assigning its parent or rate. It can also configure an assigned clock that is not one of its direct inputs, when its binding and clock topology justify doing so.

```dts
assigned-clocks = <&clock_controller SOC_CLK_VIDEO_CORE>;
assigned-clock-parents = <&clock_controller SOC_CLK_PLL_VIDEO>;
assigned-clock-rates = <600000000>;
```

Parallel assignment arrays are positional. A zero entry can mean “leave unchanged” where the generic clock binding permits it. Read the binding before using omission or zero as a placeholder.

Assignments are initial configuration, not an ownership lock. Another legitimate consumer may later change a shared ancestor, and a scaling driver may intentionally change the rate. If a device requires an invariant input frequency, that requirement belongs in its binding and driver contract.

## Rate Requests Propagate Through A Tree

The rate visible at a leaf can be constrained by:

- discrete PLL rates
- legal divider values
- a mux's available parents
- shared siblings using the same ancestor
- provider flags governing parent-rate propagation
- firmware ownership or secure-world policy

A request for 600 MHz may round to a supported value, fail, or cause an ancestor change that affects another device. A DTS reviewer therefore asks not only “is the requested rate legal?” but “which shared nodes can change to provide it?”

## Enable State Is Reference Counted

Linux clock consumers prepare and enable clocks through the framework. Multiple consumers can share a clock; disabling one consumer's handle does not necessarily stop the hardware clock. A critical or firmware-managed clock may remain enabled regardless of ordinary consumer counts.

Do not add `clk_ignore_unused` as a product fix. That boot argument is useful to prove that a driver failed to claim a clock, but it deliberately masks missing ownership descriptions.

## Assigned Clocks On Provider Nodes

Assignments are often placed on a board-level consumer or clock-controller node. Be cautious when a clock provider assigns clocks that it also provides: the framework cannot configure an output before the relevant provider exists, and cycles or early initialization rules may apply. Follow the provider binding and working upstream examples for that controller.

## Runtime Inspection

When debugfs and common-clock debug support are enabled, inspect:

```sh
cat /sys/kernel/debug/clk/clk_summary
```

Correlate names, enable/prepare counts, current rates, accuracy, and parentage with the provider driver and DTS. The debugfs names are Linux framework names, not guaranteed binding IDs.

Useful evidence also includes:

```sh
dmesg | grep -Ei 'clk|clock|pll'
find /sys/firmware/devicetree/base -name clocks -o -name assigned-clocks
```

Treat `clk_summary` as a snapshot. A zero enable count may be valid while the consumer is runtime-suspended.

## Review And Failure Patterns

- A missing clock typically causes probe failure, probe deferral, or inaccessible registers.
- A wrong clock ID may still resolve but feed the wrong gate.
- A correct leaf with a wrong parent can run at plausible but unstable rates.
- A rate specified in the wrong units can be schema-valid when the value range is broad.
- Copying all bootloader-enabled clocks into `assigned-clocks` hides which settings Linux truly requires.
- Assigning a shared PLL for one device can destabilize other consumers.

## Senior Review Questions

1. Which provider binding defines each clock specifier?
2. Which clock is the driver's `bus`, `core`, or reference input?
3. Does the requested leaf rate imply a shared-parent change?
4. Who is permitted to change the rate after probe?
5. What happens when the device runtime-suspends?
6. Does the implementation depend on bootloader enable counts or parent selection?

## Authoritative References

- [Linux Common Clock Framework](https://docs.kernel.org/driver-api/clk.html)
- [Linux camera sensor clock guidance](https://docs.kernel.org/driver-api/media/camera-sensor.html)
- [Linux Devicetree binding index](https://docs.kernel.org/devicetree/bindings/index.html)

## Continue

Proceed to [Reset Controllers And Safe Sequencing](reset-controllers-and-safe-sequencing.md).

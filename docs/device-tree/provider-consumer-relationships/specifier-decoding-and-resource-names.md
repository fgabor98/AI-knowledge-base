---
status: draft
reviewed: false
domain: device-tree
difficulty: intermediate
last_reviewed: null
---

# Specifier Decoding And Resource Names

## A Phandle Is Only The Start Of An Entry

Consider:

```dts
clocks = <&osc>, <&clock_controller 4 1>;
```

The source punctuation does not define two universal clock tuples. Each entry begins with a phandle, then carries the number of argument cells declared by the referenced provider:

```dts
osc: clock-24000000 {
        #clock-cells = <0>;
};

clock_controller: clock-controller@8000 {
        #clock-cells = <2>;
};
```

The effective entries are:

```text
&osc                         provider + 0 arguments
&clock_controller 4 1       provider + 2 arguments
```

The provider binding, not the consumer and not `dtc`, defines whether `4` is an output ID, bank, port, or something else and whether `1` is a flag, selector, or sub-index.

## What `#*-cells` Means

A provider advertises the number of argument cells following its phandle for one resource class:

```dts
#clock-cells = <1>;
#reset-cells = <1>;
#gpio-cells = <2>;
#interrupt-cells = <3>;
#dma-cells = <1>;
#iommu-cells = <1>;
#phy-cells = <1>;
#power-domain-cells = <1>;
```

The leading `#` is part of the property name. The value is a cell count, not the number of resources exported by the provider.

A multi-function provider can expose several classes with different counts:

```dts
system_controller: system-controller@8000 {
        #clock-cells = <2>;
        #reset-cells = <1>;
        #power-domain-cells = <1>;
};
```

Never substitute one count for another merely because the phandle target is the same node.

## Zero-Cell Providers

A zero-cell provider needs no selector because the phandle uniquely identifies the resource:

```dts
osc: clock-24000000 {
        compatible = "fixed-clock";
        #clock-cells = <0>;
        clock-frequency = <24000000>;
};

device@1000 {
        clocks = <&osc>;
};
```

Zero cells does not mean zero resources or an empty property. The phandle itself is still present. Adding an invented `0` would create an extra cell and violate the provider binding.

## Multi-Provider Lists Have Variable Entry Widths

This property can be valid:

```dts
clocks = <&osc>,
         <&pll 3>,
         <&clock_controller 4 1>;
```

If the providers use 0, 1, and 2 argument cells respectively, the parser advances by different lengths. A raw dump of numeric cells loses visible tuple boundaries, so resolve each phandle before advancing.

Do not parse a compiled array as fixed pairs such as `(phandle, ID)` unless every permitted provider binding proves that shape.

## Consumer Binding And Provider Binding Have Different Jobs

The consumer binding defines:

- whether the property is required
- how many resource entries are allowed
- their semantic order
- whether a names property is required
- constraints specific to this consumer

The provider binding defines:

- its `#*-cells` count
- the meaning and allowed values of argument cells
- relationships among exported resources

Correct decoding needs both.

## Resource Names

Many list properties have a companion string list:

```dts
clocks = <&clock_controller 3>, <&clock_controller 7>;
clock-names = "bus", "core";

resets = <&reset_controller 2>, <&reset_controller 5>;
reset-names = "bus", "core";
```

Names map positionally:

| Index | Clock entry | Clock name |
|---:|---|---|
| 0 | `<&clock_controller 3>` | `bus` |
| 1 | `<&clock_controller 7>` | `core` |

The name describes the resource's function at the consumer. It does not rename the provider output.

Bindings guidance recommends explicit ordering and matching constraints for phandle lists and their names. Reordering the resources without the names silently changes which resource a driver obtains.

## Names And Linux Lookup

Driver APIs commonly request the consumer-facing name:

```c
bus_clk = devm_clk_get(dev, "bus");
core_rst = devm_reset_control_get_exclusive(dev, "core");
rx_dma = dma_request_chan(dev, "rx");
usb_phy = devm_phy_get(dev, "usb");
```

The framework maps that name to the matching index in `clock-names`, `reset-names`, `dma-names`, or `phy-names`, then decodes the corresponding provider entry.

Names are ABI. Changing `"core"` to `"functional"` without changing a binding and all compatible consumers can break resource lookup.

## Optional And Required Resources

Optionality comes from the consumer binding and driver API contract, not from phandle syntax. An absent optional property differs from:

- a present but malformed property
- a phandle to a disabled provider
- a valid provider that has not probed
- an argument value rejected by the provider

Optional getter APIs often return `NULL` for a genuinely absent resource but propagate errors for broken dependencies. Treating every error as “not fitted” hides board-description defects.

## A Mechanical Decoding Worksheet

For each consumer property, record:

| Field | Question |
|---|---|
| consumer binding | What entries and names are expected? |
| property index | Which logical function is this? |
| provider path | Where does the phandle resolve? |
| provider compatible | Which binding applies? |
| `#*-cells` | How many argument cells follow? |
| raw arguments | What exact cells are present? |
| decoded meaning | What does the provider binding call each cell? |
| provider state | Is the node available and its driver registered? |

This worksheet scales from a fixed clock to a multi-level IOMMU or interrupt nexus.

## Common Errors

- Treating `#clock-cells` as the number of clocks exported.
- Adding `<&fixed_clock 0>` to a zero-cell provider.
- Assuming every provider of one resource class uses the same argument count.
- Decoding argument values from a header constant without reading the provider binding.
- Allowing `*-names` count to drift from the resource list.
- Reordering phandles but not names.
- Calling a missing required resource optional in the DTS instead of fixing the binding mismatch.

## Exercises

1. Split `<&a &b 3 &c 4 5>` when `a`, `b`, and `c` declare 0, 1, and 2 cells.
2. Explain why `#gpio-cells = <2>` says nothing by itself about which cell contains flags.
3. Map three clocks to `clock-names` and show the effect of swapping two names.
4. List the consumer-binding and provider-binding questions needed to decode one entry.
5. Explain the difference between absent optional resource and unavailable provider.

## References And Next Step

- [Devicetree Specification](https://devicetree-specification.readthedocs.io/en/stable/devicetree-basics.html)
- [Linux binding design guidance](https://docs.kernel.org/devicetree/bindings/writing-bindings.html)
- [Linux DeviceTree kernel API](https://docs.kernel.org/devicetree/kernel-api.html)

Continue with [Clocks, Resets, And Power Domains](clocks-resets-and-power-domains.md).

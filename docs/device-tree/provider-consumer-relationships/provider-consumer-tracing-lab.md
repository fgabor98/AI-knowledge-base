---
status: draft
reviewed: false
domain: device-tree
difficulty: intermediate
last_reviewed: null
---

# Provider-Consumer Tracing Lab

## Goal

Decode one consumer with mixed provider types, prove the effective entries in a DTB, and diagnose deliberate count, name, and supplier failures.

## Lab Tree

Create `trainer-dependencies.dts`:

```dts
/dts-v1/;

/ {
        compatible = "example,trainer-dependency-lab";
        #address-cells = <1>;
        #size-cells = <1>;

        osc: clock-24000000 {
                compatible = "fixed-clock";
                #clock-cells = <0>;
                clock-frequency = <24000000>;
        };

        reg_3v3: regulator-3v3 {
                compatible = "regulator-fixed";
                regulator-name = "lab-3v3";
                regulator-min-microvolt = <3300000>;
                regulator-max-microvolt = <3300000>;
        };

        providers {
                clock_controller: clock-controller {
                        compatible = "example,lab-clock-controller";
                        #clock-cells = <2>;
                };

                reset_controller: reset-controller {
                        compatible = "example,lab-reset-controller";
                        #reset-cells = <1>;
                };

                gpio0: gpio-controller {
                        compatible = "example,lab-gpio-controller";
                        gpio-controller;
                        #gpio-cells = <2>;
                        interrupt-controller;
                        #interrupt-cells = <2>;
                };

                dma0: dma-controller {
                        compatible = "example,lab-dma-controller";
                        #dma-cells = <1>;
                };

                phy0: phy-provider {
                        compatible = "example,lab-phy";
                        #phy-cells = <1>;
                };

                power0: power-controller {
                        compatible = "example,lab-power-controller";
                        #power-domain-cells = <1>;
                };
        };

        test_device: test-device@1000 {
                compatible = "example,lab-consumer";
                reg = <0x1000 0x100>;

                clocks = <&osc>, <&clock_controller 4 1>;
                clock-names = "reference", "bus";
                dmas = <&dma0 6>, <&dma0 7>;
                dma-names = "rx", "tx";
                interrupts-extended = <&gpio0 9 2>;
                phys = <&phy0 0>;
                phy-names = "link";
                power-domains = <&power0 3>;
                reset-gpios = <&gpio0 12 1>;
                resets = <&reset_controller 5>;
                reset-names = "core";
                vdd-supply = <&reg_3v3>;
        };
};
```

The compatible strings and numeric arguments are fictional. This lab teaches mechanics, not real binding validation.

## Step 1: Build And Capture Identity

```sh
dtc -@ -I dts -O dtb -o trainer-dependencies.dtb trainer-dependencies.dts
sha256sum trainer-dependencies.dtb
dtc -I dtb -O dts -o trainer-dependencies.final.dts trainer-dependencies.dtb
```

Record the tool version and output hash. Inspect generated phandles and `__symbols__`.

## Step 2: Complete A Decoding Table

Without looking at the decompiled output first, fill this table:

| Property/index | Provider | Cell-count property | Arguments | Consumer name |
|---|---|---|---|---|
| `clocks[0]` | | | | |
| `clocks[1]` | | | | |
| `dmas[0]` | | | | |
| `dmas[1]` | | | | |
| `interrupts-extended[0]` | | | | n/a |
| `phys[0]` | | | | |
| `power-domains[0]` | | | | n/a |
| `resets[0]` | | | | |
| `reset-gpios[0]` | | | | `reset` |
| `vdd-supply` | | no generic count | none | `vdd` |

Then compare your result with provider nodes in the decompiled tree.

## Step 3: Inspect Raw Cells

If `fdtget` is installed:

```sh
fdtget -tx trainer-dependencies.dtb /test-device@1000 clocks
fdtget -tx trainer-dependencies.dtb /test-device@1000 dmas
fdtget -tx trainer-dependencies.dtb /test-device@1000 interrupts-extended
fdtget -tx trainer-dependencies.dtb /test-device@1000 reset-gpios
fdtget trainer-dependencies.dtb /test-device@1000 clock-names
```

Raw numeric output does not preserve tuple boundaries. Resolve each numeric phandle to its provider before counting arguments.

## Step 4: Trace Consumer Names To APIs

Map the DTS to conceptual driver requests:

```c
devm_clk_get(dev, "reference");
devm_clk_get(dev, "bus");
dma_request_chan(dev, "rx");
dma_request_chan(dev, "tx");
devm_phy_get(dev, "link");
devm_reset_control_get_exclusive(dev, "core");
devm_gpiod_get(dev, "reset", GPIOD_OUT_LOW);
devm_regulator_get(dev, "vdd");
```

For each call, identify the property, names list if any, selected index, provider, and arguments.

## Step 5: Break A Zero-Cell Entry

Change:

```dts
clocks = <&osc>, <&clock_controller 4 1>;
```

to:

```dts
clocks = <&osc 0>, <&clock_controller 4 1>;
```

Compile and record warnings. Even if a blob is produced, explain how the extra cell destroys correct entry parsing. Restore the source.

## Step 6: Break A Names List

Reverse only `clock-names`:

```dts
clock-names = "bus", "reference";
```

The cells and phandles remain structurally valid, but named lookup now returns the wrong resource. Restore it, then remove one DMA name and explain why schema cardinality checks are essential.

## Step 7: Break A Provider Count

Change the clock controller to `#clock-cells = <1>` without changing its consumer. Rebuild and inspect how the remaining cell can be interpreted as the beginning of another entry. This demonstrates why provider bindings and consumers must evolve together.

Restore the count and source.

## Step 8: Model Supplier Unavailability

Add:

```dts
status = "disabled";
```

to `dma0`. The consumer's phandle still resolves and compilation can succeed. Explain the runtime difference among:

- phandle cannot resolve
- provider node is disabled
- provider driver is not configured
- provider driver defers
- provider registers but rejects specifier 6

These cases need different fixes despite similar consumer symptoms.

## Step 9: Draw The Dependency Graph

Draw the consumer and all direct suppliers. Then add plausible second-level dependencies: the PHY consumes a clock and regulator; DMA belongs to a power domain; the GPIO controller needs a bus clock. Mark the earliest supplier that could cause several downstream deferrals.

## Review Questions

1. Which properties embed a provider phandle in every entry?
2. Why does `interrupts` require a different first decoding step from `interrupts-extended`?
3. Which lab relationship has no `#*-cells` property?
4. Why can the same GPIO provider use two different cell-count properties?
5. What evidence distinguishes wrong consumer name from missing provider?
6. Why is property order not a resource sequencing language?

## Completion Checklist

- [ ] I decoded a mixed-width provider list.
- [ ] I handled a zero-cell provider correctly.
- [ ] I mapped `*-names` to driver lookup names.
- [ ] I distinguished GPIO and interrupt domains on one provider.
- [ ] I traced DMA, PHY, power-domain, reset, and regulator suppliers.
- [ ] I demonstrated failures that `dtc` alone may not reject.
- [ ] I can distinguish malformed, disabled, unconfigured, deferred, and rejected suppliers.
- [ ] I can draw a multi-level dependency graph and choose the earliest useful debugging point.

## References And Next Steps

- [Linux Devicetree bindings guidance](https://docs.kernel.org/devicetree/bindings/writing-bindings.html)
- [Linux DeviceTree kernel API](https://docs.kernel.org/devicetree/kernel-api.html)
- [Linux probe failure debugging](../../linux-kernel/debugging/probe-failure-debugging.md)
- [Device Tree binding validation](../../build-systems/advanced/linux-kernel/device-tree-binding-validation.md)

Return to [Provider-Consumer Relationships](../provider-consumer-relationships.md), or continue with [Standard Nodes And Properties](../standard-nodes-and-properties.md).

---
status: draft
reviewed: false
domain: device-tree
difficulty: intermediate
last_reviewed: null
---

# Regulators, Supplies, And Board Constraints

The regulator graph describes electrical rails and their consumers. It also constrains what software may request from programmable hardware. A permissive but inaccurate constraint is dangerous: the PMIC may support a voltage that the board or silicon cannot tolerate.

## Consumer Supply Properties

Consumers name their rails with binding-defined `*-supply` properties that carry a bare regulator phandle ([Regulator Supplies](../provider-consumer-relationships/dma-iommu-phy-and-regulator-dependencies.md#regulator-supplies)). The stem matches the driver's supply name (`vddio` resolves `vddio-supply`), not the regulator node name, PMIC output name, or schematic rail label.

## Fixed Regulators

A board rail controlled by a GPIO can use the generic fixed-regulator binding. The AM642 SK SD-card load switch (`k3-am642-sk.dts`):

```dts
vdd_mmc1: regulator-2 {
        /* TPS2051BD */
        compatible = "regulator-fixed";
        regulator-name = "vdd_mmc1";
        regulator-min-microvolt = <3300000>;
        regulator-max-microvolt = <3300000>;
        regulator-boot-on;
        enable-active-high;
        vin-supply = <&vcc_3v3_sys>;
        gpio = <&exp1 3 GPIO_ACTIVE_HIGH>;
};
```

The node uses the fixed-regulator binding's historical `gpio` plus its separate `enable-active-high`. Do not replace it mechanically with a consumer-style `gpios` property, and note that for this compatible Linux ignores the GPIO flag entirely: `drivers/gpio/gpiolib-of.c` takes the polarity only from `enable-active-high`.

A rail with selectable voltages uses `regulator-gpio`. The AM62x LP SK switches SD I/O between 1.8 V and 3.3 V for UHS modes (`k3-am62-lp-sk.dts`):

```dts
vddshv_sdio: regulator-4 {
        compatible = "regulator-gpio";
        regulator-name = "vddshv_sdio";
        regulator-min-microvolt = <1800000>;
        regulator-max-microvolt = <3300000>;
        regulator-boot-on;
        vin-supply = <&ldo1_reg>;
        gpios = <&main_gpio0 31 GPIO_ACTIVE_HIGH>;
        states = <1800000 0x0>,
                 <3300000 0x1>;
};

&sdhci1 {
        vmmc-supply = <&vdd_mmc1>;
        vqmmc-supply = <&vddshv_sdio>;
};
```

`startup-delay-us` describes how long the supply takes to become usable after enable. `off-on-delay-us` constrains rapid cycling. Neither property substitutes for a power-good signal when hardware requires one.

## PMIC Regulators

PMIC bindings commonly contain a `regulators` subnode with provider-specific child names. The LP8733 on the PHYTEC phyCORE-AM64x (`k3-am64-phycore-som.dtsi`, trimmed):

```dts
pmic@61 {
        compatible = "ti,lp8733";
        reg = <0x61>;

        buck0-in-supply = <&vcc_5v0_som>;
        buck1-in-supply = <&vcc_5v0_som>;

        regulators {
                vdd_core: buck0 {
                        regulator-name = "VDD_CORE";
                        regulator-min-microvolt = <750000>;
                        regulator-max-microvolt = <750000>;
                        regulator-always-on;
                        regulator-boot-on;
                };

                vdd_3v3: buck1 {
                        regulator-name = "VDD_3V3";
                        regulator-min-microvolt = <3300000>;
                        regulator-max-microvolt = <3300000>;
                        regulator-always-on;
                        regulator-boot-on;
                };
        };
};
```

Every rail is pinned to one voltage and always on: AM64x has no Linux-driven DVFS, so the board forbids any voltage change. The input supplies (`buck0-in-supply`) connect the PMIC into the regulator graph.

The child node name, allowed properties, modes, and current limits are provider-specific. Use the PMIC schema in addition to the generic regulator schema.

Do not confuse similarly named timing constraints. `regulator-ramp-delay` is a slew rate in microvolts per microsecond, while `regulator-enable-ramp-delay` and the settling-time properties are durations in microseconds. Using a measured time as a slew rate can make DVFS transitions wait too little or far too long.

## Constraints Are Board Safety Data

A PMIC buck may physically generate a wide range while the board safely permits one value; the phyCORE `VDD_CORE` above is constrained to exactly 0.75 V. DT constraints restrict the framework to the board range. Validate them against:

- the consumer absolute maximum and recommended operating values
- PMIC accuracy and transient behavior
- PCB voltage drop and load range
- regulator ramp rate and settling time
- silicon speed grade, fuse, and temperature limits
- every other consumer sharing the rail

Never widen constraints merely to make a driver's voltage request succeed. Determine whether the request, OPP data, or board description is wrong.

## `boot-on` And `always-on`

These properties are not interchangeable:

- `regulator-boot-on` says the rail is expected to be enabled at boot and that software should preserve or enable it during initial handoff; consumers still determine long-term use.
- `regulator-always-on` says the system must not disable the regulator during normal operation.

Use `always-on` only for a real system constraint, such as a rail feeding non-switchable logic. It can conceal missing consumer references and increase suspend power if used as a debugging shortcut.

## Supply Chains And Shared Rails

`vin-supply` connects one regulator to its upstream source. The resulting graph allows ordering and load propagation. It should mirror the power schematic, including intermediate load switches that Linux controls.

Several consumers can share one rail. The framework combines compatible voltage and enable requests, but it cannot make contradictory hardware requirements compatible. If one consumer needs 1.8 V and another needs 3.3 V on a physically shared rail, the design or description is wrong.

## GPIO Enable Ownership

When a fixed-regulator node owns an enable GPIO, no consumer should also request that GPIO directly. Consumers request their `*-supply`; the regulator framework owns enable counts, polarity, and delays. Duplicating the GPIO creates contention and bypasses dependency ordering.

## Runtime Evidence

With regulator debugfs available:

```sh
cat /sys/kernel/debug/regulator/regulator_summary
dmesg | grep -Ei 'regulator|supply|voltage'
```

Review use counts, open counts, requested voltage, constraints, and consumers. Names can be framework or board labels; correlate them through the live DT and provider driver.

Measure rails when behavior matters. A framework “enabled” state does not prove the PMIC output reached the load, the enable polarity is correct, or the rail remained within tolerance during a transition.

## Common Failure Patterns

- A missing mandatory supply causes failure or repeated deferral.
- A misspelled supply property may lead a driver to use a dummy regulator, depending on its API and configuration.
- Overly narrow constraints reject a valid OPP; overly wide constraints permit unsafe requests.
- Missing `vin-supply` hides real upstream ordering.
- Incorrect `always-on` keeps an island powered and masks missing users.
- An enable GPIO's polarity or delay is wrong even though the consumer probes.
- Two logical regulator nodes incorrectly describe one physical rail.

## Authoritative References

- [Linux voltage and current regulator API](https://docs.kernel.org/driver-api/regulator.html)
- [Linux fixed-regulator binding schema](https://github.com/torvalds/linux/blob/master/Documentation/devicetree/bindings/regulator/fixed-regulator.yaml)
- [Linux regulator core binding schema](https://github.com/torvalds/linux/blob/master/Documentation/devicetree/bindings/regulator/regulator.yaml)

## Continue

Proceed to [Power Domains, Runtime PM, And Device Links](power-domains-runtime-pm-and-device-links.md).

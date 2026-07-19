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

A consumer binding defines semantic properties ending in `-supply`:

```dts
sensor@1a {
        compatible = "example,temp-sensor";
        reg = <0x1a>;
        vdd-supply = <&reg_3v3_sensor>;
        vddio-supply = <&reg_1v8_io>;
};
```

The property stem matches the consumer driver's supply name. It need not match the regulator node name, PMIC output name, or schematic rail label. A driver asking for `vddio` resolves `vddio-supply`.

Each phandle identifies a regulator provider. Unlike clocks, ordinary supply references have no specifier cells after the phandle.

## Fixed Regulators

A board rail controlled by a GPIO can use the generic fixed-regulator binding:

```dts
reg_3v3_sensor: regulator-3v3-sensor {
        compatible = "regulator-fixed";
        regulator-name = "sensor-3v3";
        regulator-min-microvolt = <3300000>;
        regulator-max-microvolt = <3300000>;
        gpio = <&gpio2 4 GPIO_ACTIVE_HIGH>;
        enable-active-high;
        startup-delay-us = <5000>;
        vin-supply = <&reg_5v0_main>;
};
```

The example uses the fixed-regulator binding's historical `gpio` plus its separate `enable-active-high` semantics. Do not replace it mechanically with a consumer-style `gpios` property; follow the exact schema supported by the kernel version and binding.

`startup-delay-us` describes how long the supply takes to become usable after enable. `off-on-delay-us` constrains rapid cycling. Neither property substitutes for a power-good signal when hardware requires one.

## PMIC Regulators

PMIC bindings commonly contain a `regulators` subnode with provider-specific child names:

```dts
pmic@2d {
        compatible = "example,board-pmic";
        reg = <0x2d>;

        regulators {
                vdd_cpu: buck1 {
                        regulator-name = "vdd-cpu";
                        regulator-min-microvolt = <800000>;
                        regulator-max-microvolt = <1100000>;
                        regulator-ramp-delay = <12500>;
                };
        };
};
```

The child node name, allowed properties, modes, and current limits are provider-specific. Use the PMIC schema in addition to the generic regulator schema.

Do not confuse similarly named timing constraints. `regulator-ramp-delay` is a slew rate in microvolts per microsecond, while `regulator-enable-ramp-delay` and the settling-time properties are durations in microseconds. Using a measured time as a slew rate can make DVFS transitions wait too little or far too long.

## Constraints Are Board Safety Data

The provider may physically generate 0.6–1.5 V, while the board safely permits only 0.8–1.1 V. DT constraints restrict the framework to the board range. Validate them against:

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

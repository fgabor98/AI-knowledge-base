---
status: draft
reviewed: false
domain: device-tree
difficulty: intermediate
last_reviewed: null
---

# Operating Points, DVFS, And Performance States

An operating performance point is a supported hardware state, commonly a frequency-voltage pair with optional power, latency, bandwidth, or domain-state requirements. An OPP table is safety data and capability data. It is not a demand to run at every listed frequency.

## The `operating-points-v2` Relationship

The AM625 Cortex-A53 table (`k3-am625.dtsi`, three of six entries shown):

```dts
cpu0: cpu@0 {
        compatible = "arm,cortex-a53";
        operating-points-v2 = <&a53_opp_table>;
        clocks = <&k3_clks 135 0>;
        #cooling-cells = <2>;
        /* ... */
};

a53_opp_table: opp-table {
        compatible = "operating-points-v2-ti-cpu";
        opp-shared;
        syscon = <&opp_efuse_table>;

        opp-200000000 {
                opp-hz = /bits/ 64 <200000000>;
                opp-supported-hw = <0x01 0x0007>;
                clock-latency-ns = <6000000>;
        };

        opp-1000000000 {
                opp-hz = /bits/ 64 <1000000000>;
                opp-supported-hw = <0x01 0x0006>;
                clock-latency-ns = <6000000>;
        };

        opp-1250000000 {
                opp-hz = /bits/ 64 <1250000000>;
                opp-supported-hw = <0x01 0x0004>;
                clock-latency-ns = <6000000>;
                opp-suspend;
        };
};
```

The 64-bit frequency encoding is significant. `opp-hz` is in hertz, voltages in microvolts, and power values such as `opp-microwatt` in microwatts. Unit mistakes can remain syntactically valid while producing a catastrophically wrong table.

This table has no `opp-microvolt` at all: on AM625 Linux changes only the frequency, through the TI-SCI clock, while the core rail stays fixed. The 6 ms `clock-latency-ns` reflects a firmware-mediated clock change, not a PLL lock time.

## Voltage Forms And Tolerance

An OPP can specify one target voltage or a minimum/target/maximum triplet where the binding permits:

```dts
opp-microvolt = <900000 925000 950000>;
```

The table must agree with regulator constraints. The OPP target cannot make an unsafe regulator range safe, and wide regulator constraints cannot legitimize an invalid silicon operating point.

If a device has multiple supplies, the OPP binding can express multiple voltage values in the provider-defined order. Coupled rails require careful review of transition order, tolerances, and framework/driver support.

## Shared OPP Tables

`opp-shared` means devices sharing the table must transition together because they share the underlying performance domain; all four AM625 A53 cores reference `a53_opp_table` and form one cpufreq policy. It is not merely a source-deduplication hint.

For CPUs, several CPU nodes can reference one shared table when the hardware uses a common clock/voltage domain. If cores can scale independently, falsely marking the table shared sacrifices behavior; omitting it for a truly coupled cluster can produce conflicting requests.

## Supported Hardware And Variants

SoCs often ship in speed bins, revisions, or process variants. The AM625 table above is the union of every speed grade, gated per entry by `opp-supported-hw = <revision eFuse-mask>`. The `ti-cpufreq` driver reads the speed-grade eFuse through the `syscon` phandle (`opp_efuse_table`, a 4-byte register in the wake-up control module) and enables only matching entries: per the binding, the second cell's bits are “the eFuse bits that indicate the particular OPP is available.” `am625_efuse_xlate()` turns the eFuse value into a cumulative mask for the K, S, and T speed grades (bits 0, 1, 2), so 1.25 GHz (`0x0004`) runs only on T-grade parts, 1 GHz (`0x0006`) on S and T, and the lower frequencies (`0x0007`) on all three.

Do not create a union table like this unless the selection mechanism reliably removes unsafe entries before scaling begins, as `ti-cpufreq` does here. Validate the slowest silicon, voltage tolerance, and thermal extremes—not only a typical bench unit.

`status = "disabled"` can mark an OPP unavailable by default where the schema allows it. Board DTS layers may enable an OPP only when hardware evidence justifies the override.

## Required OPPs And Domain States

An OPP can depend on an OPP in another performance domain:

```dts
opp-600000000 {
        opp-hz = /bits/ 64 <600000000>;
        opp-microvolt = <950000>;
        required-opps = <&soc_domain_high>;
};
```

This can express a required generic power-domain performance state or another coupled resource relationship defined by the binding. It does not replace direct supplies or clocks. Trace the referenced table and determine which provider aggregates and applies the request.

## CPUFreq, Devfreq, And Policy

The OPP library stores and validates states. CPUFreq selects CPU frequencies according to its driver, governor, policy limits, scheduler interactions, and thermal pressure. Devfreq performs an analogous role for many non-CPU devices.

Therefore:

- an OPP's presence means it may be used when available
- the governor decides when to request it
- the scaling driver implements the transition
- clocks/regulators/domains execute resource changes
- thermal cooling can cap the available range

An OPP table alone does not create CPUFreq or devfreq support.

## Transition Safety

When increasing performance, voltage commonly rises before frequency. When decreasing, frequency commonly falls before voltage. Drivers and frameworks must implement the hardware-specific order, including regulator ramp delay and clock transition latency.

Review failure rollback. If the voltage succeeds but the clock change fails, the device may safely remain overvolted but should not be left in an inconsistent software state. If a lower-voltage transition occurs too early, the result may be silent data corruption.

## Power And Energy Data

`opp-microwatt` can provide total power estimates for an OPP. The Energy Model may consume OPP information for energy-aware decisions. Values must use a consistent scale and defensible measurement/model assumptions. Invented monotonic numbers can mislead scheduling and thermal policy even if they look plausible.

## Runtime Validation

For CPUs, inspect policies rather than assuming one directory per CPU:

```sh
find /sys/devices/system/cpu/cpufreq -maxdepth 2 -type f
cat /sys/devices/system/cpu/cpufreq/policy0/scaling_available_frequencies
cat /sys/devices/system/cpu/cpufreq/policy0/scaling_cur_freq
```

For devfreq devices, inspect `/sys/class/devfreq/`. Correlate available frequencies with the live OPP table, disabled-bin logic, regulator voltage, clock rate, and thermal limits.

Test every enabled OPP under voltage, temperature, and workload corners. “The frequency appears in sysfs” is not stability validation.

## Authoritative References

- [Linux Operating Performance Points library](https://docs.kernel.org/power/opp.html)
- [Linux CPU performance scaling](https://docs.kernel.org/admin-guide/pm/cpufreq.html)
- [Linux Energy Model](https://docs.kernel.org/power/energy-model.html)
- [Linux OPP v2 base binding schema](https://github.com/torvalds/linux/blob/master/Documentation/devicetree/bindings/opp/opp-v2-base.yaml)

## Continue

Proceed to [Thermal Zones, Trips, And Cooling Maps](thermal-zones-trips-and-cooling-maps.md).

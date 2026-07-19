---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# Integrated Power Bring-Up Lab

This lab reviews a fictional accelerator whose usable state depends on a switched supply, power domain, clocks, resets, operating points, and thermal cooling. The compatible strings and numeric IDs are illustrative; real hardware must follow its exact schemas and integration manual.

## Objectives

By the end, you should be able to:

- draw a multi-framework supplier graph from DTS
- distinguish initial clock assignment from runtime scaling
- verify the voltage-rate contract between regulator constraints and OPPs
- derive a safe conceptual probe and runtime-PM sequence
- connect a thermal sensor trip to accelerator throttling
- diagnose five deliberate faults using runtime evidence

## Hardware Contract

Assume the board has:

- a 5 V always-on input rail
- a PMIC-controlled 0.80–0.95 V accelerator rail with 2 ms enable-ramp time
- an SoC power controller managing isolation and retention
- bus and core clocks; the core starts at 400 MHz
- separate bus and core reset controls
- supported accelerator states at 200, 400, and 600 MHz
- a nearby calibrated thermal sensor channel 2
- a passive trip at 85 °C and critical shutdown at 105 °C

The 600 MHz state requires 0.95 V and a high power-domain performance state. The accelerator driver registers a devfreq cooling device.

## Step 1: Read The Integrated Description

```dts
#include <dt-bindings/clock/example-soc.h>
#include <dt-bindings/power/example-soc.h>
#include <dt-bindings/reset/example-soc.h>
#include <dt-bindings/thermal/thermal.h>

reg_5v0_main: regulator-5v0-main {
        compatible = "regulator-fixed";
        regulator-name = "main-5v0";
        regulator-min-microvolt = <5000000>;
        regulator-max-microvolt = <5000000>;
        regulator-always-on;
};

&i2c1 {
        pmic@2d {
                compatible = "example,board-pmic";
                reg = <0x2d>;

                regulators {
                        reg_accel: buck1 {
                                regulator-name = "accel-vdd";
                                regulator-min-microvolt = <800000>;
                                regulator-max-microvolt = <950000>;
                                regulator-enable-ramp-delay = <2000>;
                                vin-supply = <&reg_5v0_main>;
                        };
                };
        };
};

accel_opp_table: opp-table-accel {
        compatible = "operating-points-v2";

        opp-200000000 {
                opp-hz = /bits/ 64 <200000000>;
                opp-microvolt = <800000>;
                opp-microwatt = <120000>;
        };

        opp-400000000 {
                opp-hz = /bits/ 64 <400000000>;
                opp-microvolt = <875000>;
                opp-microwatt = <260000>;
        };

        opp-600000000 {
                opp-hz = /bits/ 64 <600000000>;
                opp-microvolt = <950000>;
                opp-microwatt = <480000>;
                required-opps = <&accel_pd_high>;
        };
};

power_controller: power-controller@12000 {
        compatible = "example,soc-power-controller";
        reg = <0x12000 0x1000>;
        #power-domain-cells = <1>;
        operating-points-v2 = <&accel_pd_opp_table>;

        accel_pd_opp_table: opp-table {
                compatible = "operating-points-v2";

                accel_pd_nominal: opp-1 {
                        opp-level = <1>;
                };

                accel_pd_high: opp-2 {
                        opp-level = <2>;
                };
        };
};

thermal_sensor: thermal-sensor@13000 {
        compatible = "example,soc-thermal-sensor";
        reg = <0x13000 0x1000>;
        #thermal-sensor-cells = <1>;
};

accelerator: accelerator@60000 {
        compatible = "example,soc-accelerator";
        reg = <0x60000 0x10000>;

        vdd-supply = <&reg_accel>;
        power-domains = <&power_controller SOC_PD_ACCEL>;

        clocks = <&clock_controller SOC_CLK_ACCEL_BUS>,
                 <&clock_controller SOC_CLK_ACCEL_CORE>;
        clock-names = "bus", "core";
        assigned-clocks = <&clock_controller SOC_CLK_ACCEL_CORE>;
        assigned-clock-parents = <&clock_controller SOC_CLK_PLL_ACCEL>;
        assigned-clock-rates = <400000000>;

        resets = <&reset_controller SOC_RST_ACCEL_BUS>,
                 <&reset_controller SOC_RST_ACCEL_CORE>;
        reset-names = "bus", "core";

        operating-points-v2 = <&accel_opp_table>;
        #cooling-cells = <2>;
};

thermal-zones {
        accel_thermal: accel-thermal {
                polling-delay-passive = <250>;
                polling-delay = <1000>;
                thermal-sensors = <&thermal_sensor 2>;

                trips {
                        accel_passive: trip-passive {
                                temperature = <85000>;
                                hysteresis = <3000>;
                                type = "passive";
                        };

                        accel_critical: trip-critical {
                                temperature = <105000>;
                                hysteresis = <2000>;
                                type = "critical";
                        };
                };

                cooling-maps {
                        map-accel {
                                trip = <&accel_passive>;
                                cooling-device = <&accelerator
                                                  THERMAL_NO_LIMIT
                                                  THERMAL_NO_LIMIT>;
                        };
                };
        };
};
```

The `example,*` headers do not exist. Treat this as preprocessor-based review material, not a directly compilable board file.

## Step 2: Draw The Graph

Derive these edges without using property order:

```text
reg_5v0_main -> PMIC buck -> accelerator
power_controller[ACCEL] -> accelerator
clock_controller[BUS, CORE, PLL] -> accelerator
reset_controller[BUS, CORE] -> accelerator
accel_opp_table -> accelerator
accel_pd_opp_table[HIGH] -> accel_opp_table[600 MHz]
thermal_sensor[channel 2] -> accel_thermal
accelerator cooling device -> accel_thermal passive trip
```

For every edge, record the provider binding, cell count, semantic ID, owning Linux framework, and expected runtime evidence.

## Step 3: Review The Voltage Contract

Build a table:

| Frequency | OPP voltage | Within rail constraint? | Required domain state |
|---:|---:|---|---|
| 200 MHz | 0.800 V | yes | provider default/nominal |
| 400 MHz | 0.875 V | yes | provider default/nominal |
| 600 MHz | 0.950 V | yes, at maximum | high |

Then verify these numbers against the real silicon speed grade and rail tolerance. Passing this arithmetic check does not establish electrical safety.

## Step 4: Derive A Conceptual Enable Sequence

A plausible sequence is:

1. acquire supply, domain, clocks, and exclusive reset handles
2. enable `vdd` and wait for the regulator enable-ramp delay
3. runtime-resume the power domain and remove isolation through its provider
4. assert resets if the handoff state is undefined
5. set a safe initial OPP, raising voltage before frequency where required
6. enable bus and core clocks
7. deassert bus and core resets in the hardware-defined order
8. poll accelerator readiness before exposing it to workloads

The DTS does not prescribe this sequence. Compare it with the hardware manual and driver implementation. If the domain provider internally controls reset or clocks, remove duplicate ownership from the consumer design.

## Step 5: Collect Runtime Evidence

Adapt paths and permissions to the target:

```sh
cat /sys/kernel/debug/clk/clk_summary
cat /sys/kernel/debug/regulator/regulator_summary
cat /sys/bus/platform/devices/60000.accelerator/power/runtime_status
ls -l /sys/bus/platform/devices/60000.accelerator/supplier:*
find /sys/class/devfreq -maxdepth 2 -type f
find /sys/class/thermal -maxdepth 2 -type f
dmesg | grep -Ei 'accelerator|defer|clock|reset|regulator|opp|thermal'
```

Expected evidence includes:

- both clock roles resolve and the core starts near 400 MHz
- the accelerator appears as a consumer of `accel-0v9`
- runtime idle can release its domain and ordinary clocks
- devfreq exposes exactly the safe, enabled OPP frequencies
- thermal channel 2 creates the intended zone
- the cooling device links to the passive trip and changes state under heat/load

## Step 6: Exercise The Lifecycle

Test:

1. cold boot with the bootloader leaving the accelerator off and reset
2. cold boot with inherited firmware state, if supported
3. repeated runtime suspend/resume during workloads
4. every enabled OPP under sustained load
5. passive-trip crossing and recovery through hysteresis
6. system suspend from runtime-active and runtime-suspended states
7. driver unbind/rebind if the driver supports it
8. failure rollback when one supplier is unavailable

Use voltage/current measurement and clock observation where accessible. Kernel state alone does not prove the physical rail or clock.

## Step 7: Diagnose Deliberate Faults

### Fault A: The 600 MHz OPP Requests 1.0 V

The regulator maximum is 0.95 V. Do not widen the rail constraint immediately. Determine whether the OPP belongs to a different speed bin, whether the rail description is wrong, or whether 600 MHz is unsupported on this board. A safe temporary action is to disable the suspect OPP.

### Fault B: `assigned-clock-rates` Is Removed

The device may still work if the bootloader or driver selects a usable rate. Cold boot and unbind/rebind distinguish accidental inheritance from a complete driver contract. Decide from the consumer binding whether initial assignment belongs in DT or runtime configuration belongs entirely in the driver.

### Fault C: The Power-Domain Provider Never Probes

The consumer can remain deferred even though clocks and regulators exist. Resolve the domain phandle, check provider ancestors and driver configuration, then inspect supplier links. Raising the accelerator driver's init priority cannot create the missing domain provider.

### Fault D: Thermal Sensor Channel 0 Is Used

The zone reports plausible temperatures but does not track accelerator load. Correlate channel temperature with controlled accelerator and CPU workloads, inspect the SoC thermal map, and confirm calibration. Plausibility is weaker evidence than spatial correlation and documentation.

### Fault E: Runtime Resume Enables Clocks Before The Rail

The first register access after idle intermittently faults. Trace runtime-PM, regulator, and clock events; measure the enable and clock pins; compare against the 2 ms rail delay. Fix ownership and driver/provider sequencing rather than inflating an unrelated autosuspend delay.

## Exit Review

You have completed the lab when you can present:

- the annotated dependency graph
- a binding-backed decode of every tuple and supply
- a validated OPP/voltage/domain-state table
- enable, disable, rollback, suspend, and resume sequences
- runtime evidence for each framework
- a thermal test showing trip, cooling-state response, and hysteresis
- root-cause evidence for each deliberate fault

## Authoritative References

- [Linux Common Clock Framework](https://docs.kernel.org/driver-api/clk.html)
- [Linux reset controller API](https://docs.kernel.org/driver-api/reset.html)
- [Linux regulator API](https://docs.kernel.org/driver-api/regulator.html)
- [Linux runtime power management](https://docs.kernel.org/power/runtime_pm.html)
- [Linux OPP library](https://docs.kernel.org/power/opp.html)
- [Linux thermal sysfs API](https://docs.kernel.org/driver-api/thermal/sysfs-api.html)

## Continue

Proceed to [Common Peripheral Nodes](../common-peripheral-nodes.md).

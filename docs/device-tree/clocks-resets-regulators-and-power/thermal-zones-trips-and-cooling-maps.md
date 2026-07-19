---
status: draft
reviewed: false
domain: device-tree
difficulty: intermediate
last_reviewed: null
---

# Thermal Zones, Trips, And Cooling Maps

Thermal DT data connects temperature sensing to hardware safety limits and available mitigation. A complete path contains a sensor, a thermal zone, trip points, registered cooling devices, and mappings between trips and cooling states.

## Sensor Providers

A thermal sensor provider declares `#thermal-sensor-cells` according to its binding:

```dts
tsens: thermal-sensor@13000 {
        compatible = "example,soc-tsens";
        reg = <0x13000 0x1000>;
        #thermal-sensor-cells = <1>;
};
```

The argument might select a physical sensor channel. Calibration data may come from nvmem, firmware, or provider registers. A plausible temperature is not proof that the right channel or calibration was selected.

## Thermal Zones And Trips

```dts
#include <dt-bindings/thermal/thermal.h>

thermal-zones {
        gpu_thermal: gpu-thermal {
                polling-delay-passive = <250>;
                polling-delay = <1000>;
                thermal-sensors = <&tsens 2>;

                trips {
                        gpu_passive: trip-passive {
                                temperature = <85000>;
                                hysteresis = <3000>;
                                type = "passive";
                        };

                        gpu_critical: trip-critical {
                                temperature = <105000>;
                                hysteresis = <2000>;
                                type = "critical";
                        };
                };
        };
};
```

Temperatures and hysteresis are expressed in millicelsius. The values must reflect the correct physical location and platform limit. A sensor under the CPU cluster may not safely represent a distant PMIC or accelerator hotspot.

Hysteresis prevents rapid toggling around a threshold. It is not a substitute for validating sensor noise, thermal inertia, governor behavior, and the latency of the cooling action.

## Trip Types

Common trip types have different intent:

- `passive` asks the framework to reduce heat generation through cooling devices
- `active` commonly starts active cooling such as a fan
- `hot` signals a severe condition requiring platform-specific action
- `critical` is the final safety threshold at which shutdown is expected

Do not rely on a critical trip as the normal control loop. The passive/active system should stabilize the product below safety shutdown during supported workloads and ambient conditions.

## Cooling Devices And Maps

A frequency-scaled device can register as a cooling device when its driver/subsystem supports it:

```dts
gpu@60000 {
        #cooling-cells = <2>;
};
```

The thermal zone maps a trip to allowed cooling-state limits:

```dts
cooling-maps {
        map-gpu {
                trip = <&gpu_passive>;
                cooling-device = <&gpu THERMAL_NO_LIMIT THERMAL_NO_LIMIT>;
                contribution = <1024>;
        };
};
```

The two cooling cells represent minimum and maximum cooling states according to the generic binding. State numbering is cooling-device specific; for frequency cooling, higher cooling state normally means more throttling, not a higher frequency.

`THERMAL_NO_LIMIT` delegates the bound to the cooling device. Explicit ranges can restrict which states a trip may use. Confirm that the mapped device actually registers and that its state range matches the intended mitigation.

## Governors Are Policy

The thermal core governor decides how to drive mapped cooling devices. DT describes zones, thresholds, and relationships; it should not be stretched into a workload-control language. Governor choice and tuning depend on kernel configuration and platform integration.

For the power allocator governor, sustainable power and coefficients can influence control behavior. Such values need measurements and a valid power model. Copying tuning constants between enclosures or heatsinks is not defensible.

## Polling And Interrupt-Driven Sensors

`polling-delay` controls checks outside passive mitigation; `polling-delay-passive` applies while passively cooling. Zero can be appropriate for interrupt-driven sensors when the driver reliably reports threshold events.

Very short polling wastes power and bus bandwidth. Very long polling can overshoot a critical temperature because the thermal system has finite latency. Derive values from thermal time constants, sensor behavior, and mitigation response.

## Runtime Inspection

Inspect registered objects:

```sh
for zone in /sys/class/thermal/thermal_zone*; do
        cat "$zone/type" "$zone/temp"
done

for cdev in /sys/class/thermal/cooling_device*; do
        cat "$cdev/type" "$cdev/cur_state" "$cdev/max_state"
done
```

Thermal-zone directories may expose trip temperatures, hysteresis, policy, and links to cooling devices depending on the kernel. Map sysfs objects by their `type`; numbering is not stable.

Validation should exercise controlled heating or supported temperature emulation, observe trip crossing and hysteresis, confirm cooling-state changes, and prove temperature stabilizes. Never disable critical protection on hardware without an independent safe test plan.

## Failure Patterns

- The wrong sensor channel reports a believable but unrelated temperature.
- A trip uses degrees Celsius instead of millicelsius.
- A cooling map references a node whose driver never registers a cooling device.
- Cooling-state bounds are reversed or ineffective.
- The passive trip is too close to critical for the platform's thermal inertia.
- Polling is slower than the worst-case heating rate permits.
- OPP power data and thermal tuning use inconsistent or fictional assumptions.
- The test validates idle temperatures but not sustained worst-case workloads.

## Authoritative References

- [Linux thermal framework documentation](https://docs.kernel.org/driver-api/thermal/index.html)
- [Linux thermal sysfs API](https://docs.kernel.org/driver-api/thermal/sysfs-api.html)
- [Linux thermal-zone binding schema](https://github.com/torvalds/linux/blob/master/Documentation/devicetree/bindings/thermal/thermal-zones.yaml)
- [Linux thermal binding constants](https://github.com/torvalds/linux/blob/master/include/dt-bindings/thermal/thermal.h)

## Continue

Proceed to [Power Lifecycle, Ordering, And Diagnosis](power-lifecycle-ordering-and-diagnosis.md).

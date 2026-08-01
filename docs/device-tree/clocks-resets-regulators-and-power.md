---
status: draft
reviewed: false
domain: device-tree
difficulty: intermediate
last_reviewed: null
---

# Clocks, Resets, Regulators, And Power

A device that matches its driver may still be unusable. Its register interface can be clock-gated, its logic held in reset, its voltage rail absent, its isolation asserted, or its permitted performance point incompatible with the board. These relationships form a dependency graph that Linux must understand before it can operate the device safely.

## Learning Outcomes

After completing this module, you should be able to:

- trace clock consumers through gates, muxes, dividers, and parent clocks
- distinguish consumed clocks from board-level clock assignments
- review reset controls according to their exclusive, shared, or pulse semantics
- model fixed and PMIC-controlled regulators, their upstream supplies, and safe constraints
- distinguish a voltage supply from a generic power domain
- explain how generic power domains, runtime PM, and device links interact
- construct and review `operating-points-v2` tables for CPUs and other devices
- connect thermal sensors, trips, governors, and cooling devices without embedding policy accidentally
- derive safe enable, suspend, resume, and shutdown ordering from hardware requirements
- diagnose missing suppliers, permanent probe deferral, invalid constraints, and cross-subsystem failures

## Prerequisites

Complete [Pinctrl, GPIOs, And Interrupts](pinctrl-gpios-and-interrupts.md). This module assumes you can resolve phandles and specifiers, distinguish DT state from driver state, and inspect the live tree and subsystem debug interfaces.

## Learning Path

1. [Clock Trees, Consumers, And Assignments](clocks-resets-regulators-and-power/clock-trees-consumers-and-assignments.md)
2. [Reset Controllers And Safe Sequencing](clocks-resets-regulators-and-power/reset-controllers-and-safe-sequencing.md)
3. [Regulators, Supplies, And Board Constraints](clocks-resets-regulators-and-power/regulators-supplies-and-board-constraints.md)
4. [Power Domains, Runtime PM, And Device Links](clocks-resets-regulators-and-power/power-domains-runtime-pm-and-device-links.md)
5. [Operating Points, DVFS, And Performance States](clocks-resets-regulators-and-power/operating-points-dvfs-and-performance-states.md)
6. [Thermal Zones, Trips, And Cooling Maps](clocks-resets-regulators-and-power/thermal-zones-trips-and-cooling-maps.md)
7. [Power Lifecycle, Ordering, And Diagnosis](clocks-resets-regulators-and-power/power-lifecycle-ordering-and-diagnosis.md)
8. [Integrated Power Bring-Up Lab](clocks-resets-regulators-and-power/integrated-power-bring-up-lab.md)

## Keep The Resource Models Distinct

| Resource | Physical meaning | Consumer property | Usually controlled by |
|---|---|---|---|
| clock | timed signal feeding logic | `clocks` | common clock framework |
| reset | state or pulse that initializes logic | `resets` | reset controller framework |
| regulator | electrical voltage/current supply | `*-supply` | regulator framework |
| power domain | shared switch, isolation, retention, or firmware-managed island | `power-domains` | generic PM domain framework |
| performance point | supported combination of rate, voltage, power, and level | `operating-points-v2` | OPP plus scaling subsystem |
| thermal relationship | sensor, threshold, and mitigation path | `thermal-sensors`, `trips`, `cooling-maps` | thermal framework |

These resources can originate in one PMIC or system controller, but that does not merge their namespaces or semantics. A clock ID does not identify a reset. A regulator rail is not a power domain. An OPP table does not enable a clock, and a thermal trip does not itself guarantee that a useful cooling device exists.

## Topology, Constraints, And Policy

Good review separates three questions:

1. **Topology:** what is physically connected to what?
2. **Constraints:** which states are electrically and thermally safe for this board?
3. **Policy:** when should Linux choose an allowed state?

Device Tree primarily describes the first two. Governors, drivers, firmware, workload managers, and user space usually implement the third. Properties such as `regulator-always-on`, OPP availability, or a critical thermal trip carry real behavioral consequences, but they still describe hardware requirements or safety limits rather than a workload strategy.

## The Complete Dependency Path

```text
upstream rail -> device supply -> power domain -> clocks -> reset release
                                      |              |
                                      +-> OPP voltage/rate selection
                                                    |
sensor -> thermal zone -> trip -> cooling map ------+
```

The exact order is hardware-specific. The diagram is a review aid, not a universal sequence. Some blocks require clocks before reset deassertion; some require isolation to remain asserted until the rail is stable; some firmware-controlled domains sequence everything internally.

## Completion Check

You are ready for [Common Peripheral Nodes](common-peripheral-nodes.md) when you can:

- decode every resource tuple using the correct provider binding
- explain why `assigned-clock-rates` is not a substitute for `clocks`
- select exclusive, shared, or pulse reset use from the hardware contract
- prove that regulator voltage constraints are safe for the board, not merely accepted by schema
- trace a runtime resume across device links, power domains, clocks, and supplies
- review an OPP table as coupled frequency, voltage, and power data
- follow a thermal trip to the cooling state it constrains
- distinguish a provider that has not probed from one that rejected an invalid request

## Authoritative References

- [Linux Common Clock Framework](https://docs.kernel.org/driver-api/clk.html)
- [Linux reset controller API](https://docs.kernel.org/driver-api/reset.html)
- [Linux voltage and current regulator API](https://docs.kernel.org/driver-api/regulator.html)
- [Linux device power-management basics](https://docs.kernel.org/driver-api/pm/devices.html)
- [Linux Operating Performance Points library](https://docs.kernel.org/power/opp.html)
- [Linux thermal framework documentation](https://docs.kernel.org/driver-api/thermal/index.html)

## Related Topics

- [Provider-Consumer Relationships](provider-consumer-relationships.md)
- [Pinctrl, GPIOs, And Interrupts](pinctrl-gpios-and-interrupts.md)
- [Runtime Inspection](runtime-inspection.md)

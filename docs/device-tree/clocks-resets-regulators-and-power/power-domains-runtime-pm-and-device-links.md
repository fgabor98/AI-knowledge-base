---
status: draft
reviewed: false
domain: device-tree
difficulty: intermediate
last_reviewed: null
---

# Power Domains, Runtime PM, And Device Links

A generic power domain groups devices behind shared power-management hardware or firmware. Runtime PM decides when an individual device can idle. Device links express supplier dependencies and ordering. These mechanisms cooperate, but none is a synonym for another.

## Domain Providers And Consumers

```dts
power_controller: power-controller@12000 {
        compatible = "example,soc-power-controller";
        reg = <0x12000 0x1000>;
        #power-domain-cells = <1>;
};

codec@50000 {
        power-domains = <&power_controller SOC_PD_AUDIO>;
};
```

The provider binding defines the cells. The ID may select an island managed through registers, a system controller, or firmware. A zero-cell provider node can represent one domain directly.

The domain may control switches, isolation, retention, resets, clocks, or firmware state. Consumers must not duplicate those internal controls unless their binding defines separate ownership.

## Hierarchical Domains

Power domains can themselves depend on a parent domain. Provider bindings choose how that hierarchy is represented. A subdomain must not remain on after its parent is powered off, and latency or performance requirements may propagate upward.

Do not infer the hierarchy from register layout or a marketing block diagram. Model the physical/firmware dependency accepted by the domain provider.

## A Domain Is Not A Regulator

A regulator is an electrical supply with voltage/current constraints and consumer enable requests. A power domain is a coordinated power-management context. A domain may internally control a regulator, but listing only `power-domains` does not describe a separately wired `vdd-supply`. Conversely, a supply phandle does not model isolation, retention, or firmware-managed island state.

If both relationships physically exist and the consumer binding exposes them, describe both.

## Runtime PM State

Runtime PM maintains per-device state and usage accounting. A driver or subsystem marks a device busy, allows autosuspend, and implements runtime suspend/resume callbacks. A generic PM domain can wrap those transitions by powering the shared domain when needed and turning it off only when allowed.

Important distinctions:

- a bound device can be runtime-suspended normally
- a domain can remain on because another member is active
- a device can be runtime-active while hardware is unusable due to a driver bug
- disabling runtime PM for diagnosis changes timing and may hide races

System suspend is a separate transition that coordinates the whole device hierarchy. Runtime-suspended state can sometimes be reused, but drivers and PM domains must implement the handoff correctly.

## Device Links And `fw_devlink`

Firmware-described consumer/supplier relationships can become device links. Links help enforce probe ordering, suspend/resume ordering, and runtime-PM dependencies. Linux's `fw_devlink` behavior can expose incomplete or cyclic DT dependency graphs that happened to work when probing was less constrained.

A domain membership says devices share power-management context. A device link says one device depends on a supplier device. The kernel device hierarchy plus links forms a directed graph; cycles need architectural analysis, not probe-priority hacks.

Repeated `-EPROBE_DEFER` is useful evidence at first. Permanent deferral usually means:

- the supplier node is disabled or absent
- the supplier driver is not enabled or failed probe
- the tuple is invalid
- a dependency cycle exists
- a firmware-owned provider is unavailable
- the consumer mistakenly treats an optional resource as mandatory

## Domain Performance States

Some generic PM domains expose performance states. Consumers can request states directly or associate them through OPP data and `required-opps`. A performance-state value belongs to that domain provider; it is not automatically a frequency, voltage, or clock ID.

When several devices share a domain, the provider aggregates their requests. Review the worst-case combined request and how it is released during idle.

## Latency And Residency

Powering an island off is useful only when the idle interval justifies transition cost. Domain providers can expose latency and residency information to the PM framework. These are runtime properties measured or known by the platform implementation, not reasons to add arbitrary DTS delays to consumers.

For real-time devices, validate:

- resume latency from the deepest allowed domain state
- whether context is retained
- interrupt and wake routing while the domain is off
- clock and interconnect restoration order
- first-access behavior after resume

## Runtime Inspection

Inspect the device's PM state and its supplier links:

```sh
cat /sys/bus/platform/devices/DEVICE/power/runtime_status
cat /sys/bus/platform/devices/DEVICE/power/runtime_usage
cat /sys/bus/platform/devices/DEVICE/power/control
ls -l /sys/bus/platform/devices/DEVICE/supplier:*
```

Exact sysfs availability depends on kernel configuration and bus. Tracepoints under `power` and `rpm` can reveal transitions and ordering. Pair them with provider logs, clock/regulator summaries, and measured rail state.

## Senior Review Questions

1. What state is lost when the domain powers off?
2. Which entity sequences isolation, reset, clocks, and rails?
3. Can the interrupt controller and wake path operate while the domain is off?
4. Which other devices keep the shared domain active?
5. Are supplier relationships acyclic and complete?
6. Who owns performance-state aggregation?
7. Do runtime and system suspend paths converge safely?

## Authoritative References

- [Linux device power-management basics](https://docs.kernel.org/driver-api/pm/devices.html)
- [Linux runtime power-management framework](https://docs.kernel.org/power/runtime_pm.html)
- [Linux device links](https://docs.kernel.org/driver-api/device_link.html)
- [Linux generic power-domain binding schema](https://github.com/torvalds/linux/blob/master/Documentation/devicetree/bindings/power/power-domain.yaml)

## Continue

Proceed to [Operating Points, DVFS, And Performance States](operating-points-dvfs-and-performance-states.md).

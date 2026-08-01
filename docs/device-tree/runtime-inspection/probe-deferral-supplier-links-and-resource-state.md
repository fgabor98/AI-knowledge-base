---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# Probe Deferral, Supplier Links, And Resource State

`-EPROBE_DEFER` means the consumer cannot probe yet because a required supplier/resource is not available. It is a retry state, not proof of a DT bug. Persistent deferral requires tracing the exact relationship from the live consumer property to its provider device and driver.

## Detect Deferred Devices

When debugfs and kernel support are available:

```bash
mountpoint -q /sys/kernel/debug || \
  sudo mount -t debugfs debugfs /sys/kernel/debug

sudo cat /sys/kernel/debug/devices_deferred
```

Modern drivers using `dev_err_probe()` can attach a reason visible with the deferred entry. This interface and text are diagnostic, configuration/version dependent, and not a stable machine-parsing ABI.

Capture it at several times:

```text
after device registration
after modules load
after late initcalls/userspace coldplug
after the expected supplier probes
```

Transient early deferral is normal. A device that later binds should disappear.

## Find The Consumer Relationship

For a reason such as “supplier regulator not ready”:

1. resolve consumer `of_node`
2. read its binding to identify the supply property name
3. inspect `vdd-supply` (for example) as a phandle
4. resolve phandle to provider live-node path
5. find provider Linux device through `of_node`
6. inspect provider availability, modalias, driver link, and logs
7. repeat recursively if the provider itself waits on another supplier

Do not search for a generic `regulator-names`; ordinary supplies are binding-named `*-supply` relationships.

## Device Links

Firmware-derived dependencies can produce supplier/consumer device links. Inspect a device directory:

```bash
find "$dev" -maxdepth 1 -type l \
  \( -name 'supplier:*' -o -name 'consumer:*' \) -printf '%f -> %l\n'
```

Exact link availability/naming depends on kernel version, `fw_devlink`, buses, and subsystem integration. Absence of a sysfs link does not prove absence of a real DT dependency.

Use links as corroborating evidence, then decode the DT property itself.

## Provider State Matrix

| Provider live node | provider device | driver link | Interpretation |
|---:|---:|---:|---|
| absent | absent | absent | wrong final tree/phandle or dynamic composition |
| present, unavailable | usually absent | absent | provider disabled by final DT |
| present, available | absent | absent | parent/population problem |
| present | present | absent | no match, module missing, failed/deferred provider probe |
| present | present | present | provider bound; inspect subsystem registration/resource ID |

A bound provider driver may still fail to export the exact clock/reset/domain/regulator requested by the consumer.

## Resource-Specific Evidence

Debugfs interfaces are diagnostic and version/configuration dependent. Useful examples include:

```text
/sys/kernel/debug/clk/clk_summary
/sys/kernel/debug/regulator/regulator_summary
/sys/kernel/debug/gpio
/sys/kernel/debug/pinctrl/
/sys/kernel/debug/devices_deferred
```

Other evidence:

```bash
cat /proc/interrupts
find "$dev" -maxdepth 1 -type l -name iommu_group -print -exec readlink -f {} \;
```

Always preserve full provider/consumer logs and read the subsystem binding. A clock named in DT can exist while its provider index is wrong; a regulator can be registered but constrained incompatibly.

## Common Resource Failure Signatures

| Resource | Typical log/behavior | DT checks |
|---|---|---|
| clock | failed to get/enable, defer | `clocks`, `clock-names`, provider `#clock-cells` |
| regulator | supply not found, defer, voltage error | exact `*-supply`, provider node/constraints |
| reset | lookup/assert/deassert error | `resets`, `reset-names`, provider cells |
| power domain | attach/defer/runtime PM failure | `power-domains`, provider/domain ID |
| GPIO | invalid descriptor/polarity | binding-named `*-gpios`, provider cells/flags |
| IRQ | no IRQ, trigger conflict, no counts | `interrupt-parent`, `interrupts`/extended, binding |
| DMA | channel lookup/defer | `dmas`, `dma-names`, provider cells |
| IOMMU | attach/map faults | `iommus`/maps, stream IDs, group/topology |
| PHY | PHY not found/defer | `phys`, `phy-names`, provider cells |
| firmware | request/auth/load failure | firmware name/interface, not necessarily DT |

The errno and resource name from `dev_err_probe()` are more useful than a generic “probe failed.”

## Failed Probe Versus Deferred Probe

Deferred:

- device exists
- no driver link yet
- appears in deferred diagnostics while waiting
- retried when suppliers/drivers become available

Failed with a terminal errno:

- device exists
- no driver link
- does not persist on deferred list
- log should identify error if driver reports it
- adding unrelated delay will not fix the contract

A failed probe can be retried by later manual bind or driver re-registration, but that does not make it deferred.

## Deferred-Probe Timeout

The kernel command line supports `deferred_probe_timeout=` as a debugging/control option whose exact behavior depends on current kernel code and participating drivers. It can trigger reporting and allow opted-in dependency checks to stop deferring.

Do not use a shorter timeout to hide a missing mandatory supplier. Fix provider creation, matching, configuration, or the DT relationship.

## Probe Ordering Is Not DTS Line Order

Moving provider text above consumer text does not create a reliable probe order. Device registration, driver registration, asynchronous probe, modules, and dependency links determine timing.

Model the hardware relationship accurately and let frameworks express dependency. If a driver reads a phandle manually and returns a guessed error, improve its probe diagnostics rather than ordering source files.

## Recursive Diagnosis Worksheet

```text
consumer device/of_node:
deferred reason and timestamp:
binding resource name:
raw consumer property:
resolved provider node/specifier:
provider availability:
provider Linux device/bus:
provider modalias/module/driver:
provider's own deferred/failure reason:
subsystem registration of requested resource:
first missing transition:
```

## Authoritative References

- [Linux driver infrastructure and `dev_err_probe`](https://docs.kernel.org/driver-api/infrastructure.html)
- [Linux driver model: probe and deferral](https://docs.kernel.org/driver-api/driver-model/driver.html)
- [Linux Devicetree Kernel API](https://docs.kernel.org/devicetree/kernel-api.html)
- [Linux kernel parameters](https://docs.kernel.org/admin-guide/kernel-parameters.html)

## Continue

Proceed to [Controlled Bind/Unbind And Runtime Forensics](controlled-bind-unbind-and-runtime-forensics.md).

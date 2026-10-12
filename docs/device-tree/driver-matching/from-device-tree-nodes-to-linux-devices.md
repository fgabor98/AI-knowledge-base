---
status: draft
reviewed: false
domain: device-tree
difficulty: intermediate
last_reviewed: null
---

# From Device Tree Nodes To Linux Devices

A node cannot match a driver until Linux has created an appropriate `struct device` for it. Population is bus-specific: platform devices, I2C clients, SPI devices, PCI functions, and auxiliary child devices enter the driver model through different paths.

## A Node Is Firmware Data, Not A Device Object

At early boot Linux unflattens the handed-off DTB into an in-memory tree of `struct device_node` objects. That makes properties queryable, but does not itself register every node as a Linux device.

Population code interprets selected nodes and creates devices on a bus:

```text
struct device_node
        ↓ population/enumeration policy
struct platform_device / i2c_client / spi_device / ...
        ↓ device_register()
device appears on a Linux bus
        ↓ bus match callback
driver probe
```

Consequently, a compatible visible under `/sys/firmware/devicetree/base` can coexist with no device under `/sys/bus/*/devices`.

## Platform-Device Population

Many non-discoverable SoC devices become `platform_device` instances. Architecture and OF platform code populate children of recognized buses such as `simple-bus`, subject to availability and platform rules.

On AM64x, `bus@f4000` is a `simple-bus`, so its enabled children become platform devices:

```dts
cbass_main: bus@f4000 {
        compatible = "simple-bus";
        /* ... */

        main_uart0: serial@2800000 {
                compatible = "ti,am64-uart", "ti,am654-uart";
                reg = <0x00 0x02800000 0x00 0x100>;
                interrupts = <GIC_SPI 178 IRQ_TYPE_LEVEL_HIGH>;
                /* status = "okay" from k3-am642-sk.dts */
        };
};
```

The UART node becomes the platform device `2800000.serial` (translated address plus node name) with an MMIO resource and an IRQ. `compatible` helps match it later; it is not by itself the command that creates it.

Not every child is populated this way. The `r5fss@78000000` cluster node is a platform device, but its `r5f@...` core children are created only when the K3 R5F driver probes the cluster and calls `devm_of_platform_populate()`.

A disabled bus is normally not populated, so an `okay` descendant remains absent as a platform device. A missing or incorrect bus compatible can similarly prevent traversal even when leaf nodes look valid.

## Controller-Enumerated Children

An I2C controller is often a platform device, but its children become `i2c_client` devices after the I2C controller driver registers an adapter and the I2C core enumerates firmware children. On the AM642 SK, `main_i2c1` (`20010000.i2c`) must probe before the PCA9538 expander at `0x70` and the TMP100 at `0x48` exist as I2C clients such as `1-0048`; the adapter number depends on probe order and aliases.

```text
SoC population → platform I2C controller → controller probe
→ i2c_adapter registration → DT child enumeration → i2c_client
→ I2C driver match and probe
```

SPI follows an analogous controller-and-child sequence. If the controller fails to probe, no amount of debugging the sensor driver's match table will create the child device.

This also means parent probe deferral can delay the existence of every child rather than produce deferred entries for each child driver.

## Discoverable Buses

PCI and USB enumerate devices using protocol-defined mechanisms. Device Tree normally describes the host controller and non-discoverable integration data. Enumerated vendor/device/class IDs drive primary bus matching, although a firmware node may augment a fixed device.

Do not assume every DT `compatible` is the primary match key for every bus. The bus's `match()` callback defines the identity mechanism and can consider OF, ACPI, native ID tables, or overrides in a bus-specific order.

## Multi-Function And Child Devices

A single DT node can expose several functions through one driver or multiple kernel subsystems. Child nodes are appropriate when hardware subfunctions have their own DT resources or bindings, not merely to force a driver instance.

Drivers may create MFD cells, auxiliary devices, PHYs, clocks, GPIO controllers, or other objects after their own probe. These descendants can have a Linux device-model relationship that does not map one-to-one to DT child nodes.

When tracing a device, record both:

- the firmware topology (`of_node` and DT path)
- the Linux device hierarchy and bus membership

## Evidence Of Each Stage

### Runtime node

```sh
find /sys/firmware/devicetree/base -name compatible -print
tr '\0' '\n' </sys/firmware/devicetree/base/bus@f4000/serial@2800000/compatible
```

### Linux device

```sh
find /sys/bus/platform/devices -maxdepth 2 -type l -name of_node -print
find /sys/bus/i2c/devices -maxdepth 2 -type l -name of_node -print
find /sys/bus/spi/devices -maxdepth 2 -type l -name of_node -print
```

Resolve an `of_node` symlink to connect the device instance back to its DT path. Names vary by bus and kernel; never construct the expected sysfs name solely from the DT node name.

### Population logs

```sh
dmesg | grep -i -E 'platform|i2c|spi|populate|probe'
```

Use dynamic debug or subsystem tracepoints when ordinary logs are insufficient, on a development system with an approved logging policy.

## Why A Node May Not Become A Device

- the node or an ancestor is disabled
- the final DTB does not contain the expected node
- firmware selected another DTB or overlay set
- the parent bus was not recognized for population
- the parent/controller driver did not probe
- the bus treats the device as discoverable and enumeration found nothing
- schema-invalid topology placed the node under the wrong parent
- platform policy intentionally reserves the hardware for another execution environment

These are device-existence failures, not driver-match failures.

## Senior Review Checklist

- Which exact component creates the `struct device` for each node class?
- Does child enumeration depend on a controller probe or firmware transaction?
- Are parent-bus availability and population compatibles correct?
- Are DT and Linux hierarchies intentionally different anywhere?
- Can overlays add/remove devices at runtime, and do drivers support that lifecycle?
- Do diagnostics link devices back to firmware paths rather than guessing names?
- Are secure-world, remote-processor, or hypervisor-owned nodes excluded intentionally?

## Authoritative References

- [Linux Devicetree usage model and platform population](https://docs.kernel.org/devicetree/usage-model.html)
- [Linux DeviceTree platform APIs](https://docs.kernel.org/devicetree/kernel-api.html)
- [Linux platform devices and drivers](https://docs.kernel.org/driver-api/driver-model/platform.html)
- [Linux driver-core binding model](https://docs.kernel.org/driver-api/driver-model/binding.html)

## Next Step

Continue with [`of_match_table`, Variant Data, And Probe Selection](of-match-table-variant-data-and-probe-selection.md).

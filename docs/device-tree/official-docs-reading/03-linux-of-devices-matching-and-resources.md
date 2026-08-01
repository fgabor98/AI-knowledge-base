---
status: active
reviewed: false
domain: device-tree
difficulty: intermediate
last_reviewed: null
---

# 3. Linux OF Model, Devices, Matching, And Resource Relationships

Official section: [Linux Open Firmware and Devicetree documentation](https://docs.kernel.org/devicetree/index.html)

Knowledge-guide companion: [Stage 3](knowledge-guide-companion.md#stage-3-linux-of-model-devices-matching-and-resource-relationships)

## Linux Tree Lifecycle

- [ ] **P0** [Linux and the Devicetree](https://docs.kernel.org/devicetree/usage-model.html).
- [ ] **P0** Trace architecture entry, early flat-tree scanning, unflattening, and the live `struct device_node` tree in the exact kernel.
- [ ] **P0** Identify when `/chosen`, root cells, and memory are read before ordinary device population.
- [ ] **P0** Understand availability from `status` and distinguish node existence from Linux device creation.
- [ ] **P0** Trace `of_platform_populate()` or the architecture/bus-specific population path used by the project.
- [ ] **P1** [DeviceTree Kernel API](https://docs.kernel.org/devicetree/kernel-api.html) as an API reference after reading real consumers.
- [ ] **P1** [Open Firmware Devicetree unittest](https://docs.kernel.org/devicetree/of_unittest.html) and `drivers/of/unittest.c`.

## Device Nodes And Lifetime

- [ ] **P0** `struct device_node`, properties, parents/children, full names, phandles, and availability helpers.
- [ ] **P0** `of_node_get()`/`of_node_put()` and lifetime rules.
- [ ] **P0** Property read APIs for strings, arrays, variable arrays, booleans, and raw byte data.
- [ ] **P0** Parse phandle and phandle-with-args APIs without assuming provider cell meaning.
- [ ] **P1** Changesets and dynamic-tree APIs only after ordinary boot-tree behavior is understood.
- [ ] **P2** Legacy OF APIs when maintaining code that still uses them.

## From Node To Linux Device

- [ ] **P0** Platform-device creation for suitable root/simple-bus descendants.
- [ ] **P0** I2C/SPI/MDIO/PCI and other bus-specific child enumeration; not every DT child becomes a platform device.
- [ ] **P0** Firmware-node association and the `of_node` symlink in sysfs.
- [ ] **P0** Bus modalias generation and module autoloading.
- [ ] **P0** Driver-core [binding model](https://docs.kernel.org/driver-api/driver-model/binding.html): match, probe, bind, unbind, and driver links.
- [ ] **P1** Device links and supplier/consumer ordering in the exact kernel version.

## Compatible Matching

- [ ] **P0** Ordered compatible strings as hardware/ABI contracts.
- [ ] **P0** `MODULE_DEVICE_TABLE(of, ...)`, `struct of_device_id`, and module aliases.
- [ ] **P0** `of_match_device()`, match-data retrieval, and variant data.
- [ ] **P0** Distinguish OF match, bus match, module loading, driver registration, probe, successful bind, and subsystem function.
- [ ] **P1** Board/root compatible handling and architecture/platform selection where relevant.
- [ ] **P1** ACPI/fwnode abstraction only to understand drivers shared across firmware-description models.

## Provider-Consumer Relationships

Read the generic consumer/provider documentation and the exact binding together:

- [ ] **P0** clocks and `#clock-cells`/`clocks`/`clock-names`
- [ ] **P0** resets and `#reset-cells`/`resets`/`reset-names`
- [ ] **P0** GPIOs and `#gpio-cells`/`*-gpios`
- [ ] **P0** interrupt parents/domains/specifiers
- [ ] **P0** regulators and `*-supply`
- [ ] **P0** power domains and `#power-domain-cells`
- [ ] **P0** DMA controllers and `#dma-cells`/`dmas`/`dma-names`
- [ ] **P0** IOMMUs and `#iommu-cells`/`iommus`
- [ ] **P0** PHY providers and `#phy-cells`/`phys`/`phy-names`
- [ ] **P1** interconnects, nvmem cells, resets through syscon, and other project-specific providers

For each relationship, resolve the provider first and decode arguments using that provider's binding.

## Probe And Deferral

- [ ] **P0** `-EPROBE_DEFER` and [driver infrastructure guidance](https://docs.kernel.org/driver-api/infrastructure.html).
- [ ] **P0** `dev_err_probe()` behavior and deferred-reason reporting.
- [ ] **P0** `/sys/kernel/debug/devices_deferred` where enabled.
- [ ] **P0** Distinguish absent device, no match, unloaded module, failed probe, deferred probe, bound driver, and functional subsystem.
- [ ] **P1** `fw_devlink` and device-link behavior from the exact kernel/configuration.

## Source-Tracing Exercises

- [ ] Trace one root/simple-bus node from unflattening to platform device and driver probe.
- [ ] Trace one I2C or SPI child through bus registration rather than platform population.
- [ ] Start with a live `*-supply` or `clocks` property and identify the provider node, Linux device, driver, and physical resource.
- [ ] Use a modalias to find the matching module alias and OF table entry.
- [ ] Diagnose one intentional missing provider and identify the exact deferred-probe reason.
- [ ] Find one node that exists in the live tree but correctly has no corresponding Linux device.

## Stage Completion

- [ ] I can trace boot FDT bytes into the live OF tree and then into the correct Linux bus device.
- [ ] I can explain matching, module loading, probe, deferral, binding, and function as separate stages.
- [ ] I can decode any phandle specifier by locating the provider binding and cell contract.
- [ ] I can preserve node references and property data with correct lifetime and type handling.


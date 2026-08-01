---
status: draft
reviewed: false
domain: device-tree
difficulty: intermediate
last_reviewed: null
---

# Driver Matching

Driver matching is only one stage in the path from a Device Tree node to working hardware. The node must exist in the final tree, Linux must instantiate a device on the correct bus, a driver must be present, the bus must match it, and `probe()` must acquire every required resource successfully.

This module treats those stages separately so that “the driver did not load” becomes a testable diagnosis rather than a catch-all description.

## Learning Outcomes

After completing this module, you should be able to:

- design an ordered `compatible` list with only genuine fallbacks
- distinguish root board compatibility from peripheral-driver matching
- decide whether a hardware change needs a new compatible string
- explain how DT nodes become platform, I2C, SPI, or other Linux devices
- connect an `of_match_table` entry to per-variant driver data
- trace a DT modalias through `modpost`, `depmod`, kmod, and module loading
- distinguish module presence, bus match, probe entry, probe success, and runtime function
- translate binding `required` and optional properties into robust probe behavior
- diagnose disabled, unpopulated, unmatched, deferred, failed, and successfully bound devices

## Prerequisites

Complete [Addressing And Bus Modeling](addressing-and-bus-modeling.md). You should already be able to decode resources and provider relationships in the final DTB.

## Learning Path

1. [Compatible Contracts And Fallback Chains](driver-matching/compatible-contracts-and-fallback-chains.md)
2. [Compatible Evolution And Stable ABI Decisions](driver-matching/compatible-evolution-and-stable-abi-decisions.md)
3. [From Device Tree Nodes To Linux Devices](driver-matching/from-device-tree-nodes-to-linux-devices.md)
4. [`of_match_table`, Variant Data, And Probe Selection](driver-matching/of-match-table-variant-data-and-probe-selection.md)
5. [Modaliases, Module Metadata, And Autoloading](driver-matching/modaliases-module-metadata-and-autoloading.md)
6. [Binding-Driven Probe Contracts](driver-matching/binding-driven-probe-contracts.md)
7. [Driver Matching And Probe Diagnosis Lab](driver-matching/driver-matching-and-probe-diagnosis-lab.md)

## The End-To-End Pipeline

```text
source DTS
  ↓ compile, include, overlay, firmware mutation
effective runtime node and status
  ↓ bus-specific population/enumeration
struct device on a Linux bus
  ↓ modalias and driver availability
candidate driver
  ↓ bus match against of_match_table or other ID table
probe() called
  ↓ resources, suppliers, hardware initialization
bound device
  ↓ functional validation
working subsystem
```

Each arrow has different evidence and different owners. A module listed by `lsmod` proves neither that a device exists nor that `probe()` ran. A `driver` symlink proves binding, not correct clocks, interrupts, DMA, or external wiring.

## Five Identities To Keep Separate

| Identity | Example | Purpose |
|---|---|---|
| DT compatible | `vendor,uart-v2` | hardware programming-model contract |
| DT path | `/soc/serial@4000` | firmware-node location |
| Linux device name | `4000.serial` | device-model instance identity |
| driver name | `vendor-uart` | registered driver identity |
| module name | `vendor_uart` | loadable object/kmod identity |

They may resemble one another, but Linux does not require the strings to be identical. Debugging by name similarity is unreliable; follow the actual links and ID tables.

## Scope Boundary

This module explains matching and the probe contract. Designing binding schemas in depth comes later in [Binding Design And Stable ABI](binding-design-and-stable-abi.md) and [Writing And Validating Binding Schemas](writing-and-validating-binding-schemas.md).

## Completion Check

You are ready for [Pinctrl, GPIOs, And Interrupts](pinctrl-gpios-and-interrupts.md) when you can:

- defend every fallback compatible as truly backward-compatible
- show the exact event that creates the Linux device
- prove whether a module alias exists for a compatible string
- identify the matched `of_device_id` and its variant data
- explain the difference between no match and a negative probe return
- classify a missing optional property separately from a deferred supplier
- collect runtime evidence for every pipeline stage

## Authoritative References

- [Devicetree Specification: `compatible`](https://devicetree-specification.readthedocs.io/en/stable/devicetree-basics.html)
- [Linux Devicetree usage model](https://docs.kernel.org/devicetree/usage-model.html)
- [Linux driver-core binding model](https://docs.kernel.org/driver-api/driver-model/binding.html)
- [Linux DeviceTree kernel API](https://docs.kernel.org/devicetree/kernel-api.html)

## Related Topics

- [Standard Nodes And Properties](standard-nodes-and-properties.md)
- [Provider-Consumer Relationships](provider-consumer-relationships.md)
- [Linux Device Tree Matching From Drivers](../linux-kernel/fundamentals/device-tree-matching.md)

---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# Probe Failure Debugging

## What Problem Does This Solve?

Probe failures are common during board bring-up and driver development because matching, resources, providers, and power sequencing must all line up.

## Core Concepts

- missing device
- missing driver
- failed match
- missing resource
- `-EPROBE_DEFER`
- provider dependencies
- `dev_err_probe`
- bind and unbind

## Mental Model

Classify the failure first: no device, no driver, no match, missing resource, deferred provider, or runtime initialization failure.

## Practice Skeleton

- Trace a Device Tree node to a platform device.
- Trace a compatible string to a driver table.
- Add `dev_err_probe` to a missing dependency path.
- Inspect deferred probe state.

## Debugging Checklist

- Check runtime Device Tree.
- Check driver registration logs.
- Check modalias and module autoloading.
- Check clocks, resets, regulators, GPIOs, and IRQs.

## Related Topics

- [Driver Binding, Probe, And Remove](../fundamentals/driver-binding-probe-remove.md)
- [Device Tree Matching From Drivers](../fundamentals/device-tree-matching.md)
- [Resource Lookup And Managed Allocation](../fundamentals/resource-lookup-and-devm.md)

---
status: draft
reviewed: false
domain: device-tree
difficulty: intermediate
last_reviewed: null
---

# `of_match_table`, Variant Data, And Probe Selection

A driver's `of_match_table` declares which DT hardware contracts it implements. The matching entry can also carry immutable per-compatible data so one driver supports several safe variants without reading software-policy properties from DT.

## Complete Match Table Pattern

```c
#include <linux/mod_devicetable.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>

struct acme_uart_data {
        unsigned int fifo_depth;
        bool needs_status_readback;
};

static const struct acme_uart_data ax100_data = {
        .fifo_depth = 16,
};

static const struct acme_uart_data ax200_data = {
        .fifo_depth = 64,
        .needs_status_readback = true,
};

static const struct of_device_id acme_uart_of_match[] = {
        { .compatible = "acme,ax200-uart", .data = &ax200_data },
        { .compatible = "acme,ax100-uart", .data = &ax100_data },
        { }
};
MODULE_DEVICE_TABLE(of, acme_uart_of_match);

static int acme_uart_probe(struct platform_device *pdev)
{
        const struct acme_uart_data *data;

        data = device_get_match_data(&pdev->dev);
        if (!data)
                return dev_err_probe(&pdev->dev, -EINVAL,
                                     "missing variant data\n");

        /* Acquire resources, initialize hardware, register the UART. */
        return 0;
}

static struct platform_driver acme_uart_driver = {
        .probe = acme_uart_probe,
        .driver = {
                .name = "acme-uart",
                .of_match_table = acme_uart_of_match,
        },
};
module_platform_driver(acme_uart_driver);

MODULE_LICENSE("GPL");
```

The terminating empty entry is required. The array and variant data are `const` because they describe immutable hardware facts shared by instances.

## Match Table Versus Module Table

Two lines serve different stages:

```c
.of_match_table = acme_uart_of_match;
MODULE_DEVICE_TABLE(of, acme_uart_of_match);
```

The driver's table pointer lets the bus match an already registered driver. `MODULE_DEVICE_TABLE` exposes the IDs to `modpost`, which generates module alias metadata for autoloading. Omitting the macro can still allow matching after a manual module load, while automatic loading fails.

Built-in drivers do not need module loading, but keeping correct device-table metadata supports consistent tooling and configurations where the driver becomes modular.

## Selecting The Match

For a node:

```dts
compatible = "acme,ax200-uart", "acme,ax100-uart";
```

a table containing both entries should select AX200 data. An older driver containing only AX100 can match the fallback. If a driver's table accidentally has duplicate entries for the same compatible, table order can influence the tie; duplicates should be removed rather than used as control flow.

Do not treat the match table as a version comparison. Every supported literal string must appear explicitly or be represented by a legitimate fallback in the DT list.

## Variant Data Design

Good match data contains facts implied by the compatible:

- register offsets or layout descriptors
- supported feature flags
- FIFO depth fixed by the IP revision
- errata workarounds
- callback sets for genuinely different operations
- maximum channels or hardware limits

Avoid mutable state, per-board wiring, allocated resources, or values that can differ between two instances with the same compatible. Those belong in per-device state, properties, discoverable registers, or provider relationships.

Keep variant behavior explicit:

```c
if (data->needs_status_readback)
        readl(base + STATUS_REG);
```

Large forests of compatible-string comparisons inside `probe()` are harder to review and often signal missing structured match data.

## Generic Firmware Matching

`device_get_match_data()` is useful in drivers that support multiple firmware descriptions because it retrieves the bus/firmware match data through a generic interface. OF-specific helpers such as `of_match_device()` and `of_device_get_match_data()` remain available when code genuinely needs OF details.

A driver should not require `dev->of_node` if it advertises ACPI or other firmware support and the needed properties have generic device-property APIs. Match-table design and property APIs should agree with the firmware environments the driver claims.

## Driver Name Is Not The Compatible

`.driver.name = "acme-uart"` identifies the Linux driver. It need not equal `acme,ax100-uart`, the module filename, or the platform device's sysfs name. With OF-populated platform devices, compatible matching is the important contract.

Name-based platform matching exists for legacy/non-DT platform devices. Avoid relying accidentally on a matching driver/device name that hides a broken OF table.

## Multiple ID Tables

A driver can support OF, ACPI, platform IDs, I2C IDs, SPI IDs, or PCI IDs as appropriate. Each bus has its own matching order and data representation. Ensure the match data from every supported table maps to the same internal variant model.

Do not add an unrelated ID table only to make a test bind. It broadens the driver's claimed ABI and may attach to devices it cannot operate.

## Review Checklist

- Does every documented compatible appear in the correct driver's table?
- Is the sentinel present and the table referenced by the driver?
- Does `MODULE_DEVICE_TABLE` use the same array?
- Is match data immutable and complete for every entry?
- Are safe fallbacks handled without losing required quirks?
- Could legacy name matching mask an OF-table error?
- Do OF and ACPI paths select equivalent variant data?
- Are unsupported compatibles rejected rather than guessed from prefixes?

## Authoritative References

- [Linux DeviceTree matching APIs](https://docs.kernel.org/devicetree/kernel-api.html)
- [Linux platform driver matching](https://docs.kernel.org/driver-api/driver-model/platform.html)
- [Linux driver model](https://docs.kernel.org/driver-api/driver-model/driver.html)
- [Linux driver-core infrastructure](https://docs.kernel.org/driver-api/infrastructure.html)

## Next Step

Continue with [Modaliases, Module Metadata, And Autoloading](modaliases-module-metadata-and-autoloading.md).

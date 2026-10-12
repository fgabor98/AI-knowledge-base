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

The 8250 OMAP UART driver that binds every AM64x UART (`drivers/tty/serial/8250/8250_omap.c`, trimmed):

```c
struct omap8250_platdata {
        struct omap8250_dma_params *dma_params;
        u8 habit;
};

static struct omap8250_platdata am654_platdata = {
        .dma_params     = &am654_dma,
        .habit          = UART_HAS_EFR2 | UART_HAS_RHR_IT_DIS |
                          UART_RX_TIMEOUT_QUIRK | UART_HAS_NATIVE_RS485,
};

static const struct of_device_id omap8250_dt_ids[] = {
        { .compatible = "ti,am654-uart", .data = &am654_platdata, },
        { .compatible = "ti,omap2-uart" },
        { .compatible = "ti,omap3-uart" },
        { .compatible = "ti,omap4-uart", .data = &omap4_platdata, },
        { .compatible = "ti,am3352-uart", .data = &am33xx_platdata, },
        { .compatible = "ti,am4372-uart", .data = &am33xx_platdata, },
        { .compatible = "ti,dra742-uart", .data = &omap4_platdata, },
        {},
};
MODULE_DEVICE_TABLE(of, omap8250_dt_ids);

static int omap8250_probe(struct platform_device *pdev)
{
        const struct omap8250_platdata *pdata;
        /* ... */
        pdata = of_device_get_match_data(&pdev->dev);
        if (pdata)
                priv->habit |= pdata->habit;
        /* ... */
}

static struct platform_driver omap8250_platform_driver = {
        .driver = {
                .name           = "omap8250",
                .of_match_table = omap8250_dt_ids,
                /* ... */
        },
        .probe                  = omap8250_probe,
        /* ... */
};
module_platform_driver(omap8250_platform_driver);
```

The terminating empty entry is required. Match data is optional per entry: `ti,omap2-uart` and `ti,omap3-uart` carry none, and probe handles `NULL`. The data objects describe immutable hardware facts shared by every instance of a compatible.

`drivers/remoteproc/ti_k3_r5_remoteproc.c` uses the stricter pattern in which every entry must have data, and probe fails if it does not:

```c
data = of_device_get_match_data(&pdev->dev);
if (!data) {
        dev_err(dev, "SoC-specific data is not defined\n");
        return -ENODEV;
}
```

## Match Table Versus Module Table

Two lines serve different stages:

```c
.of_match_table = omap8250_dt_ids,
MODULE_DEVICE_TABLE(of, omap8250_dt_ids);
```

The driver's table pointer lets the bus match an already registered driver. `MODULE_DEVICE_TABLE` exposes the IDs to `modpost`, which generates module alias metadata for autoloading. Omitting the macro can still allow matching after a manual module load, while automatic loading fails.

Built-in drivers do not need module loading, but keeping correct device-table metadata supports consistent tooling and configurations where the driver becomes modular.

## Selecting The Match

For `compatible = "ti,am64-uart", "ti,am654-uart";` the table above has no `ti,am64-uart` entry, so the fallback selects `am654_platdata`; a future table that added `ti,am64-uart` would win for the same DTB ([How Linux Uses A List](compatible-contracts-and-fallback-chains.md#how-linux-uses-a-list)). Remove duplicate entries for the same compatible rather than relying on table order, and list every supported literal string explicitly; the table is not a version comparison.

## Variant Data Design

Good match data contains facts implied by the compatible:

- register offsets or layout descriptors
- supported feature flags
- FIFO depth fixed by the IP revision
- errata workarounds
- callback sets for genuinely different operations
- maximum channels or hardware limits

Avoid mutable state, per-board wiring, allocated resources, or values that can differ between two instances with the same compatible. Those belong in per-device state, properties, discoverable registers, or provider relationships.

Keep variant behavior explicit. `8250_omap` tests the habit flags copied from match data at each point where hardware differs:

```c
/* omap_8250_shutdown() */
serial_out(up, UART_OMAP_WER, 0);
if (priv->habit & UART_HAS_EFR2)
        serial_out(up, UART_OMAP_EFR2, 0x0);
```

Large forests of compatible-string comparisons inside `probe()` are harder to review and often signal missing structured match data.

## Generic Firmware Matching

`device_get_match_data()` is useful in drivers that support multiple firmware descriptions because it retrieves the bus/firmware match data through a generic interface. OF-specific helpers such as `of_match_device()` and `of_device_get_match_data()` remain available when code genuinely needs OF details.

A driver should not require `dev->of_node` if it advertises ACPI or other firmware support and the needed properties have generic device-property APIs. Match-table design and property APIs should agree with the firmware environments the driver claims.

## Driver Name Is Not The Compatible

`.driver.name = "omap8250"` identifies the Linux driver. It equals neither `ti,am654-uart`, the module name `8250_omap`, nor the platform device's sysfs name `2800000.serial`. With OF-populated platform devices, compatible matching is the important contract.

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

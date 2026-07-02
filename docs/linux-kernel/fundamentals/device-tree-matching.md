---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# Device Tree Matching From Drivers

## What Problem Does This Solve?

Device Tree matching connects board-level hardware descriptions to reusable driver code without hard-coding board details in the driver.

The board says:

```dts
compatible = "example,demo-v2", "example,demo";
```

The driver says:

```c
{ .compatible = "example,demo-v2", .data = &demo_v2 },
{ .compatible = "example,demo", .data = &demo_v1 },
```

The platform bus uses that relationship to decide whether `probe()` should run and which hardware variant data the driver should use.

## Core Concepts

- `compatible`
- fallback compatible strings
- `struct of_device_id`
- `of_match_table`
- `MODULE_DEVICE_TABLE()`
- match data
- `of_device_get_match_data()`
- `device_get_match_data()`
- firmware node
- required properties
- optional properties
- binding documentation
- modalias
- module autoloading

## Mental Model

Device Tree describes what hardware exists and how it is wired. The driver advertises which compatible devices it supports and reads only the properties defined by the binding.

```text
runtime Device Tree node
  compatible = "vendor,device-rev2", "vendor,device"

driver of_match_table
  supports "vendor,device"

bus match
  calls probe()
```

The most specific compatible string should appear first in Device Tree. The driver may choose exact variant data based on the matched string.

## Basic Match Table

```c
#include <linux/mod_devicetable.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>

static const struct of_device_id demo_of_match[] = {
    { .compatible = "example,demo-device" },
    { }
};
MODULE_DEVICE_TABLE(of, demo_of_match);
```

Attach it to the driver:

```c
static struct platform_driver demo_driver = {
    .probe = demo_probe,
    .remove = demo_remove,
    .driver = {
        .name = "demo",
        .of_match_table = demo_of_match,
    },
};
```

The empty entry terminates the table. Do not omit it.

## Why `MODULE_DEVICE_TABLE()` Matters

For loadable modules, `MODULE_DEVICE_TABLE()` exports match aliases into module metadata.

Inspect:

```sh
modinfo demo.ko | grep alias
```

Example:

```text
alias: of:N*T*Cexample,demo-device
```

This helps userspace load the module automatically when a matching device appears.

Without it:

- manual `insmod demo.ko` may still work
- manual `modprobe demo` may still work if named directly
- automatic loading from modalias may fail

For built-in drivers, autoloading is irrelevant, but the match table is still needed for binding.

## Compatible String Ordering

Device Tree:

```dts
compatible = "example,demo-v2", "example,demo";
```

Meaning:

```text
try exact v2 behavior first
fall back to generic demo behavior if needed
```

Driver:

```c
static const struct of_device_id demo_of_match[] = {
    { .compatible = "example,demo-v2", .data = &demo_v2 },
    { .compatible = "example,demo", .data = &demo_v1 },
    { }
};
```

Avoid putting a generic fallback first in Device Tree:

```dts
compatible = "example,demo", "example,demo-v2"; /* bad order */
```

The first string should be the most specific.

## Match Data For Hardware Variants

Use `.data` to attach compile-time variant information:

```c
struct demo_variant {
    u32 fifo_depth;
    bool has_dma;
    bool needs_reset_delay;
};

static const struct demo_variant demo_v1 = {
    .fifo_depth = 16,
    .has_dma = false,
};

static const struct demo_variant demo_v2 = {
    .fifo_depth = 64,
    .has_dma = true,
    .needs_reset_delay = true,
};

static const struct of_device_id demo_of_match[] = {
    { .compatible = "example,demo-v1", .data = &demo_v1 },
    { .compatible = "example,demo-v2", .data = &demo_v2 },
    { }
};
MODULE_DEVICE_TABLE(of, demo_of_match);
```

Probe:

```c
static int demo_probe(struct platform_device *pdev)
{
    struct demo_priv *priv;

    priv = devm_kzalloc(&pdev->dev, sizeof(*priv), GFP_KERNEL);
    if (!priv)
        return -ENOMEM;

    priv->variant = of_device_get_match_data(&pdev->dev);
    if (!priv->variant)
        return dev_err_probe(&pdev->dev, -EINVAL,
                             "missing match data\n");

    dev_info(&pdev->dev, "fifo depth: %u\n", priv->variant->fifo_depth);
    return 0;
}
```

For drivers that may support multiple firmware systems, `device_get_match_data()` can be preferable because it abstracts over firmware match mechanisms where supported.

## Match Data Versus Device Tree Properties

Use match data for hardware facts tied to a compatible string:

- register layout variant
- FIFO depth fixed by IP revision
- feature bit availability
- required reset sequence for that silicon revision
- broken hardware quirk for a known revision

Use Device Tree properties for board wiring:

- which GPIO line resets the device
- which regulator supplies it
- which clock input is connected
- interrupt line
- bus address
- pinctrl state

Bad:

```dts
fifo-depth = <64>; /* if this is fixed by silicon revision */
```

Better:

```dts
compatible = "example,demo-v2", "example,demo";
```

and match data says v2 has a FIFO depth of 64.

Good board property:

```dts
reset-gpios = <&gpio2 5 GPIO_ACTIVE_LOW>;
```

because board wiring can vary.

## Reading Required Properties

Example binding requirement:

```dts
sample-rate-hz = <1000>;
```

Driver:

```c
u32 sample_rate;
int ret;

ret = device_property_read_u32(&pdev->dev, "sample-rate-hz", &sample_rate);
if (ret)
    return dev_err_probe(&pdev->dev, ret,
                         "missing sample-rate-hz\n");
```

Use `device_property_*()` helpers when possible for firmware-agnostic property reads. Use `of_property_*()` when you specifically need OF behavior.

## Reading Optional Properties

Example:

```dts
use-crc;
timeout-ms = <500>;
```

Driver:

```c
priv->use_crc = device_property_read_bool(&pdev->dev, "use-crc");

ret = device_property_read_u32(&pdev->dev, "timeout-ms", &priv->timeout_ms);
if (ret)
    priv->timeout_ms = 1000;
```

For optional values:

- define a sane default
- document it in the binding
- validate ranges

Example validation:

```c
if (priv->timeout_ms < 10 || priv->timeout_ms > 60000)
    return dev_err_probe(&pdev->dev, -EINVAL,
                         "timeout-ms out of range\n");
```

## Reading Strings And Arrays

String:

```c
const char *mode;

ret = device_property_read_string(&pdev->dev, "mode", &mode);
if (ret)
    mode = "normal";
```

Array:

```c
u32 thresholds[4];

ret = device_property_read_u32_array(&pdev->dev, "thresholds",
                                     thresholds, ARRAY_SIZE(thresholds));
if (ret)
    return dev_err_probe(&pdev->dev, ret,
                         "failed to read thresholds\n");
```

Do not parse arbitrary strings if an enum or numeric property would be clearer in the binding.

## Matching Without Device Tree

Many drivers support multiple match mechanisms:

```c
static const struct platform_device_id demo_id_table[] = {
    { "demo-mmio", 0 },
    { }
};
MODULE_DEVICE_TABLE(platform, demo_id_table);
```

Driver:

```c
static struct platform_driver demo_driver = {
    .probe = demo_probe,
    .remove = demo_remove,
    .id_table = demo_id_table,
    .driver = {
        .name = "demo-mmio",
        .of_match_table = demo_of_match,
    },
};
```

This helps when the same driver can bind to platform devices created without Device Tree.

## Debugging Matching

Check runtime compatible:

```sh
dtc -I fs -O dts /proc/device-tree > /tmp/running.dts
rg 'example,demo' /tmp/running.dts
```

Check platform device modalias:

```sh
find /sys/bus/platform/devices -name modalias -exec grep -H . {} \; 2>/dev/null
```

Check module alias:

```sh
modinfo demo.ko | grep alias
```

Check driver binding:

```sh
readlink /sys/bus/platform/devices/<device>/driver
```

Check logs:

```sh
dmesg | grep -i -E 'demo|probe|defer'
```

## Example: Variant-Aware Probe

```c
struct demo_variant {
    const char *name;
    u32 ctrl_offset;
    u32 status_offset;
};

struct demo_priv {
    struct device *dev;
    void __iomem *base;
    const struct demo_variant *variant;
};

static const struct demo_variant demo_a = {
    .name = "demo-a",
    .ctrl_offset = 0x00,
    .status_offset = 0x04,
};

static const struct demo_variant demo_b = {
    .name = "demo-b",
    .ctrl_offset = 0x10,
    .status_offset = 0x14,
};

static const struct of_device_id demo_of_match[] = {
    { .compatible = "example,demo-a", .data = &demo_a },
    { .compatible = "example,demo-b", .data = &demo_b },
    { }
};
MODULE_DEVICE_TABLE(of, demo_of_match);

static int demo_probe(struct platform_device *pdev)
{
    struct demo_priv *priv;
    u32 status;

    priv = devm_kzalloc(&pdev->dev, sizeof(*priv), GFP_KERNEL);
    if (!priv)
        return -ENOMEM;

    priv->dev = &pdev->dev;
    priv->variant = of_device_get_match_data(&pdev->dev);
    if (!priv->variant)
        return -EINVAL;

    priv->base = devm_platform_ioremap_resource(pdev, 0);
    if (IS_ERR(priv->base))
        return PTR_ERR(priv->base);

    status = readl(priv->base + priv->variant->status_offset);
    dev_info(&pdev->dev, "%s status=%#x\n",
             priv->variant->name, status);

    return 0;
}
```

## Common Failure Modes

| Symptom | Likely Cause | First Checks |
| --- | --- | --- |
| Driver never probes | incompatible strings do not match | source DTS, runtime DT, match table |
| Manual `insmod` works but autoload does not | missing `MODULE_DEVICE_TABLE()` | `modinfo alias` |
| Wrong variant behavior | fallback matched instead of specific string | compatible order and match data |
| Property read fails | property missing or wrong type | binding, `dtc` output |
| Probe fails with `-EINVAL` | driver rejected malformed DT | device logs |
| Works on one board, fails on another | board wiring property differs | compare runtime DTS |

## Common Mistakes

- Omitting the empty terminator in `of_device_id`.
- Forgetting `MODULE_DEVICE_TABLE(of, ...)`.
- Using match data for board wiring.
- Using Device Tree properties for fixed silicon facts.
- Accepting missing required properties silently.
- Inventing undocumented properties.
- Assuming source DTS and runtime Device Tree are identical.
- Using overly generic compatible strings.
- Putting fallback compatible strings first.

## Practice Exercises

### Exercise 1: Add Match Data

Add two compatible strings:

```c
{ .compatible = "example,demo-a", .data = &demo_a },
{ .compatible = "example,demo-b", .data = &demo_b },
```

Change the DT node and confirm probe selects the expected variant.

### Exercise 2: Inspect Autoload Metadata

Build as a module:

```sh
modinfo demo.ko | grep alias
```

Remove `MODULE_DEVICE_TABLE()` temporarily and compare metadata.

### Exercise 3: Validate Required Properties

Make a property required in code, remove it from DT, and observe the probe error. Then add it back and confirm successful probe.

## Debugging Checklist

- Is the runtime compatible string correct?
- Is the driver match table correct?
- Is the most specific compatible string first?
- Does the table have an empty terminator?
- Does a module include `MODULE_DEVICE_TABLE()`?
- Does `modinfo` show the expected alias?
- Does match data belong to hardware revision, not board wiring?
- Are required properties validated?
- Are optional properties defaulted and range-checked?
- Does the binding document every property?

## Related Topics

- [Device Tree Hardware Description](device-tree-hardware-description.md)
- [Platform Devices And Platform Drivers](platform-devices-and-drivers.md)
- [Device Tree Binding Validation](../../build-systems/advanced/linux-kernel/device-tree-binding-validation.md)
- [Probe Failure Debugging](../debugging/probe-failure-debugging.md)

## Official References

- [Linux and the Devicetree](https://docs.kernel.org/devicetree/usage-model.html)
- [Devicetree Bindings](https://docs.kernel.org/devicetree/bindings/index.html)
- [Driver Model](https://docs.kernel.org/driver-api/driver-model/index.html)

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

A driver probes only after several earlier steps succeed:

```text
hardware described or enumerated
-> device object created
-> driver registered
-> match succeeds
-> probe called
-> resources found
-> providers ready
-> hardware initialized
-> subsystem registration succeeds
```

If you skip classification, you can spend hours debugging `probe()` when the device never existed or the driver never matched.

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

```text
no device object:
  firmware/DT/ACPI/bus problem

device exists, no driver:
  match/autoload/registration problem

probe called, returns error:
  resource/provider/init/subsystem problem

probe defers:
  provider not ready or missing

probe succeeds, userspace missing:
  subsystem/class/udev problem
```

## Phase 1: Does The Device Exist?

Platform:

```sh
find /sys/bus/platform/devices -maxdepth 1 -name '*demo*' -print
```

I2C:

```sh
find /sys/bus/i2c/devices -maxdepth 2 -print
```

SPI:

```sh
find /sys/bus/spi/devices -maxdepth 2 -print
```

If no device exists, check:

- runtime Device Tree
- ACPI tables
- parent bus probe
- node `status`
- bus address or chip select
- kernel config for bus/controller
- wrong DTB deployed

Runtime Device Tree:

```sh
find /proc/device-tree -maxdepth 4 -name compatible -print
tr '\0' '\n' < /proc/device-tree/path/to/node/compatible
tr '\0' '\n' < /proc/device-tree/path/to/node/status
```

## Phase 2: Did The Driver Register?

Check module:

```sh
lsmod | grep demo
modinfo demo.ko
dmesg | grep -i demo
```

Check built-in driver logs or registration messages if available.

If module load fails:

```sh
dmesg | tail -100
modinfo demo.ko | grep vermagic
modinfo demo.ko | grep depends
```

Common causes:

- wrong kernel version
- missing dependency
- module signing rejection
- unknown symbol
- wrong architecture
- driver disabled in final `.config`

## Phase 3: Did Match Succeed?

Device modalias:

```sh
cat /sys/bus/platform/devices/48000000.demo/modalias
```

Module aliases:

```sh
modinfo demo.ko | grep alias
```

Driver symlink:

```sh
readlink /sys/bus/platform/devices/48000000.demo/driver
```

Device Tree match checklist:

- compatible string exactly matches driver table
- driver has `of_match_table`
- module has `MODULE_DEVICE_TABLE(of, ...)`
- node is enabled
- parent bus creates child devices
- driver registered on correct bus

For I2C/SPI, also check bus-specific IDs and module aliases.

## Phase 4: Was Probe Called?

Add narrow evidence:

```c
static int demo_probe(struct platform_device *pdev)
{
    struct device *dev = &pdev->dev;

    dev_dbg(dev, "probe start\n");
    ...
}
```

Enable dynamic debug:

```sh
echo 'func demo_probe +p' | sudo tee /sys/kernel/debug/dynamic_debug/control
```

Or use a temporary `dev_info()` only while bringing up a driver, then remove or downgrade it.

## Phase 5: What Error Did Probe Return?

Probe logs should preserve the original error.

Good:

```c
priv->clk = devm_clk_get(dev, NULL);
if (IS_ERR(priv->clk))
    return dev_err_probe(dev, PTR_ERR(priv->clk),
                         "failed to get clock\n");
```

Bad:

```c
if (IS_ERR(priv->clk))
    return -EINVAL;
```

Common error codes:

| Error | Meaning In Probe Context |
| --- | --- |
| `-EPROBE_DEFER` / `-517` | provider not ready yet |
| `-EINVAL` / `-22` | invalid property, argument, or state |
| `-ENODEV` / `-19` | expected device/resource absent |
| `-ENOENT` / `-2` | named resource/property missing |
| `-ENOMEM` / `-12` | allocation failed |
| `-ETIMEDOUT` / `-110` | hardware did not become ready |
| `-EIO` / `-5` | bus/hardware I/O failed |

Meaning depends on the API that returned it. Read the call site.

## Deferred Probe

Deferred probe is not automatically a bug. It means a provider was not ready when requested.

Check:

```sh
cat /sys/kernel/debug/devices_deferred 2>/dev/null
dmesg | grep -i defer
```

Common missing providers:

- regulator
- clock
- reset controller
- GPIO controller
- pinctrl provider
- PHY
- interrupt controller
- power domain

If a device stays deferred forever, the provider is likely missing, disabled, misdescribed, or not built.

## Resource Lookup Checklist

MMIO:

```c
regs = devm_platform_ioremap_resource(pdev, 0);
if (IS_ERR(regs))
    return PTR_ERR(regs);
```

IRQ:

```c
irq = platform_get_irq(pdev, 0);
if (irq < 0)
    return dev_err_probe(dev, irq, "failed to get irq\n");
```

Clock:

```c
clk = devm_clk_get(dev, "core");
if (IS_ERR(clk))
    return dev_err_probe(dev, PTR_ERR(clk),
                         "failed to get core clock\n");
```

Regulator:

```c
vdd = devm_regulator_get(dev, "vdd");
if (IS_ERR(vdd))
    return dev_err_probe(dev, PTR_ERR(vdd),
                         "failed to get vdd regulator\n");
```

Reset:

```c
rst = devm_reset_control_get_exclusive(dev, NULL);
if (IS_ERR(rst))
    return dev_err_probe(dev, PTR_ERR(rst),
                         "failed to get reset\n");
```

GPIO:

```c
gpiod = devm_gpiod_get(dev, "enable", GPIOD_OUT_LOW);
if (IS_ERR(gpiod))
    return dev_err_probe(dev, PTR_ERR(gpiod),
                         "failed to get enable gpio\n");
```

For each resource, check Device Tree property name, provider node, provider driver, and final config.

## Provider Debugfs

When debugfs is available:

```sh
cat /sys/kernel/debug/clk/clk_summary
cat /sys/kernel/debug/regulator/regulator_summary
cat /sys/kernel/debug/gpio
cat /sys/kernel/debug/pinctrl/*/pinmux-pins
cat /sys/kernel/debug/devices_deferred
```

These files help distinguish:

```text
consumer driver bug
provider missing
provider present but disabled
wrong supply/clock/GPIO name
wrong pinmux
```

## Hardware Initialization Failures

If resources are present but initialization fails, check:

- power rail enable order
- reset deassert timing
- clock rate
- pinmux
- bus address
- SPI mode
- I2C pull-ups/address conflicts
- MMIO register reset values
- runtime PM state
- required firmware

Add evidence in sequence:

```c
dev_dbg(dev, "power enabled\n");
dev_dbg(dev, "reset deasserted\n");
dev_dbg(dev, "chip id=%#x\n", id);
```

Use dynamic debug to enable only while diagnosing.

## Subsystem Registration Failure

Probe can initialize hardware and still fail later:

- character device registration
- sysfs group creation
- input device registration
- IIO device registration
- IRQ request
- DMA channel request
- firmware load

Do not assume a missing `/dev` node means probe never started. Check logs and sysfs.

## Manual Bind/Unbind

Useful lab commands:

```sh
echo 48000000.demo | sudo tee /sys/bus/platform/drivers/demo/unbind
echo 48000000.demo | sudo tee /sys/bus/platform/drivers/demo/bind
```

Use to reproduce probe/remove without rebooting.

Risks:

- can disrupt active hardware
- can crash unsafe remove paths
- may leave userspace with stale handles
- not appropriate on production systems

## Probe Debugging Decision Tree

```text
device missing?
  check runtime firmware/DT/bus

device exists, no driver?
  check compatible/modalias/MODULE_DEVICE_TABLE/module install

driver bound, probe failed?
  check first error and resource/provider

probe deferred forever?
  check devices_deferred and provider drivers

probe succeeded, user node missing?
  check subsystem registration and udev

runtime operation fails after probe?
  move to runtime I/O, IRQ, or ABI debugging
```

## Common Failure Modes

| Symptom | Likely Cause | First Checks |
| --- | --- | --- |
| probe never logs | no device or no match | sysfs device and modalias |
| `-EPROBE_DEFER` forever | provider missing/disabled | `devices_deferred` |
| `platform_get_irq()` fails | wrong interrupt property | runtime DT and irq provider |
| regulator missing | supply name mismatch | DT binding and regulator summary |
| chip ID wrong | power/reset/bus/address issue | hardware sequence |
| `/dev` node missing | subsystem or udev issue | probe success and class state |
| manual bind works but autoload fails | missing module alias or depmod | `modinfo`, `modules.alias` |

## Practice Exercises

### Exercise 1: Device To Driver Trace

For one Device Tree node, trace:

```text
runtime DT node
sysfs device
modalias
driver match table
module alias
driver symlink
probe log
```

### Exercise 2: Deferred Probe

Create or inspect a deferred probe case. Identify the missing provider and the config or Device Tree change needed to resolve it.

### Exercise 3: Resource Error Logs

Convert three probe resource failures to `dev_err_probe()` and verify the resulting logs preserve the resource name and error code.

## Debugging Checklist

- Check runtime Device Tree.
- Check driver registration logs.
- Check modalias and module autoloading.
- Check clocks, resets, regulators, GPIOs, and IRQs.
- Preserve exact probe error codes.
- Inspect `devices_deferred`.
- Confirm final `.config` enables providers.
- Check subsystem registration after hardware init.
- Use manual bind/unbind only in lab conditions.

## Related Topics

- [Driver Binding, Probe, And Remove](../fundamentals/driver-binding-probe-remove.md)
- [Device Tree Matching From Drivers](../fundamentals/device-tree-matching.md)
- [Resource Lookup And Managed Allocation](../fundamentals/resource-lookup-and-devm.md)
- [Debugfs And Sysfs Inspection](debugfs-and-sysfs-inspection.md)
- [Dynamic Debug](dynamic-debug.md)

## Official References

- [Driver binding](https://docs.kernel.org/driver-api/driver-model/binding.html)
- [Dynamic debug](https://docs.kernel.org/admin-guide/dynamic-debug-howto.html)
- [Devres managed device resources](https://docs.kernel.org/driver-api/driver-model/devres.html)

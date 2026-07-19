---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# Driver Matching And Probe Diagnosis Lab

## Goal

Trace a fictional DT node through compatible selection, platform-device creation, module alias generation, driver matching, match data, and probe outcome. Deliberate failures isolate each stage.

Use a disposable development kernel, virtual machine, or test board. Do not apply experimental overlays or force driver binding on production hardware.

## Lab Node

Add this node to a test platform's root or an OF-populated simple bus using the platform's normal DTS/overlay workflow:

```dts
match-lab {
        compatible = "example,match-lab-v2", "example,match-lab";
        example,poll-interval-ms = <250>;
        status = "okay";
};
```

The compatible and property are fictional and intentionally have no upstream schema. This node represents no physical hardware; the driver will only read metadata and log its selected variant.

For an overlay-capable system, ensure the platform supports creating platform devices for newly attached nodes. Otherwise compile the node into a test DTB and reboot the disposable target.

## Lab Driver

Create `match_lab.c` outside the knowledge-base repository:

```c
#include <linux/device.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/property.h>

struct match_lab_data {
        const char *name;
        u32 default_interval_ms;
};

static const struct match_lab_data v1_data = {
        .name = "v1-compatible path",
        .default_interval_ms = 1000,
};

static const struct match_lab_data v2_data = {
        .name = "v2-specific path",
        .default_interval_ms = 500,
};

static const struct of_device_id match_lab_of_match[] = {
        { .compatible = "example,match-lab-v2", .data = &v2_data },
        { .compatible = "example,match-lab", .data = &v1_data },
        { }
};
MODULE_DEVICE_TABLE(of, match_lab_of_match);

static int match_lab_probe(struct platform_device *pdev)
{
        const struct match_lab_data *data;
        u32 interval;
        int ret;

        data = device_get_match_data(&pdev->dev);
        if (!data)
                return dev_err_probe(&pdev->dev, -EINVAL,
                                     "no variant data\n");

        if (!device_property_present(&pdev->dev,
                                     "example,poll-interval-ms")) {
                interval = data->default_interval_ms;
        } else {
                ret = device_property_read_u32(&pdev->dev,
                                               "example,poll-interval-ms",
                                               &interval);
                if (ret)
                        return dev_err_probe(&pdev->dev, ret,
                                             "invalid poll interval\n");
        }

        dev_info(&pdev->dev, "matched %s, interval=%u ms\n",
                 data->name, interval);
        return 0;
}

static struct platform_driver match_lab_driver = {
        .probe = match_lab_probe,
        .driver = {
                .name = "match-lab",
                .of_match_table = match_lab_of_match,
        },
};
module_platform_driver(match_lab_driver);

MODULE_DESCRIPTION("Device Tree matching lab");
MODULE_LICENSE("GPL");
```

Create a `Makefile`:

```make
obj-m := match_lab.o
```

Build against the running development kernel:

```sh
make -C /lib/modules/$(uname -r)/build M=$PWD modules
modinfo ./match_lab.ko
modinfo -F alias ./match_lab.ko
```

The build needs matching kernel headers/configuration and a configured external-module toolchain. Record the kernel release, compiler, command, warnings, and module hash.

## Step 1: Prove The Runtime Node

```sh
find /sys/firmware/devicetree/base -name match-lab -print
tr '\0' '\n' </sys/firmware/devicetree/base/match-lab/compatible
hexdump -Cv /sys/firmware/devicetree/base/match-lab/example,poll-interval-ms
```

If the node is below a bus, adjust the path. Confirm the ordered compatible list in the runtime tree, not only the DTS source.

## Step 2: Prove Device Population

Find a platform device whose `of_node` resolves to the lab node:

```sh
find /sys/bus/platform/devices -maxdepth 2 -type l -name of_node -print
```

For the matching device directory, record:

```sh
readlink -f /sys/bus/platform/devices/DEVICE/of_node
cat /sys/bus/platform/devices/DEVICE/modalias
cat /sys/bus/platform/devices/DEVICE/uevent
```

If the runtime node exists but no platform device does, stop debugging the match table and fix population first.

## Step 3: Prove Module Metadata

Compare the device modalias with:

```sh
modinfo -F alias ./match_lab.ko
```

For an installed test module, run `depmod`, then query without loading:

```sh
modprobe --resolve-alias "$(cat /sys/bus/platform/devices/DEVICE/modalias)"
```

Record whether the intended module is the only result.

## Step 4: Load, Match, And Probe

Loading a module changes kernel state. On the disposable target:

```sh
sudo insmod ./match_lab.ko
dmesg | tail -n 30
readlink /sys/bus/platform/devices/DEVICE/driver
```

Expected log:

```text
matched v2-specific path, interval=250 ms
```

The `driver` symlink proves binding. The message proves probe selected V2 match data and decoded the property. It does not prove any real hardware function because the lab intentionally controls none.

## Step 5: Exercise The Fallback

Remove only the V2 entry from the driver's table, rebuild, reload safely, and keep the DT list unchanged:

```c
static const struct of_device_id match_lab_of_match[] = {
        { .compatible = "example,match-lab", .data = &v1_data },
        { }
};
```

Expected result: the node's second compatible matches and the log reports `v1-compatible path`. Explain why this demonstrates selection mechanics but does **not** prove a real hardware fallback is safe.

Restore the V2 entry.

## Step 6: Break Automatic Discovery

Remove only:

```c
MODULE_DEVICE_TABLE(of, match_lab_of_match);
```

Rebuild and compare `modinfo -F alias`. The driver can still match after `insmod` because `.of_match_table` remains, but a device modalias no longer resolves automatically to this module. Restore the macro.

## Step 7: Break Bus Matching

Keep the module table but remove:

```c
.of_match_table = match_lab_of_match,
```

Rebuild and load. The module registers a platform driver, but normal OF compatible matching should not bind it. This separates module loading from driver matching. Restore the pointer.

## Step 8: Break The Compatible

Change only the DT node to:

```dts
compatible = "example,match-lab-v3";
```

Apply through the safe test workflow and verify:

- node exists
- platform device exists
- its modalias changed
- the driver does not bind
- probe log is absent

Do not “fix” this by adding a wildcard; add a real V3 contract only if hardware semantics justify it. Restore the original list.

## Step 9: Disable Population

Set:

```dts
status = "disabled";
```

After rebuilding/rebooting or applying the test overlay lifecycle correctly, prove that the node can remain visible in the runtime tree while the platform device is absent. This is an availability/population result, not a match-table result.

Restore `okay`.

## Step 10: Separate Probe Failure

Temporarily make the property mandatory in the driver:

```c
ret = device_property_read_u32(&pdev->dev,
                               "example,poll-interval-ms",
                               &interval);
if (ret)
        return dev_err_probe(&pdev->dev, ret,
                             "poll interval is required\n");
```

Remove the property from DT. The device and candidate driver still match, so probe runs, returns an error, and no lasting `driver` symlink is created. Capture the log and errno. This is distinct from Step 8, where probe never ran.

Restore the optional-property semantics.

## Step 11: Build An Evidence Matrix

Fill one row per experiment:

| Experiment | Runtime node | Linux device | Alias resolves | Module loaded | Probe entered | Bound | Selected data/error |
|---|---|---|---|---|---|---|---|
| baseline | | | | | | | |
| fallback only | | | | | | | |
| no module table | | | | | | | |
| no OF table pointer | | | | | | | |
| incompatible V3 | | | | | | | |
| disabled | | | | | | | |
| required property absent | | | | | | | |

The matrix is the main deliverable. Every failure should stop at a different pipeline stage.

## Cleanup

Unload the test module only after confirming no dependent activity:

```sh
sudo rmmod match_lab
```

Remove the test overlay or restore the original DTB using the platform's supported lifecycle, then verify the lab device disappeared. Do not force removal if the platform cannot safely detach overlay-created devices.

## Completion Criteria

The lab is complete when you can provide:

- runtime compatible list and DT path
- Linux device path and resolved `of_node`
- device modalias and module alias metadata
- module build identity and hash
- match-data selection for specific and fallback cases
- distinct evidence for no population, no autoload, no match, and failed probe
- a completed pipeline matrix
- safe cleanup evidence

## Authoritative References

- [Linux driver-core binding model](https://docs.kernel.org/driver-api/driver-model/binding.html)
- [Linux DeviceTree matching and modalias APIs](https://docs.kernel.org/devicetree/kernel-api.html)
- [Linux external-module build guide](https://docs.kernel.org/kbuild/modules.html)
- [Linux platform devices and drivers](https://docs.kernel.org/driver-api/driver-model/platform.html)

## Continue

Proceed to [Pinctrl, GPIOs, And Interrupts](../pinctrl-gpios-and-interrupts.md).

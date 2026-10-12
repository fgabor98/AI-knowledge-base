---
status: draft
reviewed: false
domain: device-tree
difficulty: intermediate
last_reviewed: null
---

# Driver Matching And Probe Diagnosis Lab

## Goal

Trace a real DT node—the TMP100 temperature sensor at I2C address `0x48` on the TI AM642 SK—through device creation, module alias generation, autoloading, driver matching, match data, and probe outcome. Deliberate, non-persistent DT edits in U-Boot isolate each stage.

## Target And Safety

- an AM642 SK running a kernel built with `CONFIG_SENSORS_LM75=m` (the arm64 `defconfig` setting) and the matching `k3-am642-sk.dtb`
- serial console access to U-Boot
- edits are applied only to U-Boot's in-memory working FDT, so a normal reboot restores the original tree

Only touch `tmp100@48`. Do not disable `main_i2c1`: the same bus carries the PCA9538 expander that switches the SD card supply, and the board may lose its root filesystem.

## The Node And Driver

From `k3-am642-sk.dts`:

```dts
&main_i2c1 {
        /* SoC power supply temperature */
        tmp100@48 {
                compatible = "ti,tmp100";
                reg = <0x48>;
        };
};
```

From `drivers/hwmon/lm75.c`, trimmed:

```c
static const struct i2c_device_id lm75_i2c_ids[] = {
        /* ... */
        { "tmp100", tmp100, },
        /* ... */
};
MODULE_DEVICE_TABLE(i2c, lm75_i2c_ids);

static const struct of_device_id __maybe_unused lm75_of_match[] = {
        /* ... */
        {
                .compatible = "ti,tmp100",
                .data = (void *)tmp100
        },
        /* ... */
};
MODULE_DEVICE_TABLE(of, lm75_of_match);
```

The driver has two ID tables. Both carry the same variant enum, which `i2c_get_match_data()` retrieves in probe.

## Step 1: Prove The Runtime Node

```sh
N=/sys/firmware/devicetree/base/bus@f4000/i2c@20010000/tmp100@48
tr '\0' '\n' <"$N/compatible"
hexdump -C "$N/reg"
```

Confirm the compatible and address in the runtime tree, not only in the DTS.

## Step 2: Prove Device Creation

The I2C core, not platform population, creates this device after `main_i2c1` probes:

```sh
ls -l /sys/bus/i2c/devices/
readlink -f /sys/bus/i2c/devices/1-0048/of_node
cat /sys/bus/i2c/devices/1-0048/name
cat /sys/bus/i2c/devices/1-0048/modalias
```

Expected: the client `1-0048` (adapter 1 because of the `i2c1` alias), `name` = `tmp100`, and modalias `of:Ntmp100T(null)Cti,tmp100`. The client name comes from the text after the comma in the first compatible (`of_alias_from_compatible()` in `drivers/of/base.c`).

## Step 3: Prove Module Metadata

```sh
modinfo -F alias lm75 | grep -i tmp100
modprobe --resolve-alias "$(cat /sys/bus/i2c/devices/1-0048/modalias)"
```

Expect `of:N*T*Cti,tmp100`, `of:N*T*Cti,tmp100C*`, and `i2c:tmp100`, and the alias resolves to `lm75`.

## Step 4: Prove Binding And Function

```sh
readlink /sys/bus/i2c/devices/1-0048/driver
ls /sys/bus/i2c/devices/1-0048/hwmon/
cat /sys/bus/i2c/devices/1-0048/hwmon/hwmon*/name
cat /sys/bus/i2c/devices/1-0048/hwmon/hwmon*/temp1_input
```

The `driver` symlink proves binding. A plausible millidegree reading proves the I2C transaction and the TMP100 register model work. Repeat for `1-0049`, the DDR supply sensor.

## Step 5: Separate Module Loading From Binding

```sh
echo 1-0048 | sudo tee /sys/bus/i2c/drivers/lm75/unbind
readlink /sys/bus/i2c/devices/1-0048/driver || echo unbound
lsmod | grep lm75
echo 1-0048 | sudo tee /sys/bus/i2c/drivers/lm75/bind
```

The module stays loaded while one device is unbound, so “module loaded” and “device bound” are different facts. Then unload and let udev reload it:

```sh
sudo modprobe -r lm75
sudo udevadm trigger --action=add /sys/bus/i2c/devices/1-0048
readlink /sys/bus/i2c/devices/1-0048/driver
```

## Step 6: Wrong Vendor Prefix

Reboot, interrupt U-Boot, load the kernel and DTB the way your boot flow does, then before `booti`:

```text
=> fdt addr ${fdt_addr_r}
=> fdt resize 0x1000
=> fdt set /bus@f4000/i2c@20010000/tmp100@48 compatible "acme,tmp100"
```

`acme,tmp100` is a deliberately wrong, invented string. After boot:

```sh
cat /sys/bus/i2c/devices/1-0048/name
cat /sys/bus/i2c/devices/1-0048/modalias
readlink /sys/bus/i2c/devices/1-0048/driver || echo unbound
sudo modprobe lm75
readlink /sys/bus/i2c/devices/1-0048/driver
```

Expected: the client name is still `tmp100`, the modalias is `of:Ntmp100T(null)Cacme,tmp100`, which no module alias matches, so nothing autoloads. But once `lm75` is loaded manually, the device **binds** through the `i2c_device_id` table: `i2c_device_match()` in `drivers/i2c/i2c-core-base.c` tries OF matching, then ACPI, then the I2C ID table against the client name. Explain why “works after `modprobe`” here hides a DT bug rather than a packaging bug.

## Step 7: Unknown Part

Repeat Step 6 with the invented compatible `"ti,tmp99"`. Now neither the OF table nor the I2C ID table (`tmp99`) matches, so the device stays unbound even with the module loaded. This is a match failure, not an autoload failure.

## Step 8: Disable The Device

Repeat with:

```text
=> fdt set /bus@f4000/i2c@20010000/tmp100@48 status disabled
```

The runtime node still exists under `/sys/firmware/devicetree/base`, but `1-0048` does not exist. This is an availability/population result; matching never runs.

## Step 9: Separate Probe Failure

Repeat with a wrong address:

```text
=> fdt set /bus@f4000/i2c@20010000/tmp100@48 reg <0x4a>
```

Unit address and `reg` now disagree, and nothing answers at `0x4a`. Expect a client `1-004a`, a match against `lm75`, a probe that fails when the driver first reads the chip, and no lasting `driver` symlink:

```sh
ls /sys/bus/i2c/devices/ | grep 004a
readlink /sys/bus/i2c/devices/1-004a/driver || echo unbound
dmesg | grep -i -E 'lm75|1-004a'
```

Record the errno. This is distinct from Step 7, where probe never ran.

## Step 10: Build An Evidence Matrix

| Experiment | Runtime node | I2C client | Modalias resolves | Module loaded | Probe entered | Bound | Notes |
|---|---|---|---|---|---|---|---|
| baseline | | | | | | | |
| unbind/bind | | | | | | | |
| wrong vendor prefix | | | | | | | |
| unknown part | | | | | | | |
| disabled | | | | | | | |
| wrong address | | | | | | | |

Every failure should stop at a different pipeline stage.

## Cleanup

Reboot without U-Boot edits and confirm `1-0048` and `1-0049` are bound to `lm75` again.

## Completion Criteria

The lab is complete when you can provide:

- runtime compatible list and DT path
- I2C client path, name, and resolved `of_node`
- device modalias and module alias metadata
- evidence that the I2C ID table can bind a mis-prefixed node without autoloading it
- distinct evidence for no population, no autoload, no match, and failed probe
- a completed pipeline matrix

## Authoritative References

- [Linux driver-core binding model](https://docs.kernel.org/driver-api/driver-model/binding.html)
- [Linux I2C device instantiation](https://docs.kernel.org/i2c/instantiating-devices.html)
- [Linux DeviceTree matching and modalias APIs](https://docs.kernel.org/devicetree/kernel-api.html)
- [U-Boot `fdt` command](https://docs.u-boot.org/en/latest/usage/cmd/fdt.html)

## Continue

Proceed to [Runtime Inspection](../runtime-inspection.md).

---
status: draft
reviewed: false
domain: device-tree
difficulty: intermediate
last_reviewed: null
---

# Matching, Modaliases, Modules, And Bound Drivers

Once a Linux device exists, its bus attempts to match registered drivers. For a DT-backed device, the ordered compatible list contributes to OF matching and modalias generation. Module autoloading, driver registration, probe, and successful binding remain separate stages.

## Inspect The Device

```bash
dev=/sys/bus/platform/devices/48000000.device

cat "$dev/modalias"
cat "$dev/uevent"
readlink -f "$dev/of_node"
readlink -f "$dev/driver" 2>/dev/null || true
```

Interpretation:

- `modalias`: match/autoload key exported for this device
- `uevent`: environment used by userspace device management, often including `MODALIAS`
- `of_node`: DT origin
- `driver`: appears after successful binding/probe

The exact device path and alias format are bus-specific.

## Compare Ordered Compatible Data

```bash
tr '\0' '\n' <"$(readlink -f "$dev/of_node")/compatible"
```

Compare this runtime list with:

- driver `of_match_table`
- module alias metadata
- binding-compatible fallback rules

A driver can match a later fallback string. Do not inspect only the first compatible.

## Resolve Module Alias

```bash
alias_value=$(cat "$dev/modalias")
modprobe --resolve-alias "$alias_value"
```

This asks installed kmod metadata which module names satisfy the alias. Outcomes:

- one or more modules: autoload metadata exists
- none: driver may be built-in, module absent, alias metadata missing, or no supported match

Check an installed module:

```bash
modinfo MODULE
modinfo -F alias MODULE
```

`MODULE_DEVICE_TABLE(of, ...)` commonly causes OF aliases to be exported for modular drivers. It does not control the driver's in-memory match table itself.

## Built-In Versus Module

`lsmod` shows loaded modules only. A built-in driver will never appear there.

Evidence sources:

- running kernel configuration, when reliably available
- driver directory under `/sys/bus/BUS/drivers`
- bound device's `driver` symlink
- kernel symbol/build metadata in a development environment
- boot logs

Do not “fix” a built-in driver by trying to `modprobe` its source name.

## Registered Driver

```bash
ls -l /sys/bus/platform/drivers
ls -l /sys/bus/platform/drivers/DRIVER
```

A driver directory proves registration on that bus. It may contain symlinks to bound devices and control files such as `bind`/`unbind`, depending on the bus/driver.

Driver name, module name, and compatible string can all differ. Establish each explicitly.

## Bound Driver

```bash
driver_path=$(readlink -f "$dev/driver")
printf '%s\n' "$driver_path"
basename "$driver_path"
```

A valid `driver` symlink means the driver core completed binding after probe success. It does not prove:

- every optional hardware feature works
- interrupts fire
- DMA is correct
- the external device is electrically present
- suspend/resume or recovery works
- userspace configured the interface

Continue to subsystem evidence.

## Capture Probe Logs

Prefer the complete boot log with monotonic timestamps:

```bash
journalctl -k -b 0 -o short-monotonic > kernel-boot.log
```

On systems without persistent journal access:

```bash
dmesg --time-format=rel > kernel-boot.log
```

Then filter the captured copy:

```bash
rg -i '48000000|acme|probe|defer|clock|regulator|reset|irq' kernel-boot.log
```

Kernel ring buffers can wrap, rate-limit, or omit debug messages. A missing log line is not proof that probe never ran.

## Distinguish States

| Device exists | driver link | likely state |
|---:|---:|---|
| no | n/a | population/parent/availability issue |
| yes | no | no match, driver unavailable, probe failed, or probe deferred |
| yes | yes | probe succeeded and device is bound |

Use modalias, registered drivers, deferred list, and logs to split the middle row.

## Autoload Versus Manual Loading

If manual `modprobe MODULE` creates a driver directory and binding occurs, but automatic loading did not:

- compare device modalias with `modinfo -F alias`
- inspect installed `modules.alias`
- run the correct `depmod` for the installed kernel
- confirm userspace uevent handling is running
- confirm module file matches `uname -r`

If module loads but device remains unbound, move to matching/probe—not packaging.

## `driver_override`

Some buses expose:

```bash
cat "$dev/driver_override"
```

Writing a driver name can restrict matching to that driver and bypass normal bus-specific matching when the bus supports it. Upstream driver-core documentation states that writing the override does not automatically unbind the current driver or load/bind the requested driver.

Use only in a controlled lab. An override can force consideration of a driver that the DT compatible would not normally match; it does not make incompatible hardware safe.

Record and clear any override after the experiment according to the bus ABI.

## Functional Endpoint

After binding, locate subsystem state:

- `/sys/class/net` and `ip link`
- `/sys/class/hwmon`
- `/sys/bus/iio/devices`
- `/proc/bus/input/devices`
- DRM/media/sound subsystem tools
- MTD/block device inventories
- remoteproc/RPMsg sysfs

The relevant subsystem documentation defines success. Do not use creation of a character device or interface name as the only hardware test.

## Match Record

```text
device path and bus:
resolved of_node and compatible list:
device modalias:
resolved module aliases:
module built-in/installed/loaded state:
registered driver path/name:
driver symlink:
probe/defer/failure log:
subsystem interface and functional result:
```

## Authoritative References

- [Linux driver-core binding model](https://docs.kernel.org/driver-api/driver-model/binding.html)
- [Linux driver model: drivers](https://docs.kernel.org/driver-api/driver-model/driver.html)
- [Linux Devicetree Kernel API](https://docs.kernel.org/devicetree/kernel-api.html)
- [Linux kmod documentation](https://www.kernel.org/pub/linux/utils/kernel/kmod/)

## Continue

Proceed to [Probe Deferral, Supplier Links, And Resource State](probe-deferral-supplier-links-and-resource-state.md).

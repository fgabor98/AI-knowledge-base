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

## Resolve Module Alias And Built-In State

```bash
alias_value=$(cat "$dev/modalias")
modprobe --resolve-alias "$alias_value"
modinfo -F alias MODULE
```

No result means the driver may be built in, the module absent, alias metadata missing, or no supported match exists; [Modaliases, Module Metadata, And Autoloading](../driver-matching/modaliases-module-metadata-and-autoloading.md) explains each case. `lsmod` never shows built-in drivers, so confirm them through `/sys/bus/BUS/drivers`, the `driver` symlink, kernel configuration, or boot logs instead of trying to `modprobe` them.

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

A valid `driver` symlink means the driver core completed binding after probe success; it is [not functional proof](../driver-matching/binding-driven-probe-contracts.md#successful-probe-is-not-functional-proof). Continue to subsystem evidence.

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

If manual `modprobe MODULE` binds the device but automatic loading did not, the fault is in the autoload path ([Packaging And Policy Failures](../driver-matching/modaliases-module-metadata-and-autoloading.md#packaging-and-policy-failures)): compare modalias with module aliases, rerun `depmod` for the installed kernel, and confirm uevent handling. If the module loads but the device remains unbound, move to matching/probe—not packaging. Forced matching through `driver_override` is covered in [Controlled Bind/Unbind And Runtime Forensics](controlled-bind-unbind-and-runtime-forensics.md#driver_override-experiment).

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

---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# Debugfs And Sysfs Inspection

## What Problem Does This Solve?

Runtime filesystems expose kernel and device state for inspection and controlled diagnostics.

They often answer questions before code changes are needed:

- does the device object exist?
- did a driver bind?
- which module owns this device?
- did the IRQ count increase?
- is a regulator enabled?
- is a clock prepared?
- does a subsystem expose diagnostic state?
- is userspace looking at the right device node?

## Core Concepts

- sysfs
- debugfs
- configfs overview
- device hierarchy
- driver bind and unbind
- subsystem debug files
- ABI stability
- production exposure

## Mental Model

Sysfs is a stable-ish user ABI surface when documented. Debugfs is diagnostic and should not be required for normal product operation.

```text
sysfs:
  kernel object model and documented ABI

debugfs:
  diagnostics and internal state
  not stable ABI

procfs:
  process and selected kernel state

configfs:
  userspace-created kernel object configuration
```

Use runtime filesystems to inspect state, not to guess.

## Mount Checks

```sh
mount | grep -E 'sysfs|debugfs|tracefs|configfs'
```

Common mount points:

```text
/sys
/sys/kernel/debug
/sys/kernel/tracing
/sys/kernel/config
/proc
```

Mount debugfs in a lab:

```sh
sudo mount -t debugfs none /sys/kernel/debug
```

Do not assume debugfs is available or mounted in production.

## Device Existence

Platform devices:

```sh
find /sys/bus/platform/devices -maxdepth 1 -print
find /sys/bus/platform/devices -maxdepth 1 -name '*demo*' -print
```

I2C devices:

```sh
find /sys/bus/i2c/devices -maxdepth 2 -print
```

SPI devices:

```sh
find /sys/bus/spi/devices -maxdepth 2 -print
```

If the device does not exist, probe cannot run. Debug Device Tree, ACPI, bus enumeration, or board setup first.

## Driver Binding State

Check whether a device is bound:

```sh
readlink /sys/bus/platform/devices/48000000.demo/driver
```

Check modalias:

```sh
cat /sys/bus/platform/devices/48000000.demo/modalias
```

Check module aliases:

```sh
modinfo demo.ko | grep alias
```

List devices bound to a driver:

```sh
find /sys/bus/platform/drivers/demo -maxdepth 1 -type l -print
```

Manual bind/unbind for lab diagnosis:

```sh
echo 48000000.demo | sudo tee /sys/bus/platform/drivers/demo/unbind
echo 48000000.demo | sudo tee /sys/bus/platform/drivers/demo/bind
```

Use this carefully. It exercises remove/probe paths and can disrupt hardware.

## Module State

```sh
lsmod | grep demo
modinfo demo
cat /sys/module/demo/refcnt
find /sys/module/demo -maxdepth 2 -type f | sort
```

Parameters:

```sh
find /sys/module/demo/parameters -type f -maxdepth 1 -print
cat /sys/module/demo/parameters/debug
```

If `rmmod` says the module is in use, check open file descriptors, references, and bound devices.

## Device Tree Runtime Inspection

Runtime Device Tree:

```sh
find /proc/device-tree -maxdepth 4 -name compatible -print
```

Read null-separated string properties:

```sh
tr '\0' '\n' < /proc/device-tree/path/to/node/compatible
tr '\0' '\n' < /proc/device-tree/path/to/node/status
```

Do not assume the DTB you edited is the DTB that booted. Check runtime state.

## Character Device And Udev State

```sh
ls -l /dev/demo*
cat /proc/devices | grep demo
udevadm info /dev/demo0
find /sys/class -maxdepth 3 -name '*demo*' -print
```

If `/dev` is missing:

1. confirm probe succeeded
2. confirm class/device registration succeeded
3. confirm uevent was emitted
4. check udev or static device-node policy

Do not debug udev before checking kernel registration.

## IRQ State

```sh
cat /proc/interrupts | grep -i demo
cat /proc/irq/42/smp_affinity
```

Use this to distinguish:

```text
IRQ never fires
IRQ fires but handler returns wrong status
IRQ storm
wrong CPU affinity assumption
```

For ordering details, use tracepoints.

## Useful Subsystem Debugfs Files

Availability depends on config and mounted debugfs.

Common examples:

```sh
cat /sys/kernel/debug/gpio
cat /sys/kernel/debug/clk/clk_summary
cat /sys/kernel/debug/regulator/regulator_summary
cat /sys/kernel/debug/pinctrl/*/pins
cat /sys/kernel/debug/pinctrl/*/pinmux-pins
find /sys/kernel/debug/regmap -maxdepth 2 -type f -print
cat /sys/kernel/debug/devices_deferred
cat /sys/kernel/debug/wakeup_sources
```

Use these to answer provider-state questions:

- is the GPIO requested?
- is the clock enabled?
- is the regulator on?
- did a device defer probe?
- is a wake source active?
- do regmap registers look sane?

## Sysfs ABI Versus Debugfs Diagnostics

Sysfs files used by product software should be documented ABI, often under `Documentation/ABI/` in the kernel tree.

Debugfs files:

- can change without compatibility guarantees
- may expose internal details
- may be absent in production
- may require elevated permissions

Do not build product behavior around debugfs.

## Configfs Overview

Configfs lets userspace create and configure kernel objects in some subsystems.

Examples include:

- USB gadget configuration
- some target/storage configurations
- subsystem-specific object creation

Configfs is not just inspection; writes create kernel objects. Treat it as a configuration interface with product policy and testing.

## Permissions And Security Policy

If a file exists but access fails, check:

```sh
ls -l path
id
cat /proc/self/status | grep Cap
cat /sys/kernel/security/lsm 2>/dev/null
dmesg | grep -i denied
```

Access may be restricted by:

- UNIX permissions
- capabilities
- LSM policy
- cgroup device policy
- namespace/container boundaries
- product mount policy

## Evidence Checklist

For a driver issue, capture:

```text
device sysfs path
driver symlink
modalias
module info
runtime Device Tree node
relevant /proc/interrupts line
relevant provider debugfs summaries
/dev node and udev info if applicable
```

This often proves whether the failure is enumeration, binding, probe, runtime, or userspace policy.

## Common Failure Modes

| Symptom | Likely Cause | First Checks |
| --- | --- | --- |
| no sysfs device | DT/bus enumeration problem | `/proc/device-tree`, bus devices |
| device exists but no driver | match/autoload failure | modalias and module aliases |
| `/dev` node missing | class/device registration or udev issue | sysfs class and dmesg |
| debugfs file missing | config or mount policy | debugfs mount and config |
| access denied | permissions/LSM/cgroups | denial logs |
| product script breaks after kernel update | script used debugfs as ABI | replace with supported ABI |

## Practice Exercises

### Exercise 1: Device Path Map

For one device, record:

```text
bus path
driver symlink
modalias
module
/dev node if any
runtime DT node
IRQ line
```

### Exercise 2: Provider State Inspection

Use debugfs to check one provider dependency such as regulator, clock, GPIO, pinctrl, or regmap.

### Exercise 3: ABI Classification

Classify every runtime file used by your test scripts as:

```text
stable/documented ABI
debug-only
lab-only
unsafe for product
```

## Debugging Checklist

- Confirm filesystems are mounted.
- Distinguish stable ABI from debug-only state.
- Avoid scripting product behavior around debugfs.
- Check permissions and security policy.
- Check device existence before checking probe.
- Check driver binding before checking runtime callbacks.
- Check provider debugfs before guessing resource failure causes.
- Capture sysfs paths in bug reports.

## Related Topics

- [Sysfs Attributes](../fundamentals/sysfs-attributes.md)
- [Dynamic Debug](dynamic-debug.md)
- [Module Signing And Hardening](../configuration-and-platform-policy/module-signing-and-hardening.md)
- [Probe Failure Debugging](probe-failure-debugging.md)

## Official References

- [Debugfs](https://docs.kernel.org/filesystems/debugfs.html)
- [ABI testing sysfs](https://docs.kernel.org/admin-guide/abi-testing.html)
- [Driver binding](https://docs.kernel.org/driver-api/driver-model/binding.html)

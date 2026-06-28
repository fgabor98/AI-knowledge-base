---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# Built-In Drivers Vs Loadable Modules

## What Problem Does This Solve?

Drivers can be linked into the kernel image or shipped as loadable modules. The choice affects boot order, update strategy, recovery, dependency handling, firmware availability, product security, and debugging workflow.

This is not just a build preference. It changes when the driver can exist:

```text
built-in driver
  available as soon as its initcall runs during kernel boot

loadable module
  available only after module storage, module loader, dependencies, and policy allow it
```

## Core Concepts

- `CONFIG_FOO=y`
- `CONFIG_FOO=m`
- built-in object
- loadable `.ko`
- initcall ordering
- module init
- module exit
- module autoloading
- modalias
- initramfs module loading
- firmware availability
- root filesystem dependencies
- `MODULE_DEVICE_TABLE()`
- module signing
- Secure Boot and lockdown policy

## Mental Model

Build a driver in when the system needs it before userspace can load modules. Build it as a module when optional hardware, iterative development, late loading, diagnostics, package updates, or removable device support matter more.

```text
needed to mount rootfs?
  -> usually built in or loaded from initramfs

optional device after boot?
  -> module is often fine

security policy forbids unsigned modules?
  -> built-in or signed module

rapid driver iteration?
  -> module if hardware and lifetime allow it
```

## How Kconfig Expresses The Choice

Kconfig value:

```text
CONFIG_EXAMPLE_DRIVER=y
```

means the driver is linked into the kernel image.

Kconfig value:

```text
CONFIG_EXAMPLE_DRIVER=m
```

means Kbuild produces a loadable module.

Unset:

```text
# CONFIG_EXAMPLE_DRIVER is not set
```

means the driver is not built.

Kbuild often connects this directly:

```make
obj-$(CONFIG_EXAMPLE_DRIVER) += example_driver.o
```

The same source file can become built-in, modular, or absent depending on the final `.config`.

## Built-In Driver Behavior

A built-in driver:

- is part of the kernel image
- cannot be unloaded with `rmmod`
- does not appear in `lsmod`
- may have its `__init` code freed after initialization
- is available before rootfs if its initcall runs early enough
- must be updated by updating the kernel image
- may still expose parameters under `/sys/module/<name>/parameters`

Check whether a driver is built in:

```sh
grep '^CONFIG_EXAMPLE_DRIVER=y' /boot/config-$(uname -r)
zcat /proc/config.gz | grep '^CONFIG_EXAMPLE_DRIVER'
```

Not every system exposes `/proc/config.gz`; it depends on kernel config.

Runtime clues:

```sh
dmesg | grep -i example
find /sys/bus -name '*example*' 2>/dev/null
```

Do not expect:

```sh
lsmod | grep example
```

to show built-in drivers.

## Loadable Module Behavior

A module:

- is stored as a `.ko`
- is loaded by `insmod`, `modprobe`, udev, systemd, initramfs scripts, or another policy agent
- can often be unloaded when unused
- appears in `lsmod`
- has metadata visible with `modinfo`
- must match the running kernel sufficiently
- may depend on other modules
- may be subject to signing policy

Check:

```sh
modinfo example_driver.ko
modinfo example_driver.ko | grep vermagic
lsmod | grep example
```

Load:

```sh
sudo modprobe example_driver
```

Unload:

```sh
sudo modprobe -r example_driver
```

## Boot-Critical Drivers

Build in drivers needed before the module loader and rootfs are available.

Common examples:

- boot storage controller
- root filesystem driver
- block layer features needed for rootfs
- initramfs support if using initramfs
- early console or serial console in some systems
- SoC pinctrl and clock providers needed by early devices
- regulators needed before storage or bus drivers can run

Example for rootfs on eMMC with ext4:

```text
CONFIG_MMC=y
CONFIG_MMC_SDHCI=y
CONFIG_MMC_SDHCI_PLTFM=y
CONFIG_EXT4_FS=y
```

Risky without initramfs:

```text
CONFIG_MMC=m
CONFIG_EXT4_FS=m
```

Typical boot failure:

```text
VFS: Cannot open root device
Kernel panic - not syncing: VFS: Unable to mount root fs
```

That is usually a configuration/boot sequencing issue, not a bug in the storage driver source.

## Initramfs As A Middle Ground

An initramfs can load modules before the real root filesystem is mounted.

Use this when:

- storage support is modular by policy
- disk encryption requires early userspace
- rootfs discovery needs scripts
- hardware support varies across machines
- distro initramfs tooling owns early boot

The module still must be present inside the initramfs and match the kernel:

```text
kernel image
-> built-in initramfs or bootloader-provided initramfs
-> early userspace loads modules
-> rootfs becomes accessible
```

If a module is required for rootfs but missing from initramfs, boot still fails.

## Firmware Timing

Some drivers need firmware files:

```c
request_firmware(&fw, "example/device.bin", dev);
```

For modules loaded after rootfs is mounted, firmware can often live under:

```text
/lib/firmware/
```

For built-in drivers that request firmware very early, firmware may need to be:

- built into the kernel
- included in initramfs
- loaded later after rootfs is available
- avoided during early probe

Failure symptom:

```text
firmware: failed to load example/device.bin
```

Check:

```sh
dmesg | grep -i firmware
find /lib/firmware -name '*example*'
```

## Module Autoloading

Modules can be loaded automatically when a matching device appears. The path depends on aliases generated from metadata such as:

```c
MODULE_DEVICE_TABLE(of, demo_of_match);
```

For a Device Tree platform driver, this helps produce module aliases that userspace can use for autoloading.

Inspect aliases:

```sh
modinfo demo.ko | grep alias
```

Example alias:

```text
of:N*T*Cexample,demo-device
```

If autoloading fails:

- ensure `MODULE_DEVICE_TABLE()` exists
- run `depmod` after installing modules
- inspect the device modalias in sysfs
- inspect udev events

Useful commands:

```sh
find /sys -name modalias -exec grep -H example {} \; 2>/dev/null
udevadm monitor --kernel --property
modprobe -v demo
```

## Initcall Ordering

Built-in drivers initialize through initcalls. Different code may run at different boot phases.

Commonly seen initcall levels include:

```text
early_initcall
core_initcall
postcore_initcall
arch_initcall
subsys_initcall
fs_initcall
device_initcall
late_initcall
```

Most normal drivers use helper macros that choose appropriate registration timing. For example:

```c
module_platform_driver(demo_driver);
```

works for both module and built-in cases. When built in, the generated init function becomes an initcall.

Avoid changing initcall levels unless you understand the dependency being solved. Many ordering problems should be handled through the device model, provider/consumer APIs, and probe deferral instead.

## Module Signing And Policy

Some products require signed modules:

```text
CONFIG_MODULE_SIG=y
CONFIG_MODULE_SIG_FORCE=y
```

With enforcement enabled, an unsigned module may fail to load even when it matches the kernel.

Symptoms:

```text
Required key not available
module verification failed
Lockdown: Loading of unsigned modules is restricted
```

Check:

```sh
dmesg | tail -n 80
grep '^CONFIG_MODULE_SIG' /boot/config-$(uname -r)
```

For secured products, built-in drivers reduce runtime module-loading surface, but they also require kernel image updates for changes.

## Development Tradeoffs

Modules are convenient during bring-up:

```sh
make -C /path/to/kernel/build M=$PWD modules
scp demo.ko target:/tmp/
ssh target 'insmod /tmp/demo.ko'
ssh target 'rmmod demo'
```

This loop is fast, but it only works when:

- the driver can be safely removed
- hardware can be returned to a clean state
- no userspace process holds it open
- no active interrupt/work/timer path remains
- module signing policy allows the module

For hardware that cannot be reset safely or drivers that are core to boot, test with built-in builds earlier.

## Decision Table

| Situation | Prefer |
| --- | --- |
| Rootfs storage controller | Built-in or initramfs-loaded |
| Filesystem needed for rootfs | Built-in or initramfs-loaded |
| Optional USB device | Module |
| Removable hardware | Module |
| Sensor used by product application after boot | Usually module or subsystem policy |
| Early console | Built-in |
| Security-sensitive appliance with no runtime module loading | Built-in or signed modules only |
| Rapid driver development | Module if lifecycle allows |
| Vendor field update only ships kernel image | Built-in may be simpler |
| Package-managed distro driver | Module is common |

## Common Failure Modes

| Symptom | Likely Cause | First Checks |
| --- | --- | --- |
| Driver not in `lsmod` | It is built in, not loaded, or not built | `.config`, `modinfo`, `dmesg` |
| Boot cannot mount rootfs | rootfs driver/filesystem modular and not in initramfs | `.config`, initramfs contents |
| `modprobe` cannot find module | missing install or `depmod` metadata | `/lib/modules/$(uname -r)` |
| Autoload does not happen | missing alias or udev/module policy issue | `modinfo alias`, sysfs `modalias` |
| Firmware load fails | firmware unavailable at probe time | `/lib/firmware`, initramfs, dmesg |
| `rmmod` fails busy | open files, active users, dependencies | `lsmod`, `fuser`, driver state |
| Unsigned module rejected | signing or lockdown policy | `dmesg`, config |

## Practice Exercises

### Exercise 1: Build The Same Driver Both Ways

Set:

```text
CONFIG_EXAMPLE_DRIVER=m
```

Build and inspect `.ko` output.

Then set:

```text
CONFIG_EXAMPLE_DRIVER=y
```

Build and confirm no `.ko` exists for that driver.

Questions:

- Where does runtime evidence appear in each case?
- Does `lsmod` show the built-in driver?
- What changes in deployment?

### Exercise 2: Inspect Module Aliases

For a module with an OF match table:

```sh
modinfo demo.ko | grep alias
find /sys -name modalias -exec grep -H demo {} \; 2>/dev/null
```

Questions:

- Does the module advertise the expected compatible string?
- Does the device expose a modalias?

### Exercise 3: Test Rootfs Dependency Reasoning

Review a board config:

```sh
grep -E 'CONFIG_(MMC|SCSI|ATA|NVME|EXT4|NFS|BLK_DEV_INITRD)' build/.config
```

Questions:

- Which storage and filesystem pieces are needed before rootfs?
- Are they built in, modular, or initramfs-loaded?

## Debugging Checklist

- Check the final `.config`, not only fragments.
- Confirm `CONFIG_FOO=y`, `CONFIG_FOO=m`, or unset.
- For modules, check `modinfo`, `vermagic`, aliases, and dependencies.
- For built-ins, check boot logs and sysfs, not `lsmod`.
- Confirm boot-critical dependencies are available before rootfs.
- Confirm firmware timing.
- Confirm module signing and lockdown policy.
- Check whether `MODULE_DEVICE_TABLE()` exists for autoloading.
- Check `depmod` output and `/lib/modules/$(uname -r)`.

## Related Topics

- [Kernel Configuration And Platform Policy](../configuration-and-platform-policy/index.md)
- [Built-In Vs Module Policy](../configuration-and-platform-policy/built-in-vs-module-policy.md)
- [Module Signing And Hardening](../configuration-and-platform-policy/module-signing-and-hardening.md)
- [Kconfig And Defconfig](../../build-systems/advanced/linux-kernel/kconfig-and-defconfig.md)
- [Kernel Module Lifecycle](kernel-module-lifecycle.md)
- [Kernel Build And Install Overview](../source-build-and-tailoring/kernel-build-and-install-overview.md)

## Official References

- [Building External Modules](https://docs.kernel.org/kbuild/modules.html)
- [The Kernel's Command-Line Parameters](https://docs.kernel.org/admin-guide/kernel-parameters.html)
- [Kernel Module Signing Facility](https://docs.kernel.org/admin-guide/module-signing.html)

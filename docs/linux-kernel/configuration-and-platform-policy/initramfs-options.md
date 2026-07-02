---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# Initramfs Options

## What Problem Does This Solve?

Initramfs configuration determines which early userspace files, modules, firmware, and recovery tools are available before the real root filesystem is mounted.

The initramfs is often the difference between a bootable product and a kernel that panics because it cannot find rootfs. It can also be the difference between a recoverable field device and a board that must be reflashed manually.

Initramfs policy decides:

- what is available before rootfs
- which modular drivers can participate in boot
- where firmware comes from
- how rootfs discovery works
- whether recovery and manufacturing flows exist
- how much diagnostic exposure is allowed early in boot

## Core Concepts

- built-in initramfs
- external initramfs
- early userspace
- module loading
- firmware loading
- recovery shell
- rootfs discovery
- manufacturing and rescue flows

## Mental Model

Initramfs is the bridge between kernel startup and product userspace. Keep its contents minimal but sufficient for boot, recovery, and diagnostics.

```text
kernel starts
-> optional initramfs unpacked
-> early userspace runs /init
-> load modules / firmware / find rootfs / recover
-> switch_root or equivalent to real rootfs
```

If a driver is needed before real rootfs and is not built in, the initramfs must contain and load it.

## Built-In Versus External Initramfs

Built-in initramfs:

- compiled into the kernel image
- travels with the kernel
- useful for tiny rescue systems or tightly controlled appliances
- increases kernel image size
- requires kernel rebuild to change contents

External initramfs:

- separate boot artifact loaded by bootloader
- easier to update independently
- common in distributions and many embedded products
- must be kept version-compatible with kernel modules

Policy question:

```text
Should recovery be inseparable from the kernel, or independently updateable?
```

## What Belongs In Initramfs?

Only include what is needed before real rootfs or for recovery.

Common contents:

- `/init`
- shell or minimal init tool
- storage modules
- filesystem modules
- bus/controller modules
- firmware needed before rootfs
- device nodes or devtmpfs mount logic
- rootfs discovery scripts
- cryptsetup or key handling, if used
- network tools for recovery, if required
- signed modules and dependency files

Avoid:

- full product application stack
- broad debug tools in production initramfs
- stale modules from a different kernel
- private keys or secrets
- unbounded rescue shell access in locked products

## Module And Firmware Requirements

If early userspace must load modules, include:

```text
modules under /lib/modules/$(uname -r)
modules.dep
modules.alias
modprobe or insmod
required firmware under /lib/firmware
module signing compatibility
```

Version mismatch is common:

```text
kernel release: 6.6.40-product
initramfs modules: 6.6.38-product
```

This can fail even if symbol names look similar.

## Rootfs Discovery

Root discovery may use:

```text
root=/dev/mmcblk0p2
root=PARTUUID=...
root=UUID=...
root=LABEL=...
rootwait
rootfstype=...
```

Initramfs scripts may also scan for devices or validate rootfs.

Prefer stable identifiers when device enumeration can change.

Questions:

- Is root storage driver built in or loaded from initramfs?
- Is filesystem support built in or loaded from initramfs?
- Is firmware needed before root appears?
- What happens if rootfs is corrupted?
- Is there a fallback rootfs or recovery shell?

## Recovery Initramfs

A recovery initramfs may include:

- shell or rescue UI
- storage repair tools
- network provisioning
- firmware updater
- signed service modules
- log extraction
- reset reason collection
- factory reset logic
- cryptographic verification tools

Recovery policy must define access control. A rescue shell on a serial console may be unacceptable in production without authentication or physical access assumptions.

## Manufacturing Initramfs

Manufacturing images may need:

- board test tools
- EEPROM/OTP provisioning
- MAC address programming
- storage flashing
- calibration data handling
- debug logs
- broader hardware access

Do not ship manufacturing initramfs as production recovery unless that is explicitly reviewed.

## Security Considerations

Initramfs can undermine product security if it contains:

- unsigned modules on a signed-module product
- shell access without policy
- hardcoded credentials
- private keys
- tools that bypass update verification
- debug boot arguments
- writable hooks that attackers can modify

Secure boot should verify the initramfs or include it in a verified boot flow when it is part of the trust chain.

## Inspecting Initramfs

Commands depend on format and distribution.

Common approaches:

```sh
lsinitramfs initramfs.img
unmkinitramfs initramfs.img out
file initramfs.img
```

Generic cpio-style inspection may look like:

```sh
mkdir /tmp/initramfs
cd /tmp/initramfs
zcat /path/to/initramfs.img | cpio -id
```

Use the tool appropriate to the compression and format your build system generates.

## Runtime Evidence

Boot logs should show:

- initramfs unpacking or early userspace start
- module load failures
- firmware load failures
- rootfs discovery
- switch to real rootfs
- recovery fallback decisions

Capture:

```sh
dmesg
cat /proc/cmdline
mount
cat /proc/mounts
```

If early userspace fails before persistent logging, use serial console, early console, or a lab debug initramfs.

## Test Matrix

Test:

```text
boot without initramfs, if intended to work
boot with normal initramfs
boot with missing root device
boot with missing firmware
boot with corrupted rootfs
boot recovery path
boot with signed-module enforcement
boot after kernel version update
```

Do not declare an initramfs correct because normal boot succeeds once.

## Common Failure Modes

| Symptom | Likely Cause | First Checks |
| --- | --- | --- |
| cannot mount rootfs | missing storage/fs driver or wrong root arg | initramfs contents and cmdline |
| module load fails | module version/signature mismatch | `uname -r`, dmesg |
| firmware missing | firmware not included early | `/lib/firmware` in image |
| recovery shell unavailable | `/init` logic or console missing | initramfs manifest |
| production exposes shell | rescue config shipped accidentally | image profile |
| update boots old modules | stale initramfs artifact | artifact versioning |
| secure boot bypass | initramfs not verified | boot-chain policy |

## Practice Exercises

### Exercise 1: Manifest Audit

Generate an initramfs manifest and classify every file:

```text
boot critical
firmware
module
recovery
diagnostic
unwanted
```

### Exercise 2: Missing Root Driver Test

Build the root storage driver as a module and remove it from initramfs in a lab image. Confirm the failure mode and logs.

### Exercise 3: Recovery Drill

Corrupt or hide the normal rootfs in a lab setup and verify the recovery path works without manual reflashing.

## Debugging Checklist

- Check initramfs contents.
- Check kernel command line root arguments.
- Check firmware and module paths.
- Check whether required drivers are built in or included in initramfs.
- Check module version and signatures.
- Check `/init` permissions and interpreter.
- Check rootfs discovery timing and `rootwait`.
- Check recovery shell/access policy.
- Archive initramfs manifest with releases.

## Related Topics

- [Initramfs And Built-In Root Filesystem](../../build-systems/advanced/linux-kernel/initramfs-and-built-in-rootfs.md)
- [Built-In Vs Module Policy](built-in-vs-module-policy.md)
- [Embedded Linux](../../embedded-linux/index.md)
- [Kernel Command Line Policy](kernel-command-line-policy.md)
- [Module Signing And Hardening](module-signing-and-hardening.md)

## Official References

- [Using the initial RAM disk](https://docs.kernel.org/admin-guide/initrd.html)
- [The kernel command-line parameters](https://docs.kernel.org/admin-guide/kernel-parameters.html)
- [Kernel module signing facility](https://docs.kernel.org/admin-guide/module-signing.html)

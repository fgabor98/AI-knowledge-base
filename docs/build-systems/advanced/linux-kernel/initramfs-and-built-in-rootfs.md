---
status: draft
reviewed: false
domain: build-systems
difficulty: advanced
reviewer: null
last_reviewed: null
---

# Initramfs and Built-In Root Filesystem

## What Problem Does This Solve?

An initramfs provides early userspace before the real root filesystem is mounted. It can load modules, assemble storage, unlock encryption, run diagnostics, or provide a rescue shell.

For embedded Linux, initramfs is useful when boot-critical functionality cannot or should not be built directly into the kernel.

## Core Concepts

- initramfs
- initrd
- early userspace
- built-in initramfs
- external initramfs
- rootfs handoff
- `/init`
- `CONFIG_INITRAMFS_SOURCE`
- bootloader ramdisk loading
- boot-critical driver

## Mental Model

Boot flow with initramfs:

```text
bootloader
-> kernel image
-> optional DTB
-> optional initramfs
-> kernel initializes early drivers
-> run /init from initramfs
-> mount real rootfs
-> switch_root or pivot_root
-> real init system
```

Without initramfs, the kernel must have everything needed to mount the real rootfs built in.

## Built-In Vs External Initramfs

### Built-In Initramfs

Configured into the kernel image:

```text
CONFIG_INITRAMFS_SOURCE="/path/to/initramfs"
```

Advantages:

- single kernel image contains early userspace
- bootloader does not need separate ramdisk handling
- useful for simple rescue images

Disadvantages:

- changing initramfs requires rebuilding kernel
- image can grow significantly
- easy to mix product policy into kernel build

### External Initramfs

Loaded by the bootloader as a separate artifact.

Advantages:

- can update independently from kernel source
- visible as a release artifact
- flexible for boot scripts and FIT images

Disadvantages:

- bootloader must load it correctly
- artifact matching becomes more important
- deployment can accidentally use an old ramdisk

## Minimal Initramfs Contents

A minimal initramfs needs:

```text
/init
/bin/busybox
/dev
/proc
/sys
/newroot
```

Example `/init`:

```sh
#!/bin/sh
mount -t proc proc /proc
mount -t sysfs sysfs /sys
mount -t devtmpfs devtmpfs /dev

echo "early userspace started"

mount /dev/mmcblk0p2 /newroot
exec switch_root /newroot /sbin/init
```

The actual device path and mount options depend on board and storage layout.

## Creating A Simple CPIO Archive

From an initramfs directory:

```sh
find . -print0 | cpio --null -ov --format=newc | gzip -9 > ../initramfs.cpio.gz
```

The kernel understands common compressed cpio initramfs formats when the relevant decompressor support is enabled.

## Kernel Configuration

Useful options:

```text
CONFIG_BLK_DEV_INITRD=y
CONFIG_INITRAMFS_SOURCE=""
CONFIG_DEVTMPFS=y
CONFIG_DEVTMPFS_MOUNT=y
```

If compression is used, ensure the kernel supports the format:

```text
CONFIG_RD_GZIP=y
CONFIG_RD_XZ=y
CONFIG_RD_ZSTD=y
```

Choose the compression that matches your bootloader, kernel, and build pipeline.

## Boot-Critical Drivers

If the real rootfs is on eMMC, SD, USB, NVMe, NAND, NFS, or encrypted storage, the kernel needs enough support to reach it.

Two strategies:

1. Build boot-critical drivers into the kernel.
2. Put required modules and loading logic in initramfs.

Built-in is simpler for many embedded products. Initramfs is useful when rootfs discovery is dynamic or storage setup is complex.

## Loading Modules In Initramfs

The initramfs must contain:

- matching modules
- dependency files
- `modprobe` or equivalent tooling
- `/lib/modules/<kernelrelease>/`

Example:

```sh
modprobe mmc_block
modprobe ext4
```

This only works if the initramfs modules match the running kernel.

## Kernel Command Line

Common parameters:

```text
root=/dev/mmcblk0p2
rootwait
rootfstype=ext4
init=/init
rdinit=/init
```

For debugging:

```text
ignore_loglevel
earlycon
initcall_debug
```

The exact command line may come from U-Boot environment, boot script, FIT config, or firmware.

## Handoff To Real Rootfs

Common handoff tools:

- `switch_root`
- `pivot_root`
- custom init script

Typical sequence:

```sh
mount real rootfs at /newroot
move or recreate /dev, /proc, /sys
exec switch_root /newroot /sbin/init
```

If handoff fails, keep an emergency shell if the product security model allows it.

## FIT Image Integration

Some embedded systems package kernel, DTB, and ramdisk into a FIT image.

Validate:

- selected FIT configuration
- ramdisk entry
- hashes/signatures
- load addresses
- boot command

A correct initramfs file on disk does not matter if U-Boot selects a FIT config that does not include it.

## Yocto And TI SDK Integration

Yocto can generate initramfs images and bundle them with kernel artifacts.

Check:

- selected initramfs image recipe
- whether initramfs is bundled into kernel or deployed separately
- deploy directory contents
- image dependencies
- final bootloader configuration

For TI SDK systems, identify whether the SDK expects a separate ramdisk, a FIT image, or no initramfs in the default boot flow.

## Debugging Early Userspace

Useful symptoms:

- kernel panic: no working init found
- cannot mount rootfs
- module load failure
- device node missing
- switch_root failure
- bootloader did not load ramdisk

Checks:

```sh
lsinitramfs initramfs.cpio.gz
file initramfs.cpio.gz
```

On systems without `lsinitramfs`:

```sh
mkdir unpack
cd unpack
gzip -dc ../initramfs.cpio.gz | cpio -idmv
```

## Common Mistakes

- Building storage driver as module without initramfs support.
- Shipping initramfs modules from a different kernel.
- Forgetting `/init` or missing execute permission.
- Bootloader loads old ramdisk.
- Kernel lacks decompressor support for the initramfs format.
- Assuming `root=` is correct without checking actual device enumeration.
- Not archiving initramfs with kernel release artifacts.

## Debugging Checklist

- Confirm whether initramfs is built-in or external.
- Confirm bootloader loads it.
- Confirm kernel config supports initrd/initramfs.
- Confirm `/init` exists and is executable.
- Confirm required modules match `uname -r`.
- Confirm root device appears before mount.
- Confirm command line root parameters.
- Confirm handoff command.
- Archive initramfs with kernel release artifacts.

## Related Topics

- [Cross-Building and Installing](cross-building-and-installing.md)
- [Modules and External Modules](modules-and-external-modules.md)
- [Kernel Release Artifacts](kernel-release-artifacts.md)
- [Boot Debugging and Runtime Validation](../bsp-integration/boot-debugging-and-runtime-validation.md)

## References

- Linux kernel initramfs documentation
- Linux kernel admin guide
- U-Boot documentation
- Yocto Project image and kernel documentation

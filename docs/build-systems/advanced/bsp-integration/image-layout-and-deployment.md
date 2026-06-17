---
status: draft
reviewed: false
domain: build-systems
difficulty: advanced
reviewer: null
last_reviewed: null
---

# Image Layout and Deployment

## What Problem Does This Solve?

Image layout decides where bootloader, kernel, device trees, root filesystem, data partitions, recovery artifacts, and update slots live on storage. Deployment puts those artifacts onto real media.

If image layout and deployment are not understood, a correct build can still produce a board that boots old files, cannot update safely, or cannot be recovered in production.

## Core Concepts

- partition table
- boot partition
- rootfs partition
- data partition
- recovery partition
- A/B slots
- U-Boot environment storage
- raw NAND
- UBI
- UBIFS
- WIC
- genimage
- flashing script
- factory image
- OTA image

## Mental Model

Separate artifact creation from storage layout:

```text
kernel Image, DTB, rootfs.ext4, U-Boot
-> image layout metadata
-> partitioned image or flashing package
-> physical storage
-> bootloader load commands
```

The build output is not enough. The board boots according to storage layout and bootloader environment.

## Common Storage Types

### SD / eMMC

Common layout:

```text
partition 1: boot filesystem, often FAT
partition 2: root filesystem, ext4/squashfs/etc.
partition 3: data or recovery
```

Often used with WIC, genimage, or vendor flashing scripts.

### Raw NAND

Common concepts:

```text
MTD partitions
UBI volumes
UBIFS rootfs
bad block handling
wear leveling
```

Raw NAND should not be treated like a block device. Use the correct UBI/UBIFS tooling and bootloader support.

### NOR / SPI Flash

Often used for bootloader, environment, or small recovery components.

Key concerns:

- fixed offsets
- erase block size
- environment location
- redundant environment
- secure boot storage constraints

## Layout Metadata

### WIC

Yocto commonly uses WIC `.wks` files to describe partitioned images.

Questions:

- which files go into the boot partition?
- which rootfs is used?
- are labels or UUIDs assigned?
- does kernel command line use label, UUID, or device path?

### genimage

Buildroot workflows often use `genimage` configs.

Questions:

- which input files are consumed?
- where are boot artifacts placed?
- what partition sizes are fixed?
- does post-image logic modify outputs?

### Vendor Flashing Scripts

Vendor BSPs may use scripts that wrap partitioning, image packing, signing, or flashing.

Questions:

- does the script use current build outputs?
- does it flash SD, eMMC, QSPI, or NAND?
- does it preserve data partitions?
- does it select a boot slot?

## Deployment Workflows

### Factory Image

Factory images often initialize the whole device:

- partition table
- bootloader
- rootfs
- data partition defaults
- recovery partition
- certificates or identity later in provisioning

Factory images may be destructive by design.

### Development Flash

Development flashing may update only:

- boot partition
- kernel image
- DTB
- rootfs partition
- one application package

This is fast but can hide product integration issues if it differs from factory flow.

### OTA Image

OTA artifacts usually update through a controlled client:

- RAUC bundle
- SWUpdate bundle
- Mender artifact
- OSTree commit

OTA artifacts are not the same as factory images. They must handle rollback, compatibility, signatures, and interrupted update behavior.

## Minimal Inspection Commands

Inspect partition image:

```sh
fdisk -l image.wic
parted image.wic print
```

Inspect block devices on target:

```sh
lsblk
blkid
mount
cat /proc/mtd
```

Inspect boot partition:

```sh
mkdir -p /tmp/bootmnt
mount -o loop,ro boot.vfat /tmp/bootmnt
find /tmp/bootmnt -maxdepth 2 -type f
```

Inspect rootfs:

```sh
mkdir -p /tmp/rootmnt
mount -o loop,ro rootfs.ext4 /tmp/rootmnt
find /tmp/rootmnt/boot /tmp/rootmnt/lib/modules -maxdepth 2
```

## Common Scenarios

### Board Boots Old Kernel After Flashing

Likely causes:

- flashed rootfs but not boot partition
- U-Boot loads from eMMC while SD was updated
- boot script uses TFTP
- A/B slot points to old partition
- update system did not mark slot active

Debug:

```text
U-Boot: printenv bootcmd bootargs
Linux: cat /proc/cmdline
Linux: lsblk
```

### Rootfs Updated But Kernel Modules Mismatch

Symptoms:

- `modprobe` fails
- driver missing
- module version mismatch

Likely causes:

- kernel image and rootfs modules from different builds
- boot partition updated but rootfs not updated
- rootfs updated but kernel image not updated

Check:

```sh
uname -r
find /lib/modules -maxdepth 1 -type d
```

### DTB In FIT Image

If kernel and DTB are packed into a FIT image, replacing a standalone `.dtb` file may do nothing.

Debug:

- inspect U-Boot boot command
- inspect FIT image contents
- confirm which DTB is selected

### Data Partition Preservation

Development and OTA updates often preserve data partitions. Factory flashing may wipe them.

Always document:

- which partitions are destructive
- which are preserved
- which contain identity/calibration data

## Common Mistakes

- Treating image generation as a file copy.
- Flashing the wrong storage device.
- Updating boot artifacts without updating rootfs modules.
- Updating rootfs without matching kernel image.
- Forgetting saved U-Boot environment location.
- Using `/dev/mmcblk0p2` in bootargs when device numbering can change.
- Ignoring A/B slot selection.
- Testing development flash flow but releasing factory or OTA flow.

## Debugging Checklist

- Draw the partition layout.
- Identify active boot media.
- Identify active slot.
- Check bootloader environment.
- Inspect image contents before flashing.
- Verify checksums after flashing where possible.
- Compare `uname -r` with `/lib/modules`.
- Confirm DTB source and deployed DTB.
- Confirm factory and OTA images update the intended partitions.

## Related Topics

- [BSP Build Integration](../bsp-build-integration.md)
- [Artifact Flow and Provenance](artifact-flow-and-provenance.md)
- [Boot Debugging and Runtime Validation](boot-debugging-and-runtime-validation.md)
- [Release Reproducibility](release-reproducibility.md)

## References

- Yocto WIC documentation
- Buildroot manual
- U-Boot documentation
- Linux MTD and UBI documentation

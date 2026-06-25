---
status: draft
reviewed: false
domain: build-systems
difficulty: advanced
reviewer: null
last_reviewed: null
---

# Deployment and Flashing

## Goal

Understand how generated TI SDK artifacts reach real boot media and how to prove that the board is booting what you built.

## Deployment Targets

Common deployment media:

- SD card
- eMMC
- OSPI/QSPI flash
- NAND
- NFS rootfs during development
- USB or network recovery paths

Supported methods depend on SoC, board design, boot switches, and SDK release.

## SD Card Workflow

Typical SD workflow:

```bash
lsblk
sudo bmaptool copy image.wic.xz /dev/sdX
sync
```

or:

```bash
xzcat image.wic.xz | sudo dd of=/dev/sdX bs=4M status=progress conv=fsync
```

Be careful with device names. `dd` to the wrong block device is destructive.

## Partition-Level Updates

During development, you may update only boot files or only rootfs content. This is faster but riskier.

Use partition-level updates when:

- you know which partition U-Boot reads
- you know whether the board boots from SD, eMMC, or flash
- you know persistent U-Boot environment does not redirect boot
- you record exactly which artifacts were copied

For release validation, test a full image write.

## eMMC And Flash

eMMC and flash workflows often involve:

- booting from SD first
- writing eMMC from Linux
- U-Boot commands for flash erase/write
- TI-specific flashing tools
- serial or USB boot recovery
- partition layout constraints

The build-system concern is artifact mapping. Know which generated file is written to which offset, partition, or filesystem path.

## Proving What Booted

Collect:

- full serial boot log from reset
- U-Boot version
- U-Boot environment
- kernel version
- DTB model
- rootfs build ID or image manifest
- firmware load messages

Useful target commands:

```bash
uname -a
cat /proc/cmdline
cat /etc/os-release
dmesg | head -n 80
dmesg | grep -i firmware
find /boot -maxdepth 2 -type f -printf '%p %s\n'
```

## Artifact-To-Media Checklist

Before saying "the new build does not work", verify:

- correct `.wic` or boot files were copied
- correct block device was written
- board boot switches select that media
- eMMC/OSPI does not override SD boot
- persistent U-Boot environment does not redirect to old files
- boot partition contains updated `tiboot3.bin`, `tispl.bin`, `u-boot.img`, kernel, and DTB as applicable
- rootfs partition is from the same build

## Common Mistakes

- Updating `/boot` on the rootfs partition while U-Boot reads a separate boot partition.
- Writing an SD card image but booting from eMMC.
- Reusing an old persistent U-Boot environment.
- Copying only kernel image but not matching modules.
- Copying only DTB but not updating WIC image for release testing.
- Not saving serial logs from power-on.

## Related Topics

- [Boot Artifact Pipeline](boot-artifact-pipeline.md)
- [U-Boot Integration](u-boot-integration.md)
- [Debugging TI SDK Builds and Boots](debugging-ti-sdk-builds-and-boots.md)

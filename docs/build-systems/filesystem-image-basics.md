---
status: draft
reviewed: false
domain: build-systems
difficulty: intermediate
reviewer: null
last_reviewed: null
---

# Filesystem Image Basics

## What Problem Does This Solve?

Embedded Linux builds do not only produce binaries. They produce root filesystems, boot partitions, disk images, recovery media, and deployable artifacts. This page introduces the image formats and layout concepts used before Yocto WIC, Buildroot images, or vendor SDK image tooling.

## Core Concepts

- root filesystem
- boot partition
- partition table
- ext4
- squashfs
- initramfs
- tar rootfs
- UUID
- PARTUUID
- `/etc/fstab`
- bootloader load path

## Common Image Types

| Format | Use |
| --- | --- |
| tar rootfs | easy extraction into a partition or NFS root |
| ext4 image | writable rootfs image |
| squashfs | compressed read-only rootfs |
| cpio/initramfs | early userspace loaded by kernel |
| WIC/disk image | full media image with partitions |
| boot partition files | bootloader, kernel, DTB, scripts |

## Simple Rootfs Archive

```bash
sudo tar --numeric-owner -cpf rootfs.tar -C rootfs .
```

`--numeric-owner` matters because target UID/GID values must survive extraction even when host user names differ.

## Simple ext4 Image

```bash
dd if=/dev/zero of=rootfs.ext4 bs=1M count=256
mkfs.ext4 rootfs.ext4
mkdir mnt
sudo mount rootfs.ext4 mnt
sudo tar --numeric-owner -xpf rootfs.tar -C mnt
sudo umount mnt
```

This is the manual version of what build systems automate.

## Bootloader View

The bootloader usually needs:

- readable storage driver
- readable filesystem
- kernel path
- DTB path
- optional initramfs path
- kernel command line

Linux then needs:

- correct `root=`
- correct filesystem driver
- matching modules or built-in support
- valid `/sbin/init`
- valid `/etc/fstab` if mounting more filesystems

## Common Mistakes

- building a rootfs without device nodes where they are required
- using host UID/GID names instead of numeric ownership
- changing partition UUIDs without updating boot arguments
- putting the DTB in one partition while U-Boot reads another
- using squashfs and expecting runtime writes to persist
- forgetting that initramfs and rootfs are different artifacts

## Related Topics

- [Object Files and Linking](object-files-and-linking.md)
- [WIC and Partition Layouts](advanced/yocto-openembedded/wic-and-partition-layouts.md)
- [Initramfs, Recovery, And Manufacturing Images](advanced/initramfs-recovery-and-manufacturing-images.md)


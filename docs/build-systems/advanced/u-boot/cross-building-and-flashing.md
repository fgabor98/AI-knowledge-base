---
status: draft
reviewed: false
domain: build-systems
difficulty: advanced
reviewer: null
last_reviewed: null
---

# Cross-Building and Flashing

## What Problem Does This Solve?

U-Boot is usually cross-built on a host and deployed to boot media such as SD, eMMC, SPI NOR, NAND, or a network/recovery path. The build step and flashing step must agree about board, artifact, offset, and boot source.

For embedded Linux work, this page connects the build output to a board that actually boots the expected U-Boot.

## Core Concepts

- `CROSS_COMPILE`
- `O=`
- board defconfig
- boot media
- flash offset
- partition layout
- eMMC boot partitions
- SPI NOR
- SD card boot
- recovery path
- serial log verification

## Mental Model

```text
select defconfig
-> cross-build artifacts
-> identify required boot files
-> write to correct boot media/location
-> verify serial log and version
```

Building is only half the work. Deployment decides what the board runs.

## Basic Cross-Build

```sh
make O=build CROSS_COMPILE=aarch64-linux-gnu- board_defconfig
make O=build CROSS_COMPILE=aarch64-linux-gnu- -j8
```

Some architectures or platforms also require `ARCH`, but many U-Boot board defconfigs select the architecture. Follow the board documentation.

## Clean Output Per Board

Use separate output directories:

```text
build-am62x-evm/
build-custom-board/
build-debug/
build-release/
```

Avoid switching defconfigs in one output directory.

## Identifying Flash Artifacts

After build:

```sh
find build -maxdepth 3 -type f \( -name '*.bin' -o -name '*.img' -o -name '*.itb' -o -name 'MLO' \)
```

Then map each file to the board boot flow.

Do not flash every generated file blindly.

## SD Card Deployment

Common pattern:

```text
partition 1: boot artifacts
partition 2: rootfs
```

Some SoCs require raw writes at fixed offsets instead of filesystem copies.

Check:

- partition table
- boot ROM requirements
- file names
- filesystem type
- alignment
- offset

## eMMC Deployment

eMMC may have:

- user area
- boot0 partition
- boot1 partition
- hardware boot partition selection

A board may keep bootloader stages in eMMC boot0 while Linux uses the user area.

Common mistake: updating `/boot` in the rootfs or user partition while the SoC still boots an older bootloader from boot0.

## SPI NOR / NAND Deployment

SPI NOR and NAND often use fixed offsets or named partitions.

Check:

- flash map
- erase block size
- environment location
- redundant environment
- bad block handling for NAND
- recovery procedure

Never experiment with destructive flash writes without a recovery path.

## Network And UART Recovery

Many boards provide recovery paths:

- UART boot
- USB DFU
- TFTP
- fastboot
- JTAG
- ROM recovery mode

Before changing bootloader storage, know how to recover from a bad flash.

## Version Verification

Set a visible build identity where appropriate:

```text
U-Boot 2024.xx-product-g123456
```

After boot:

```text
U-Boot SPL ...
U-Boot ...
```

Check that serial logs show the expected version and build date. If not, you are not running the artifact you think you flashed.

## Environment Verification

Persistent environment can override new default boot commands.

At U-Boot prompt:

```text
printenv
env default -a
saveenv
```

Use environment reset carefully. It can change boot behavior and device identity.

## TI SDK Deployment

For TI Processor SDK, use SDK documentation and deploy output naming. Common generated/deployed artifacts can include:

```text
tiboot3.bin
tispl.bin
u-boot.img
```

Practical checks:

- selected machine
- selected boot media
- GP/HS security variant
- deploy directory files
- SD card creation script
- eMMC flashing procedure
- serial log stage names

## Common Mistakes

- Building one defconfig and flashing artifacts for another board.
- Flashing U-Boot proper but not SPL.
- Updating SD card while board boots eMMC.
- Updating eMMC user area while board boots boot0.
- Forgetting persistent environment.
- Using host tools from another SDK release.
- Not having a recovery path before flashing.

## Debugging Checklist

- Confirm defconfig.
- Confirm toolchain.
- Confirm output artifacts.
- Confirm boot media selected by straps/environment.
- Confirm flashing offset or filename.
- Confirm persistent environment.
- Confirm serial log version.
- Confirm SPL and U-Boot proper are both updated.
- Confirm recovery method exists.

## Related Topics

- [Source Tree and Outputs](source-tree-and-outputs.md)
- [SPL, TPL, and U-Boot Proper](spl-tpl-and-u-boot-proper.md)
- [FIT Images and Boot Artifacts](fit-images-and-boot-artifacts.md)
- [Image Layout and Deployment](../bsp-integration/image-layout-and-deployment.md)

## References

- U-Boot board documentation
- U-Boot environment documentation
- TI Processor SDK Linux documentation

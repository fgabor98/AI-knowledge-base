---
status: draft
reviewed: false
domain: build-systems
difficulty: advanced
reviewer: null
last_reviewed: null
---

# U-Boot Documentation Reading Guide

## What Problem Does This Solve?

The U-Boot `doc/` directory contains build, usage, board, driver model, environment, FIT, verified boot, and API documentation. It is a strong source, but reading it without order can be confusing because U-Boot spans ROM handoff, SPL, U-Boot proper, Linux handoff, and production flashing.

This guide gives a practical order for reading U-Boot documentation so you can understand the build system, board porting, boot flow, driver model, secure boot, and release workflows.

## Core Concepts

- `doc/`
- board docs
- build docs
- Kconfig
- SPL/TPL
- driver model
- environment
- boot flow
- FIT image
- verified boot
- board porting
- command documentation
- API documentation

## Mental Model

Read U-Boot documentation in boot-chain order:

```text
how U-Boot is built
-> what artifacts are produced
-> how stages execute
-> how devices bind
-> how boot flow selects artifacts
-> how Linux is launched
-> how artifacts are signed, flashed, and maintained
```

This matches how real embedded failures appear on boards.

## Phase 1: Documentation Orientation

Start with:

```text
doc/
doc/develop/
doc/usage/
doc/board/
```

Goal:

- understand the split between developer documentation, user documentation, board documentation, and command usage
- learn where board-specific instructions live
- identify whether your SoC vendor has board docs in-tree

Do not read every board page. Find the closest reference board and read that board's page first.

## Phase 2: Build System And Configuration

Read:

```text
doc/build/
doc/develop/
doc/develop/kconfig.rst
```

Also inspect source-side inputs:

```text
configs/
Kconfig
Makefile
scripts/
```

What to learn:

- how to select a board defconfig
- how final `.config` is generated
- how generated config feeds the build
- how `O=` output directories work
- how host tools are built
- how board-specific image packaging appears in build output

Practice:

```sh
make O=build CROSS_COMPILE=aarch64-linux-gnu- <board>_defconfig
make O=build CROSS_COMPILE=aarch64-linux-gnu- -j8
find build -maxdepth 3 -type f
```

You should be able to answer:

- which defconfig selected this board?
- where is final `.config`?
- which artifact should be flashed?
- which artifacts are intermediate?

## Phase 3: Board Documentation

Read the closest board docs:

```text
doc/board/<vendor>/
```

For TI work, look for TI board documentation under the U-Boot tree and compare it with the Processor SDK documentation.

What to learn:

- required toolchain
- board defconfig
- required firmware blobs
- generated artifact names
- boot media layout
- flashing procedure
- known boot modes
- secure device notes

Embedded rule:

The board documentation defines the flashing contract. The existence of `u-boot.bin` does not mean that file is flashable for your board.

## Phase 4: SPL, TPL, And Staged Boot

Read:

```text
doc/develop/spl.rst
```

Also inspect board docs for stage-specific artifact names.

What to learn:

- why SPL/TPL exist
- what runs before full U-Boot
- how SPL loads U-Boot proper
- size constraints
- stage-specific configuration
- SPL driver model limitations
- DRAM initialization responsibilities

Practice:

- identify SPL artifacts
- inspect SPL size
- check `CONFIG_SPL_*` symbols
- capture serial logs from reset

You should be able to distinguish:

- ROM failure
- SPL failure
- U-Boot proper failure
- Linux handoff failure

## Phase 5: Driver Model

Read:

```text
doc/develop/driver-model/
```

Then read command usage related to inspection:

```text
doc/usage/cmd/
```

What to learn:

- uclasses
- bind/probe lifecycle
- device tree matching
- parent/child devices
- pre-relocation behavior
- SPL driver model

Practice at U-Boot prompt:

```text
dm tree
dm uclass
dm drivers
```

You should be able to debug:

- driver built but device missing
- device present but not probed
- SPL cannot use storage while U-Boot proper can
- missing clock/pinctrl/regulator dependency

## Phase 6: U-Boot Device Tree

Read U-Boot device tree documentation under:

```text
doc/develop/
doc/usage/
```

Then inspect:

```text
arch/<arch>/dts/
```

What to learn:

- `CONFIG_DEFAULT_DEVICE_TREE`
- U-Boot internal DTB
- SPL DTB
- DTB passed to Linux
- U-Boot-specific properties
- pre-relocation markers
- overlays and fixups where used

Practice:

```sh
grep '^CONFIG_DEFAULT_DEVICE_TREE' build/.config
find build -name '*dtb'
```

On target:

```text
fdt addr ${fdt_addr_r}
fdt print / model
```

Then verify Linux runtime:

```sh
cat /proc/device-tree/model
```

## Phase 7: Environment And Boot Flow

Read:

```text
doc/usage/environment.rst
doc/develop/bootstd/
doc/usage/cmd/
```

Documentation names vary by U-Boot version, so search in `doc/` for:

```text
environment
bootstd
bootflow
bootmeth
distro
extlinux
```

What to learn:

- default vs saved environment
- `bootcmd`
- `boot_targets`
- standard boot
- boot methods
- boot scripts
- `extlinux.conf`
- persistent environment storage

Practice:

```text
printenv
printenv bootcmd
printenv boot_targets
bootflow scan
bootflow list
```

Command availability depends on U-Boot version and configuration.

You should be able to trace one full boot path from reset to Linux kernel entry.

## Phase 8: Commands And Usage Docs

Read relevant command docs:

```text
doc/usage/cmd/
```

Prioritize:

- `bootm`
- `booti`
- `bootz`
- `load`
- `fatload`
- `ext4load`
- `mmc`
- `sf`
- `dhcp`
- `tftpboot`
- `fdt`
- `env`
- `bdinfo`
- `version`
- `dm`

What to learn:

- command syntax
- address arguments
- image formats
- filesystem loading
- network loading
- device tree inspection
- environment manipulation

This phase makes serial-console debugging practical.

## Phase 9: FIT Images And Linux Handoff

Read:

```text
doc/usage/fit/
doc/uImage.FIT/
```

Documentation path varies by U-Boot version. Search:

```sh
find doc -iname '*fit*' -o -iname '*verified*'
```

What to learn:

- ITS source
- ITB output
- image nodes
- configuration nodes
- hashes
- signatures
- load address
- entry address
- `bootm` FIT syntax

Practice:

```sh
dumpimage -l image.itb
mkimage -l image.itb
```

You should be able to identify:

- which kernel is in the FIT
- which DTBs are included
- which configuration is default
- whether signatures exist
- which configuration U-Boot booted

## Phase 10: Verified Boot And Secure Boot

Read:

```text
doc/uImage.FIT/signature.txt
doc/usage/fit/
doc/develop/
```

Search for:

```text
verified boot
signature
vboot
hash
public key
```

Then read vendor-specific secure boot docs outside U-Boot if the SoC ROM authenticates first-stage images.

What to learn:

- FIT signature model
- where public keys live
- what is verified by U-Boot
- what is verified by the SoC ROM
- what changes require resigning
- debug vs production keys
- secure device variants

Embedded focus:

- U-Boot verified boot and SoC secure boot are related but not identical
- first-stage signing may be vendor-specific
- signed FIT changes must be release-controlled

## Phase 11: Board Porting And API Development

Read:

```text
doc/develop/
doc/develop/driver-model/
doc/board/<vendor>/
```

Then inspect source:

```text
board/<vendor>/<board>/
arch/<arch>/dts/
drivers/
include/configs/
```

What to learn:

- board init hooks
- board detection
- DRAM setup
- PMIC/regulator setup
- pinmux setup
- device tree ownership
- driver model APIs
- command implementation patterns

For custom boards, read docs in this order:

1. closest reference board page
2. SPL documentation
3. driver model documentation
4. environment/boot flow documentation
5. FIT/verified boot documentation if production requires it

## Phase 12: Flashing, Updating, And Recovery

Read board docs and command docs for:

- MMC/eMMC
- SPI flash
- NAND
- USB DFU
- fastboot
- TFTP
- boot scripts

Search:

```text
dfu
fastboot
mmc
sf
nand
tftp
```

What to learn:

- where first-stage artifacts live
- whether eMMC boot0/boot1 are used
- flash offsets
- redundant environment
- recovery mode
- readback verification

Never treat flashing as generic across boards.

## Recommended Reading Order For U-Boot Build Mastery

1. `doc/build/`
2. `doc/develop/kconfig.rst`
3. closest `doc/board/<vendor>/` page
4. `doc/develop/spl.rst`
5. `doc/develop/driver-model/`
6. U-Boot device tree docs and board DTS
7. `doc/usage/environment.rst`
8. standard boot/bootflow documentation
9. `doc/usage/cmd/`
10. FIT image documentation
11. verified boot documentation
12. board flashing/recovery documentation

## Recommended Reading Order For Board Porting

1. closest vendor board documentation
2. generated artifact and flashing instructions
3. SPL documentation
4. board defconfig and Kconfig docs
5. U-Boot DTS and driver model docs
6. environment and boot flow docs
7. FIT and Linux handoff docs
8. secure boot/signing docs if production hardware requires it
9. update/recovery docs

## What Good Understanding Looks Like

You understand U-Boot build and board support when you can:

- select and modify the correct defconfig
- identify every generated boot artifact
- explain which artifact the ROM, SPL, and U-Boot proper consume
- distinguish default and saved environment
- trace `bootcmd` to loaded kernel/DTB/initramfs
- inspect FIT images and selected configurations
- debug a built-but-unprobed driver
- explain pre-relocation constraints
- bring up a custom board from an EVM baseline
- preserve release artifacts and serial logs
- separate vendor, board, product, and temporary patches

## Related Topics

- [Environment and Boot Flow](environment-and-boot-flow.md)
- [Board Porting and Bring-Up](board-porting-and-bring-up.md)
- [Driver Model and Pre-Relocation](driver-model-and-pre-relocation.md)
- [Secure Boot and Signing](secure-boot-and-signing.md)
- [Release Artifacts and Provenance](release-artifacts-and-provenance.md)

## References

- U-Boot `doc/` directory
- U-Boot `doc/develop/`
- U-Boot `doc/usage/`
- U-Boot `doc/board/`

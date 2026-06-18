---
status: draft
reviewed: false
domain: build-systems
difficulty: advanced
reviewer: null
last_reviewed: null
---

# U-Boot Build System

## What Problem Does This Solve?

U-Boot builds the early boot artifacts that prepare hardware and load Linux. Its build system uses Make, Kbuild-style object selection, Kconfig, board defconfigs, generated configuration, device trees, SPL/TPL stages, and board-specific image packaging.

For embedded Linux work, U-Boot build knowledge is essential when bringing up boards, changing boot media, adjusting device trees, enabling secure boot, or integrating vendor BSP releases.

## Core Concepts

- board defconfig
- Kconfig
- generated configuration
- SPL
- TPL
- U-Boot proper
- U-Boot device tree
- boot artifacts
- FIT images
- board directories
- `ARCH`
- `CROSS_COMPILE`
- `O=`
- flashing layout

## Mental Model

U-Boot build flow:

```text
board defconfig
-> .config and generated headers
-> SPL/TPL if enabled
-> U-Boot proper
-> board-specific packaged artifacts
-> flash/boot media layout
```

The "right output file" is board-specific. Never assume `u-boot.bin` is the artifact to flash without checking the board documentation.

## Learning Materials

1. [Source Tree and Outputs](source-tree-and-outputs.md)
2. [Board Defconfigs](board-defconfigs.md)
3. [Kconfig and Generated Config](kconfig-and-generated-config.md)
4. [SPL, TPL, and U-Boot Proper](spl-tpl-and-u-boot-proper.md)
5. [Device Tree in U-Boot](device-tree-in-u-boot.md)
6. [FIT Images and Boot Artifacts](fit-images-and-boot-artifacts.md)
7. [Cross-Building and Flashing](cross-building-and-flashing.md)
8. [Debugging U-Boot Builds](debugging-u-boot-builds.md)

## Detailed Roadmap

### 1. Source Tree And Outputs

Learn:

- U-Boot source tree layout
- `arch/`, `board/`, `configs/`, `drivers/`, `include/`
- generated output directories
- common artifacts: `u-boot`, `u-boot.bin`, `u-boot.img`, `u-boot.itb`, `SPL`, `spl/u-boot-spl.bin`
- board-specific combined images

Practice:

- build a known board defconfig
- list generated artifacts
- identify which artifact the board boots

### 2. Board Defconfigs

Learn:

- `configs/*_defconfig`
- machine/board naming
- vendor board directories
- `make <board>_defconfig`
- `savedefconfig`
- board migration across releases

Practice:

- inspect an EVM defconfig
- modify one option
- save and compare defconfig changes

### 3. Kconfig And Generated Config

Learn:

- U-Boot Kconfig symbols
- generated config headers
- legacy config headers where present
- `CONFIG_*` ownership
- command and driver selection

Practice:

- enable/disable a command
- enable/disable a driver
- confirm generated configuration changed

### 4. SPL, TPL, And U-Boot Proper

Learn:

- why SPL/TPL exist
- size constraints
- RAM initialization responsibilities
- boot media loading
- separate SPL configuration
- handoff to U-Boot proper

Practice:

- identify whether a board builds SPL
- inspect SPL output size
- trace SPL artifact packaging

### 5. Device Tree In U-Boot

Learn:

- U-Boot DTS locations
- `CONFIG_DEFAULT_DEVICE_TREE`
- shared vs separate kernel/U-Boot device trees
- driver model
- pre-relocation constraints

Practice:

- find the selected U-Boot DTS
- change a harmless property
- verify rebuilt DTB/artifact

### 6. FIT Images And Boot Artifacts

Learn:

- FIT image structure
- kernels, DTBs, ramdisks, configurations
- hashes and signatures
- load addresses
- boot scripts
- board-specific packaging tools

Practice:

- inspect a FIT image
- identify included DTBs and configurations
- understand signing implications before changing artifacts

### 7. Cross-Building And Flashing

Learn:

- `ARCH`
- `CROSS_COMPILE`
- output directories with `O=`
- boot media layout
- SD/eMMC/NOR/NAND deployment
- recovery path

Practice:

- cross-build U-Boot
- copy artifacts to boot media
- confirm serial boot log shows the expected version

### 8. Debugging U-Boot Builds

Learn:

- verbose builds
- stale defconfig issues
- wrong artifact flashed
- SPL size failures
- missing device tree nodes
- wrong boot command/environment

Practice:

- debug a build output mismatch
- debug a board booting an older U-Boot
- debug a missing driver command

## Common Mistakes

- Flashing the wrong artifact.
- Assuming every board uses the same U-Boot output format.
- Reusing a build directory after switching defconfigs.
- Forgetting SPL has different constraints from U-Boot proper.
- Editing generated config instead of defconfig/Kconfig source.
- Mixing U-Boot artifacts from one build with kernel/DTB artifacts from another.
- Updating the boot partition but leaving an older bootloader in eMMC boot0, SPI NOR, or another earlier boot source.
- Debugging Linux boot arguments without checking whether U-Boot environment overrides the expected boot script.

## Related Topics

- [BSP Build Integration](../bsp-build-integration.md)
- [Embedded Linux](../../../embedded-linux/index.md)
- [Linux Kernel Build System](../linux-kernel/index.md)

## References

- U-Boot documentation
- U-Boot board documentation
- U-Boot Kconfig files
- U-Boot FIT image documentation

---
status: draft
reviewed: false
domain: build-systems
difficulty: advanced
reviewer: null
last_reviewed: null
---

# Linux Kernel Build System

## What Problem Does This Solve?

The Linux kernel build system turns architecture-specific source, Kconfig selections, Kbuild rules, device trees, and module configuration into bootable kernel artifacts.

For embedded Linux work, understanding the kernel build system is required for board support, driver work, device tree changes, kernel configuration, external modules, and BSP integration.

## Core Concepts

- top-level kernel `Makefile`
- Kbuild
- Kconfig
- `.config`
- defconfig
- config fragments
- `ARCH`
- `CROSS_COMPILE`
- `O=`
- `M=`
- built-in objects
- loadable modules
- device tree builds
- kernel image outputs

## Mental Model

Kernel build flow:

```text
architecture + defconfig/fragments
-> .config
-> generated headers
-> Kbuild object selection
-> built-in objects and modules
-> vmlinux
-> architecture-specific images
-> DTBs and module install outputs
```

The kernel build is configuration-driven. Source files exist in the tree, but they are built only when selected by Kconfig and Kbuild.

## Roadmap Pages

Planned pages:

1. `source-tree-and-outputs.md`
2. `kconfig-and-defconfig.md`
3. `kbuild-objects-and-directories.md`
4. `modules-and-external-modules.md`
5. `device-tree-builds.md`
6. `cross-building-and-installing.md`
7. `debugging-kernel-builds.md`

## Detailed Roadmap

### 1. Source Tree And Outputs

Learn:

- top-level source tree layout
- `arch/`, `drivers/`, `include/`, `scripts/`, `tools/`
- `vmlinux`
- `Image`, `zImage`, `bzImage`
- modules: `*.ko`
- device trees: `*.dtb`
- generated headers
- output directories with `O=`

Practice:

- build a kernel natively
- build with `O=build`
- identify generated artifacts
- map output files back to build targets

### 2. Kconfig And Defconfig

Learn:

- Kconfig syntax basics
- `CONFIG_*` symbols
- dependencies and selects
- `defconfig`
- `menuconfig`
- `oldconfig`
- `savedefconfig`
- final `.config`
- generated `autoconf.h`

Practice:

- start from a defconfig
- change one option with `menuconfig`
- inspect `.config`
- save a minimal defconfig
- confirm a driver is enabled as built-in or module

### 3. Kbuild Objects And Directories

Learn:

- `obj-y`
- `obj-m`
- `obj-$(CONFIG_FOO)`
- directory recursion
- built-in archives
- module object selection
- generated dependency files

Practice:

- trace a driver from Kconfig option to built object
- add a simple source file to a local kernel tree
- understand why a file exists but is not built

### 4. Modules And External Modules

Learn:

- built-in vs module
- `M=$PWD`
- kernel build directory
- `Module.symvers`
- module versioning
- install paths
- `depmod`

Practice:

- build a simple external module
- cross-build the module
- install it into a staging rootfs
- inspect `modinfo`

### 5. Device Tree Builds

Learn:

- DTS and DTSI files
- architecture-specific DTB locations
- `dtbs` target
- overlays where used
- binding documentation
- generated DTBs

Practice:

- build DTBs only
- locate the selected DTB
- decompile with `dtc`
- compare source DTS with runtime `/proc/device-tree`

### 6. Cross-Building And Installing

Learn:

- `ARCH`
- `CROSS_COMPILE`
- output directories
- modules install
- headers install
- staging rootfs
- boot partition artifact copy

Practice:

- cross-build kernel and DTBs
- install modules into a staging rootfs
- copy kernel and DTB into a boot partition layout

### 7. Debugging Kernel Builds

Learn:

- `V=1`
- generated command lines
- missing config symbols
- missing generated headers
- stale output trees
- wrong compiler
- module/kernel version mismatch

Practice:

- debug a missing driver build
- debug a failed external module build
- debug mismatched modules on target

## Common Mistakes

- Editing `.config` without preserving defconfig or fragments.
- Building with the wrong `ARCH`.
- Forgetting the trailing hyphen in `CROSS_COMPILE`.
- Mixing kernel image, DTB, and modules from different builds.
- Building external modules against the wrong kernel build directory.
- Assuming device tree source changes affect the board without checking the deployed DTB.

## Related Topics

- [BSP Build Integration](../bsp-build-integration.md)
- [Embedded Linux](../../../embedded-linux/index.md)
- [Linux Kernel Programming](../../../linux-kernel/index.md)

## References

- Linux kernel Kbuild documentation
- Linux kernel Kconfig documentation
- Linux kernel external module documentation
- Linux kernel device tree documentation

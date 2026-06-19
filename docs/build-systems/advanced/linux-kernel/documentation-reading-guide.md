---
status: draft
reviewed: false
domain: build-systems
difficulty: advanced
reviewer: null
last_reviewed: null
---

# Kernel Documentation Reading Guide

## What Problem Does This Solve?

The Linux kernel `Documentation/` directory is large. Reading it randomly is inefficient, especially if the goal is embedded Linux board support, kernel builds, device trees, modules, and SoC BSP work.

This guide gives a practical reading order for the kernel documentation so you can understand the build system first, then configuration, modules, device tree, driver APIs, board support, debugging, and release workflows.

## Core Concepts

- `Documentation/`
- Kbuild
- Kconfig
- kernel configuration
- modules
- device tree
- driver model
- subsystem documentation
- userspace API
- admin guide
- process documentation

## Mental Model

Read kernel documentation in dependency order:

```text
how the kernel is built
-> how features are configured
-> how objects/modules are selected
-> how hardware is described
-> how drivers bind to hardware
-> how APIs and subsystems work
-> how to debug and maintain changes
```

For embedded Linux, this is more useful than reading by directory order.

## Phase 1: Orientation And Documentation Structure

Start with:

```text
Documentation/index.rst
Documentation/process/index.rst
Documentation/process/howto.rst
```

Goal:

- understand how kernel documentation is organized
- understand the difference between user docs, developer docs, API docs, and process docs
- learn how kernel docs refer to source files and subsystems

Do not try to read every process document yet. The first pass is orientation.

## Phase 2: Kbuild And Kernel Build Mechanics

Read:

```text
Documentation/kbuild/index.rst
Documentation/kbuild/makefiles.rst
Documentation/kbuild/kbuild.rst
Documentation/kbuild/modules.rst
Documentation/kbuild/headers_install.rst
Documentation/kbuild/reproducible-builds.rst
```

What to learn:

- top-level kernel `make` flow
- how Kbuild Makefiles select objects
- `obj-y`, `obj-m`, and directory recursion
- built-in vs loadable module build paths
- external module builds
- generated headers
- exported userspace headers
- reproducibility controls

Practice while reading:

```sh
make O=build ARCH=arm64 defconfig
make O=build ARCH=arm64 V=1 Image modules dtbs
make O=build ARCH=arm64 M=$PWD modules
```

You should be able to answer:

- why does a source file exist but not build?
- why is a driver built-in instead of a module?
- what does `M=` do?
- what artifacts must be archived for a release?

## Phase 3: Kconfig And Configuration

Read:

```text
Documentation/kbuild/kconfig-language.rst
Documentation/kbuild/kconfig-macro-language.rst
Documentation/kbuild/kconfig.rst
```

Then connect it with practical commands:

```sh
make O=build ARCH=arm64 menuconfig
make O=build ARCH=arm64 olddefconfig
make O=build ARCH=arm64 savedefconfig
```

What to learn:

- symbol types
- dependencies
- prompts
- defaults
- `select`
- `imply`
- choices
- generated `.config`
- generated `autoconf.h`

Embedded focus:

- boot-critical storage drivers must usually be built in
- fragments are requests, final `.config` is truth
- dependencies can silently block requested options
- vendor defconfigs should not be blindly copied across releases

## Phase 4: Modules And External Modules

Read:

```text
Documentation/kbuild/modules.rst
Documentation/admin-guide/modules.rst
Documentation/core-api/symbol-namespaces.rst
```

Also inspect source-side files:

```text
scripts/mod/
include/linux/module.h
```

What to learn:

- `*.ko` build flow
- `Module.symvers`
- `modpost`
- exported symbols
- symbol versioning
- `vermagic`
- module install layout

Practice:

```sh
make O=build ARCH=arm64 modules
make O=build ARCH=arm64 INSTALL_MOD_PATH=$PWD/rootfs modules_install
modinfo path/to/module.ko
```

You should be able to debug:

- `Invalid module format`
- `Unknown symbol`
- missing module package
- module built against wrong kernel

## Phase 5: Device Tree Basics

Read:

```text
Documentation/devicetree/index.rst
Documentation/devicetree/usage-model.rst
Documentation/devicetree/bindings/writing-bindings.rst
Documentation/devicetree/bindings/submitting-patches.rst
```

Then read binding files for the hardware you actually use:

```text
Documentation/devicetree/bindings/serial/
Documentation/devicetree/bindings/mmc/
Documentation/devicetree/bindings/net/
Documentation/devicetree/bindings/spi/
Documentation/devicetree/bindings/i2c/
Documentation/devicetree/bindings/gpio/
Documentation/devicetree/bindings/pinctrl/
Documentation/devicetree/bindings/regulator/
Documentation/devicetree/bindings/clock/
Documentation/devicetree/bindings/arm/
```

What to learn:

- DTS and DTSI inclusion
- compatible strings
- `status`
- address and size cells
- interrupts
- clocks
- resets
- regulators
- pinctrl
- PHY links
- YAML binding validation

Practice:

```sh
make O=build ARCH=arm64 dtbs
make O=build ARCH=arm64 dtbs_check
dtc -I dtb -O dts -o board.dts board.dtb
```

Embedded focus:

- a DTB can compile but still violate bindings
- U-Boot may pass a different DTB than the one you edited
- runtime `/proc/device-tree` is the final truth

## Phase 6: Driver Model And Core Driver APIs

Read based on the hardware you work with. Start with:

```text
Documentation/driver-api/index.rst
Documentation/driver-api/driver-model/
Documentation/driver-api/device-io.rst
Documentation/driver-api/firmware/
Documentation/driver-api/pm/
```

Then read subsystem docs:

```text
Documentation/driver-api/gpio/
Documentation/driver-api/i2c.rst
Documentation/driver-api/spi.rst
Documentation/networking/
Documentation/mmc/
Documentation/driver-api/serial/
Documentation/driver-api/pinctrl.rst
Documentation/power/
```

What to learn:

- device/driver/bus model
- probe/remove lifecycle
- platform devices
- resources from device tree
- managed resource helpers
- runtime power management
- subsystem-specific expectations

This phase connects DTS changes to real driver behavior.

## Phase 7: Userspace API And ABI

Read:

```text
Documentation/userspace-api/index.rst
Documentation/ABI/
Documentation/admin-guide/abi.rst
```

What to learn:

- what kernel interfaces userspace can rely on
- sysfs ABI
- ioctl/user header implications
- difference between internal kernel APIs and exported UAPI

Embedded focus:

- product software should depend on stable userspace ABI, not internal kernel headers
- custom drivers need deliberate userspace interface design

## Phase 8: Boot, Initramfs, And Runtime Administration

Read:

```text
Documentation/admin-guide/index.rst
Documentation/admin-guide/kernel-parameters.rst
Documentation/admin-guide/initrd.rst
Documentation/filesystems/ramfs-rootfs-initramfs.rst
```

What to learn:

- kernel command line
- initramfs/initrd behavior
- root filesystem handoff
- boot-time debugging parameters
- runtime module and device behavior

Embedded focus:

- rootfs mount failures often come from config/module/initramfs mismatch
- `root=`, `rootwait`, console, and early debug parameters are part of the build/deploy contract

## Phase 9: Debugging And Tracing

Read:

```text
Documentation/admin-guide/bug-hunting.rst
Documentation/admin-guide/dynamic-debug-howto.rst
Documentation/trace/index.rst
Documentation/dev-tools/index.rst
Documentation/dev-tools/kasan.rst
Documentation/dev-tools/kcsan.rst
Documentation/dev-tools/ubsan.rst
```

What to learn:

- dynamic debug
- ftrace
- tracepoints
- kernel sanitizers
- oops/panic analysis
- debug config tradeoffs

Embedded focus:

- debug options affect image size, timing, and sometimes boot behavior
- release builds must archive `vmlinux`, `System.map`, and `.config`

## Phase 10: Architecture And SoC-Specific Documentation

Read your architecture docs:

```text
Documentation/arch/arm/
Documentation/arch/arm64/
```

Then read vendor/subsystem docs related to the SoC and board.

For TI Sitara-style work, also inspect:

- TI-specific device tree bindings
- TI networking/PRU/remoteproc bindings where relevant
- remoteproc/rpmsg documentation if using auxiliary cores
- power management documentation relevant to the SoC

The exact docs depend on kernel version and vendor branch.

## Phase 11: Process, Patch, And Maintenance Docs

Read:

```text
Documentation/process/submitting-patches.rst
Documentation/process/coding-style.rst
Documentation/process/maintainer-handbooks.rst
Documentation/process/stable-kernel-rules.rst
Documentation/process/development-process.rst
```

What to learn:

- patch structure
- commit message expectations
- maintainership
- stable backport policy
- why upstreamable and downstream-only patches should be separated

Embedded focus:

- clean patch stacks make vendor BSP upgrades possible
- DTS/config/driver/product-policy changes should not be mixed randomly

## Recommended Reading Order For Board Bring-Up

For a new custom SoC board, use this order:

1. `Documentation/kbuild/`
2. `Documentation/kbuild/kconfig*`
3. relevant `Documentation/devicetree/` basics
4. relevant `Documentation/devicetree/bindings/`
5. relevant `Documentation/driver-api/` subsystem docs
6. `Documentation/admin-guide/kernel-parameters.rst`
7. initramfs/rootfs docs if rootfs boot is involved
8. tracing/debug docs
9. process/patch docs before preserving product changes

## Recommended Reading Order For Kernel Build Mastery

For build-system competence:

1. `Documentation/kbuild/makefiles.rst`
2. `Documentation/kbuild/kbuild.rst`
3. `Documentation/kbuild/kconfig-language.rst`
4. `Documentation/kbuild/modules.rst`
5. `Documentation/kbuild/headers_install.rst`
6. `Documentation/kbuild/reproducible-builds.rst`
7. device tree build and binding docs
8. admin guide boot/runtime docs

## What Good Understanding Looks Like

You understand the kernel build system when you can:

- explain why a source file is or is not compiled
- trace Kconfig to Kbuild to object output
- build kernel image, modules, and DTBs separately
- audit final `.config`
- build and install external modules
- validate DTBs against bindings
- identify the deployed DTB and runtime device tree
- preserve release artifacts for debugging
- rebase product changes across vendor kernel updates

## Related Topics

- [Kconfig and Defconfig](kconfig-and-defconfig.md)
- [Kbuild Objects and Directories](kbuild-objects-and-directories.md)
- [Modules and External Modules](modules-and-external-modules.md)
- [Device Tree Binding Validation](device-tree-binding-validation.md)
- [Vendor Kernel Patch Management](vendor-kernel-patch-management.md)

## References

- Linux kernel `Documentation/` directory
- Linux kernel `Documentation/kbuild/`
- Linux kernel `Documentation/devicetree/`
- Linux kernel `Documentation/driver-api/`

---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# Kernel Source, Build, And Tailoring

This track teaches the source, configuration, build, and deployment knowledge a driver developer needs before kernel code can be tested honestly.

The detailed build-system mechanics live in the [Linux Kernel Build System](../../build-systems/advanced/linux-kernel/index.md) chapter. This track stays closer to the driver-development question:

```text
Which exact kernel am I targeting, what did it build, and what did the board actually boot?
```

## What Problem Does This Solve?

Kernel driver work becomes unreliable when source, configuration, build output, modules, Device Tree blobs, and the running target drift apart. A driver can look broken when the real issue is a stale DTB, the wrong `.config`, a missing module dependency, a different compiler, a vendor patch stack, or a module built against the wrong kernel release.

For driver developers, the kernel build is not background machinery. It determines:

- which APIs and internal data structures are available
- whether a driver is built at all
- whether it is built into the image or as a module
- which probe path is active at runtime
- which symbols external modules can use
- whether module loading is accepted or rejected
- whether the board is using the DTB that describes the hardware you are debugging
- whether crash logs can be symbolized later

The aim is to make every experiment reproducible enough that a failed probe, compile error, boot hang, or `modprobe` failure points to the real layer.

## Learning Materials

1. [Kernel Source Acquisition](kernel-source-acquisition.md)
2. [Kernel Configuration And Tailoring](kernel-configuration-and-tailoring.md)
3. [Kernel Build And Install Overview](kernel-build-and-install-overview.md)

Read them in order if you are new to kernel builds. If you are debugging a specific problem, use this map:

| Problem | Start Here |
| --- | --- |
| You do not know which source tree matches the board | [Kernel Source Acquisition](kernel-source-acquisition.md) |
| A driver source file exists but no `.ko` or built-in object appears | [Kernel Configuration And Tailoring](kernel-configuration-and-tailoring.md) |
| A module says `Invalid module format` | [Kernel Build And Install Overview](kernel-build-and-install-overview.md) |
| A Device Tree edit has no effect | [Kernel Build And Install Overview](kernel-build-and-install-overview.md) |
| A config fragment requests an option but final `.config` lacks it | [Kernel Configuration And Tailoring](kernel-configuration-and-tailoring.md) |
| An external module builds but reports unknown symbols | [Kernel Source Acquisition](kernel-source-acquisition.md), then [Kernel Build And Install Overview](kernel-build-and-install-overview.md) |

## Core Concepts

- upstream kernel source
- stable and longterm kernel branches
- vendor BSP kernel source
- distro kernel source and headers
- source provenance
- patch stack
- `.config`
- defconfig
- Kconfig symbol
- Kbuild object selection
- built-in driver
- loadable module
- generated headers
- `Module.symvers`
- `vmlinux`
- `System.map`
- kernel image
- DTB and DTBO
- `ARCH`
- `CROSS_COMPILE`
- `O=`
- `modules_install`
- rootfs staging
- artifact manifest

## Mental Model

A kernel build is a chain of identities:

```text
source tree + patch stack
-> configuration
-> generated build tree
-> kernel image, DTBs, modules, debug artifacts
-> deployed boot partition and rootfs
-> running kernel on target
```

Driver debugging is trustworthy only when those identities line up.

For example, this is coherent:

```text
source commit: 3f02c1d...
patch stack: vendor-bsp-2026.03 + product-gpio-fix.patch
config: final build/.config
compiler: aarch64-linux-gnu-gcc 13.2.0
kernel release: 6.6.32-product-g3f02c1d
boot artifacts: Image + board.dtb
rootfs artifacts: /lib/modules/6.6.32-product-g3f02c1d/
debug artifacts: vmlinux + System.map + Module.symvers
target: uname -r reports 6.6.32-product-g3f02c1d
```

This is not coherent:

```text
kernel image copied from today's build
DTB copied from last week's build
modules still in rootfs from vendor image
external module built against distro headers
source tree inspected from upstream mainline
```

The second setup can produce valid-looking symptoms that point in the wrong direction.

## The Build Identity Checklist

When you touch kernel source or a driver, record these values:

| Identity Item | Example Command |
| --- | --- |
| running kernel release | `uname -r` |
| running kernel version string | `cat /proc/version` |
| kernel command line | `cat /proc/cmdline` |
| source commit | `git rev-parse HEAD` |
| source tag or describe string | `git describe --tags --dirty --always` |
| local patch state | `git status --short` |
| build output directory | `realpath build-arm64` |
| final config | `grep '^CONFIG_LOCALVERSION' build-arm64/.config` |
| kernel release from build tree | `make O=build-arm64 ARCH=arm64 kernelrelease` |
| module metadata | `modinfo ./driver.ko` |
| deployed module directory | `ls /lib/modules/$(uname -r)` |
| runtime Device Tree model | `tr -d '\0' < /proc/device-tree/model` |

The exact commands vary between a normal distro, a vendor SDK, Yocto, Buildroot, and a hand-built kernel, but the same identity questions remain.

## Artifact Groups

Keep the following groups separate in your head and in your release directories.

| Group | Examples | Why It Matters |
| --- | --- | --- |
| Source inputs | kernel git commit, patches, DTS, Kconfig, Makefiles | These explain what could be built. |
| Configuration inputs | defconfig, fragments, command-line selections | These express what you requested. |
| Resolved configuration | final `.config`, `include/generated/autoconf.h` | This is what Kbuild actually used. |
| Build outputs for boot | `Image`, `zImage`, `bzImage`, `fitImage`, `*.dtb`, `*.dtbo` | These are deployed to boot media. |
| Build outputs for runtime | `*.ko`, `/lib/modules/<release>/`, firmware | These must match the running kernel. |
| Debug outputs | `vmlinux`, `System.map`, `Module.symvers`, build logs | These make later debugging possible. |
| Runtime evidence | `uname -r`, `/proc/version`, `/proc/cmdline`, `/proc/device-tree` | These prove what actually booted. |

## Recommended Driver Developer Workflow

Use this loop for ordinary driver work:

```text
identify target kernel
-> acquire or locate matching source
-> configure from board baseline
-> enable only the needed driver/debug options
-> build kernel image, DTBs, modules
-> install modules into a staging rootfs
-> deploy image + DTB + modules together
-> boot target
-> verify runtime identity
-> test driver
-> archive artifacts if the result matters
```

Minimal example:

```sh
make O=build-arm64 ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- vendor_board_defconfig
make O=build-arm64 ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- menuconfig
make O=build-arm64 ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- -j$(nproc) Image dtbs modules
make O=build-arm64 ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- \
  INSTALL_MOD_PATH=$PWD/rootfs-staging \
  modules_install
make O=build-arm64 ARCH=arm64 kernelrelease
```

Then on the target:

```sh
uname -r
cat /proc/version
ls /lib/modules/$(uname -r)
tr -d '\0' < /proc/device-tree/model
```

## Example Artifact Manifest

For any result worth keeping, create a small manifest:

```text
board: example-a53-board
purpose: test i2c temperature sensor driver probe
kernel_source: git@example.com/vendor/linux.git
kernel_commit: 3f02c1d9c8a4
kernel_describe: v6.6.32-product-17-g3f02c1d-dirty
patches: product/i2c-sensor-bringup.patch
config_base: arch/arm64/configs/vendor_board_defconfig
config_fragments: fragments/debug.config fragments/i2c-sensor.config
output_dir: build-arm64
kernel_release: 6.6.32-product-g3f02c1d
compiler: aarch64-linux-gnu-gcc 13.2.0
boot_image: deploy/boot/Image
dtb: deploy/boot/example-a53-board.dtb
modules: deploy/rootfs/lib/modules/6.6.32-product-g3f02c1d/
debug_files: deploy/debug/vmlinux deploy/debug/System.map deploy/debug/Module.symvers
target_uname_r: 6.6.32-product-g3f02c1d
```

This looks bureaucratic until the first time a board boots an old DTB while your source tree contains the new one.

## Boundaries With The Build-System Chapter

This chapter explains what a driver developer must understand and check. The build-system chapter goes deeper into Kbuild internals and build framework integration:

- [Kernel Source Tree and Outputs](../../build-systems/advanced/linux-kernel/source-tree-and-outputs.md)
- [Kbuild Objects and Directories](../../build-systems/advanced/linux-kernel/kbuild-objects-and-directories.md)
- [Kconfig and Defconfig](../../build-systems/advanced/linux-kernel/kconfig-and-defconfig.md)
- [Configuration Fragments and Auditing](../../build-systems/advanced/linux-kernel/configuration-fragments-and-auditing.md)
- [Cross-Building and Installing](../../build-systems/advanced/linux-kernel/cross-building-and-installing.md)
- [Device Tree Builds](../../build-systems/advanced/linux-kernel/device-tree-builds.md)
- [Modules and External Modules](../../build-systems/advanced/linux-kernel/modules-and-external-modules.md)
- [Kernel Release Artifacts](../../build-systems/advanced/linux-kernel/kernel-release-artifacts.md)

## Completion Criteria

You are ready to move on when you can:

- identify the source tree and commit used for a running target
- explain why installed kernel headers are not always enough for embedded driver work
- tell whether a driver is built in, modular, or absent
- find the Kconfig symbol and Kbuild rule that select a driver source file
- produce a final `.config` from a board defconfig and small fragments
- build kernel image, DTBs, and modules in an `O=` output directory
- install modules into a staging rootfs instead of the host rootfs
- build an external module against a matching prepared kernel build tree
- compare `make kernelrelease`, `uname -r`, and module `vermagic`
- prove that the target booted the intended kernel image and Device Tree
- archive `vmlinux`, `System.map`, `.config`, and `Module.symvers` for debug

## Common Mistakes

- Reading upstream source while the board runs a heavily patched vendor kernel.
- Building a module against `/lib/modules/$(uname -r)/build` on the host when the target uses another kernel.
- Editing `.config` directly and losing the reproducible source of the change.
- Treating a config fragment as proof that the final `.config` contains the requested value.
- Copying `Image` but not the matching DTB.
- Copying modules but forgetting to run `depmod` for the target rootfs.
- Testing an external module without checking `vermagic`.
- Deleting `vmlinux`, `System.map`, or `Module.symvers` after a meaningful build.
- Reusing one output directory for multiple architectures.
- Assuming a build command updated the board when it only produced local artifacts.

## Official References

- [Linux Kernel Makefiles](https://docs.kernel.org/kbuild/makefiles.html)
- [Kbuild](https://docs.kernel.org/kbuild/kbuild.html)
- [Kconfig Language](https://docs.kernel.org/kbuild/kconfig-language.html)
- [Configuration targets and editors](https://docs.kernel.org/kbuild/kconfig.html)
- [Building External Modules](https://docs.kernel.org/kbuild/modules.html)
- [Linux and the Devicetree](https://docs.kernel.org/devicetree/usage-model.html)

## Related Topics

- [Kernel Foundations For Driver Developers](../foundations/index.md)
- [Reading Kernel Source](../foundations/reading-kernel-source.md)
- [Linux Kernel Build System](../../build-systems/advanced/linux-kernel/index.md)
- [Kernel Configuration And Platform Policy](../configuration-and-platform-policy/index.md)

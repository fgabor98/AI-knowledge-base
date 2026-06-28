---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# Kernel Build And Install Overview

## What Problem Does This Solve?

Driver development requires enough build fluency to produce, install, deploy, and verify a coherent set of kernel artifacts:

```text
kernel image
DTB/DTBO files
loadable modules
module dependency metadata
optional firmware
debug artifacts
```

A driver test is not trustworthy if the board boots a new kernel image with old modules, a new module against an old kernel, or a new Device Tree source that was never deployed.

This page gives a practical workflow. For deeper Kbuild internals, use the linked build-system pages.

## Core Concepts

- source tree
- output directory
- in-tree build
- out-of-tree output with `O=`
- `ARCH`
- `CROSS_COMPILE`
- `LLVM=1`
- host tools
- kernel image
- `vmlinux`
- `System.map`
- DTB
- DTBO
- modules
- `Module.symvers`
- `modules_install`
- `INSTALL_MOD_PATH`
- `INSTALL_MOD_STRIP`
- `INSTALL_HDR_PATH`
- external modules
- staging root filesystem
- boot partition
- initramfs
- kernel release
- module `vermagic`

## Mental Model

Build and deployment are separate.

```text
configure kernel
-> build image, DTBs, modules
-> install modules into a staging rootfs
-> copy boot artifacts to the boot location
-> assemble image or update board
-> boot target
-> verify runtime identity
```

Running `make` does not automatically update the board unless a higher-level build framework or custom script does that. Always know where artifacts were produced and where the target loads them from.

## Source Tree And Output Tree

The source tree contains maintained inputs:

```text
arch/
drivers/
include/
kernel/
mm/
net/
scripts/
Documentation/
```

The output tree contains generated state and build products:

```text
build-arm64/
  .config
  include/generated/
  arch/arm64/boot/Image
  arch/arm64/boot/dts/
  Module.symvers
  System.map
  vmlinux
```

Prefer `O=` output directories:

```sh
make O=build-arm64 ARCH=arm64 vendor_board_defconfig
make O=build-arm64 ARCH=arm64 -j$(nproc) Image modules dtbs
```

Benefits:

- keeps generated files out of the source tree
- allows multiple board or architecture builds from one source checkout
- makes build identity easier to inspect
- reduces accidental reuse of stale generated files

Avoid using one output directory for different architectures or unrelated board configurations.

## Native Build

For a host kernel experiment:

```sh
make O=build-x86_64 defconfig
make O=build-x86_64 -j$(nproc)
```

For modules only:

```sh
make O=build-x86_64 -j$(nproc) modules
```

This is useful for learning. It is not the same as building for an embedded target.

## Cross Build

Embedded targets are usually cross-built.

Example for 64-bit Arm:

```sh
make O=build-arm64 ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- vendor_board_defconfig
make O=build-arm64 ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- -j$(nproc) Image dtbs modules
```

Example for 32-bit Arm:

```sh
make O=build-arm ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- vendor_board_defconfig
make O=build-arm ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- -j$(nproc) zImage dtbs modules
```

Example for RISC-V:

```sh
make O=build-riscv ARCH=riscv CROSS_COMPILE=riscv64-linux-gnu- defconfig
make O=build-riscv ARCH=riscv CROSS_COMPILE=riscv64-linux-gnu- -j$(nproc) Image dtbs modules
```

Check what compiler is actually used:

```sh
make O=build-arm64 ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- V=1 Image
```

If the command uses the wrong compiler prefix, fix `CROSS_COMPILE` or the build environment before debugging C errors.

## LLVM Builds

Some kernels and products build with LLVM/Clang:

```sh
make O=build-arm64 ARCH=arm64 LLVM=1 defconfig
make O=build-arm64 ARCH=arm64 LLVM=1 -j$(nproc) Image dtbs modules
```

Do not switch compiler families casually for a product kernel. Compiler choice can affect warnings, generated code, module metadata, debug info, and reproducibility. Match the product build unless the experiment is specifically about changing toolchains.

## Common Build Targets

| Target | Purpose |
| --- | --- |
| `defconfig` | Generate a baseline `.config`. |
| `<board>_defconfig` | Generate a board/vendor baseline `.config`. |
| `olddefconfig` | Refresh config using defaults for new options. |
| `menuconfig` | Interactive config editor. |
| `Image` | Common uncompressed Arm/Arm64 boot image. |
| `zImage` | Common compressed 32-bit Arm boot image. |
| `bzImage` | Common x86 boot image. |
| `vmlinux` | Linked ELF kernel, useful for debugging. |
| `dtbs` | Build Device Tree blobs. |
| `dtbs_check` | Run Device Tree schema checks where available. |
| `modules` | Build loadable modules. |
| `modules_install` | Install modules under a rootfs staging path. |
| `headers_install` | Install UAPI headers for userspace builds. |
| `kernelrelease` | Print the release string used for module paths. |
| `clean` | Remove many generated files. |
| `mrproper` | More aggressive cleanup, including config. |

Use `clean` and `mrproper` carefully in a shared or valuable build directory. They remove generated state you may still need for debugging.

## First Full Build Workflow

A typical driver-development kernel build:

```sh
make O=build-arm64 ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- vendor_board_defconfig
make O=build-arm64 ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- menuconfig
make O=build-arm64 ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- olddefconfig
make O=build-arm64 ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- -j$(nproc) Image dtbs modules
make O=build-arm64 ARCH=arm64 kernelrelease
```

Find outputs:

```sh
find build-arm64 -name Image -o -name '*.dtb' -o -name '*.ko'
ls -l build-arm64/vmlinux build-arm64/System.map build-arm64/Module.symvers
```

Expected artifact groups:

```text
boot:
  build-arm64/arch/arm64/boot/Image
  build-arm64/arch/arm64/boot/dts/.../*.dtb

rootfs:
  modules installed under /lib/modules/<kernelrelease>/

debug:
  build-arm64/vmlinux
  build-arm64/System.map
  build-arm64/.config
  build-arm64/Module.symvers
```

## Installing Modules Into A Staging Rootfs

Do not install target modules into the host root filesystem by accident. Use `INSTALL_MOD_PATH`.

```sh
make O=build-arm64 ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- \
  INSTALL_MOD_PATH=$PWD/rootfs-staging \
  modules_install
```

This creates:

```text
rootfs-staging/
  lib/modules/<kernelrelease>/
    kernel/
    modules.alias
    modules.dep
    modules.symbols
    modules.order
```

Check:

```sh
make O=build-arm64 ARCH=arm64 kernelrelease
find rootfs-staging/lib/modules -maxdepth 2 -type f | head
```

If you need to regenerate dependency metadata manually for a target rootfs:

```sh
depmod -b rootfs-staging <kernelrelease>
```

Strip installed modules when appropriate:

```sh
make O=build-arm64 ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- \
  INSTALL_MOD_PATH=$PWD/rootfs-staging \
  INSTALL_MOD_STRIP=1 \
  modules_install
```

Keep unstripped debug artifacts separately if you need symbolized debugging.

## Installing UAPI Headers

Userspace programs should not include arbitrary internal kernel headers. If you need kernel UAPI headers for userspace builds:

```sh
make O=build-arm64 ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- \
  INSTALL_HDR_PATH=$PWD/headers-staging \
  headers_install
```

This installs sanitized UAPI headers, not a full kernel development tree.

## Boot Artifact Deployment

Different boards load different boot artifact names.

Common layouts:

```text
/boot/Image
/boot/zImage
/boot/bzImage
/boot/fitImage
/boot/board.dtb
/boot/overlays/*.dtbo
/boot/extlinux/extlinux.conf
```

A manual staging layout:

```sh
mkdir -p deploy/boot deploy/rootfs deploy/debug
cp build-arm64/arch/arm64/boot/Image deploy/boot/
cp build-arm64/arch/arm64/boot/dts/vendor/example-board.dtb deploy/boot/
cp build-arm64/vmlinux build-arm64/System.map build-arm64/.config build-arm64/Module.symvers deploy/debug/
```

Then install modules:

```sh
make O=build-arm64 ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- \
  INSTALL_MOD_PATH=$PWD/deploy/rootfs \
  modules_install
```

The exact copy destination may be:

- a removable boot partition
- a TFTP directory
- a Yocto deploy directory
- a Buildroot `output/images/` directory
- an initramfs staging directory
- a packaging directory
- a board update bundle

The important rule: deploy the kernel image, DTB, and modules from the same build identity.

## Device Tree Build And Deploy Checks

Build DTBs:

```sh
make O=build-arm64 ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- dtbs
```

Find the generated DTB:

```sh
find build-arm64/arch/arm64/boot/dts -name '*example*.dtb'
```

Optionally decompile it for inspection:

```sh
dtc -I dtb -O dts deploy/boot/example-board.dtb > /tmp/example-board.dts
```

On the target:

```sh
tr -d '\0' < /proc/device-tree/model
find /proc/device-tree -maxdepth 3 -name compatible -print
cat /proc/cmdline
```

If a DTS edit does not affect runtime, check:

- Did you rebuild `dtbs`?
- Did you copy the generated DTB?
- Does the bootloader load that filename?
- Does a FIT image contain another DTB?
- Are overlays replacing or modifying the node?
- Does U-Boot use a separate DTB workflow?

## External Module Build

External modules are built outside the kernel source tree but still use the kernel build system.

Minimal module source:

```c
#include <linux/init.h>
#include <linux/module.h>

static int __init hello_init(void)
{
    pr_info("hello module loaded\n");
    return 0;
}

static void __exit hello_exit(void)
{
    pr_info("hello module unloaded\n");
}

module_init(hello_init);
module_exit(hello_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Minimal external module example");
MODULE_AUTHOR("Example");
```

Minimal Makefile:

```make
obj-m += hello.o

KDIR ?= /lib/modules/$(shell uname -r)/build

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
```

Build for the host:

```sh
make
modinfo hello.ko
```

Build for an embedded target using a prepared build tree:

```sh
make -C /path/to/linux-source \
  O=/path/to/build-arm64 \
  ARCH=arm64 \
  CROSS_COMPILE=aarch64-linux-gnu- \
  M=$PWD \
  modules
```

or, when your prepared build directory supports being passed directly:

```sh
make -C /path/to/build-arm64 \
  ARCH=arm64 \
  CROSS_COMPILE=aarch64-linux-gnu- \
  M=$PWD \
  modules
```

Verify:

```sh
modinfo hello.ko | grep -E 'filename|vermagic|depends|srcversion'
make -C /path/to/linux-source O=/path/to/build-arm64 ARCH=arm64 kernelrelease
```

Before loading on target:

```sh
uname -r
modinfo /tmp/hello.ko | grep vermagic
```

If the release strings do not match, stop and fix the build identity.

## External Module Install

Install an external module into a staging rootfs:

```sh
make -C /path/to/linux-source \
  O=/path/to/build-arm64 \
  ARCH=arm64 \
  CROSS_COMPILE=aarch64-linux-gnu- \
  M=$PWD \
  INSTALL_MOD_PATH=$PWD/rootfs-staging \
  modules_install
```

Then update dependencies:

```sh
depmod -b rootfs-staging <kernelrelease>
```

On target, prefer `modprobe` for normal dependency handling:

```sh
modprobe hello
```

Use `insmod` for direct experiments:

```sh
insmod /tmp/hello.ko
rmmod hello
dmesg | tail -n 50
```

`insmod` does not resolve dependencies automatically. If a module depends on another module, `modprobe` is normally the better test.

## Module Compatibility Checks

Check the running kernel:

```sh
uname -r
cat /proc/version
```

Check the module:

```sh
modinfo ./hello.ko
modinfo ./hello.ko | grep vermagic
```

Check the build tree:

```sh
make O=build-arm64 ARCH=arm64 kernelrelease
grep '^CONFIG_MODVERSIONS' build-arm64/.config
test -f build-arm64/Module.symvers
```

Common interpretations:

| Evidence | Meaning |
| --- | --- |
| `vermagic` release differs from `uname -r` | Built against a different kernel release. |
| architecture differs | Built for the wrong target. |
| `Unknown symbol` in `dmesg` | Missing dependency, missing export, or symbol version mismatch. |
| module path under wrong `/lib/modules/` directory | Installed to the wrong rootfs or kernel release. |
| `modprobe` cannot find module | `modules_install` or `depmod` did not run for the target rootfs. |
| module signing error | Kernel enforces signature policy and module is unsigned or signed by wrong key. |

## Build Output Inspection

Useful commands:

```sh
make O=build-arm64 ARCH=arm64 kernelrelease
grep '^CONFIG_LOCALVERSION' build-arm64/.config
find build-arm64 -name '*.ko' | sort
find build-arm64 -name '*.dtb' | sort
ls -lh build-arm64/vmlinux build-arm64/System.map build-arm64/Module.symvers
```

Inspect an object build command:

```sh
make O=build-arm64 ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- V=1 drivers/i2c/busses/
```

Search for a missing module:

```sh
grep '^CONFIG_EXAMPLE_DRIVER' build-arm64/.config
rg "CONFIG_EXAMPLE_DRIVER|example_driver" drivers
find build-arm64 -name '*example*'
```

## Runtime Verification After Boot

On target:

```sh
uname -r
cat /proc/version
cat /proc/cmdline
ls /lib/modules/$(uname -r)
tr -d '\0' < /proc/device-tree/model
dmesg | head -n 30
```

For modules:

```sh
modprobe -v my_driver
lsmod | grep my_driver
dmesg | tail -n 100
```

For built-in drivers:

```sh
dmesg | grep -i my_driver
find /sys/bus -name '*device_or_driver_hint*' 2>/dev/null
```

For Device Tree:

```sh
find /proc/device-tree -maxdepth 4 -name compatible -print
find /proc/device-tree -maxdepth 4 -name status -print
```

Runtime verification closes the loop. Without it, you only know that artifacts were built, not that the target used them.

## Common Build And Install Scenarios

### Scenario: Build Succeeds But Module Missing

Checks:

```sh
grep '^CONFIG_MY_DRIVER' build-arm64/.config
rg "MY_DRIVER|my_driver" drivers
find build-arm64 -name '*my*driver*.ko'
```

Likely explanations:

- driver is built in with `CONFIG_MY_DRIVER=y`
- driver is unset
- Kbuild object name differs from source name
- module target was not built
- inspecting the wrong `O=` directory

### Scenario: Board Boots Old Kernel

Checks on target:

```sh
uname -r
cat /proc/version
cat /proc/cmdline
```

Checks on boot media:

```sh
sha256sum deploy/boot/Image
sha256sum /path/to/boot/partition/Image
```

Likely explanations:

- copied artifact to wrong partition
- bootloader loads from TFTP or another device
- FIT image still contains old kernel
- bootloader environment points to another filename
- A/B update slot booted the previous slot

### Scenario: Device Tree Edit Has No Effect

Checks:

```sh
make O=build-arm64 ARCH=arm64 dtbs
find build-arm64 -name '*board*.dtb'
sha256sum build-arm64/arch/arm64/boot/dts/vendor/board.dtb
tr -d '\0' < /proc/device-tree/model
```

Likely explanations:

- old DTB deployed
- wrong board DTB edited
- included `.dtsi` overridden later
- overlay changes runtime tree
- U-Boot loads another DTB path
- FIT image needs regeneration

### Scenario: `Invalid module format`

Checks:

```sh
uname -r
modinfo ./my_driver.ko | grep vermagic
dmesg | tail -n 50
```

Likely explanations:

- wrong kernel release
- wrong architecture
- different preemption/SMP/module configuration
- module versioning mismatch
- compiler/toolchain mismatch
- module signing enforcement

### Scenario: `Unknown symbol`

Checks:

```sh
dmesg | grep -i 'unknown symbol'
modinfo ./my_driver.ko | grep depends
grep 'symbol_name' build-arm64/Module.symvers
```

Likely explanations:

- dependency module not loaded
- required feature built out
- symbol not exported
- wrong `Module.symvers`
- vendor kernel lacks a symbol present upstream

### Scenario: Build Uses Wrong Architecture

Checks:

```sh
file build-arm64/arch/arm64/boot/Image
make O=build-arm64 ARCH=arm64 kernelrelease
```

Likely explanations:

- forgot `ARCH`
- reused output dir from another architecture
- build framework set environment variables differently
- external module Makefile defaulted to host `/lib/modules`

## Staging Layout For Driver Development

Use a predictable local layout:

```text
work/
  linux/
  build-arm64/
  fragments/
  deploy/
    boot/
      Image
      board.dtb
    rootfs/
      lib/modules/<kernelrelease>/
    debug/
      vmlinux
      System.map
      .config
      Module.symvers
    logs/
      build.log
      install.log
```

This layout makes it clear which files go to the target and which files stay on the development machine for analysis.

## Release Artifacts To Keep

For any kernel build used by other people, archived in CI, or tested meaningfully, keep:

- kernel image actually deployed
- DTB/DTBO or FIT image actually deployed
- loadable modules
- final `.config`
- `Module.symvers`
- `vmlinux`
- `System.map`
- build log
- source commit and patch list
- compiler version
- artifact checksums
- target runtime evidence if available

Losing `vmlinux` or `System.map` makes later crash analysis much harder. Losing `Module.symvers` makes external module rebuilds less reliable.

## Common Mistakes

- Running `make modules_install` without `INSTALL_MOD_PATH` and touching the host module tree.
- Copying `Image` but leaving old modules in the rootfs.
- Copying modules but not running dependency generation for the target rootfs.
- Editing DTS, building kernel image, but forgetting `dtbs`.
- Building an external module against host headers for an embedded target.
- Ignoring `make kernelrelease`.
- Reusing an `O=` directory across architectures.
- Deleting `Module.symvers` before external modules are built.
- Assuming `insmod` resolves dependencies.
- Debugging a driver before proving the target booted the intended kernel and DTB.

## Practical Checklist

Before deployment:

- Confirm source commit and patch state.
- Confirm `ARCH`, `CROSS_COMPILE`, and compiler version.
- Confirm output directory.
- Confirm final `.config`.
- Run `make kernelrelease`.
- Build kernel image, DTBs, and modules.
- Install modules into a staging rootfs.
- Preserve `vmlinux`, `System.map`, `.config`, and `Module.symvers`.
- Copy kernel image and DTB from the same build.
- Record checksums.

After boot:

- Check `uname -r`.
- Check `/proc/version`.
- Check `/proc/cmdline`.
- Check `/lib/modules/$(uname -r)`.
- Check runtime Device Tree.
- Check module `vermagic` if loading modules.
- Check `dmesg` for probe, symbol, and module errors.

## Practice Exercises

### Exercise 1: Build And Inspect Artifacts

```sh
make O=build-arm64 ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- defconfig
make O=build-arm64 ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- -j$(nproc) Image dtbs modules
make O=build-arm64 ARCH=arm64 kernelrelease
find build-arm64 -name Image -o -name '*.dtb' -o -name '*.ko'
```

Answer:

- Where is the kernel image?
- Where are DTBs?
- Where are modules?
- What is the kernel release?

### Exercise 2: Install Modules Into Staging

```sh
make O=build-arm64 ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- \
  INSTALL_MOD_PATH=$PWD/rootfs-staging \
  modules_install
find rootfs-staging/lib/modules -maxdepth 3 -type f | head
```

Answer:

- Which directory was created under `lib/modules/`?
- Does it match `make kernelrelease`?

### Exercise 3: Build An External Module

Build a simple `hello.ko` against your prepared build tree:

```sh
make -C /path/to/linux-source O=/path/to/build-arm64 ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- M=$PWD modules
modinfo hello.ko | grep vermagic
```

Answer:

- Does `vermagic` match the target kernel release?
- Is `Module.symvers` present in the kernel build tree?

### Exercise 4: Prove Runtime Identity

After deploying a kernel:

```sh
uname -r
cat /proc/version
ls /lib/modules/$(uname -r)
tr -d '\0' < /proc/device-tree/model
```

Answer:

- Did the target boot the kernel you built?
- Does the module directory match?
- Does the runtime Device Tree model match your intended board?

## Related Topics

- [Kernel Source, Build, And Tailoring](index.md)
- [Kernel Source Tree and Outputs](../../build-systems/advanced/linux-kernel/source-tree-and-outputs.md)
- [Cross-Building and Installing](../../build-systems/advanced/linux-kernel/cross-building-and-installing.md)
- [Device Tree Builds](../../build-systems/advanced/linux-kernel/device-tree-builds.md)
- [Modules and External Modules](../../build-systems/advanced/linux-kernel/modules-and-external-modules.md)
- [Kernel Release Artifacts](../../build-systems/advanced/linux-kernel/kernel-release-artifacts.md)
- [Module Signing And Hardening](../configuration-and-platform-policy/module-signing-and-hardening.md)

## Official References

- [Kbuild](https://docs.kernel.org/kbuild/kbuild.html)
- [Linux Kernel Makefiles](https://docs.kernel.org/kbuild/makefiles.html)
- [Building External Modules](https://docs.kernel.org/kbuild/modules.html)
- [Linux and the Devicetree](https://docs.kernel.org/devicetree/usage-model.html)

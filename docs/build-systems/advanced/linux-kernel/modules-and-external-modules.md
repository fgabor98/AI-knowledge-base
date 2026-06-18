---
status: draft
reviewed: false
domain: build-systems
difficulty: advanced
reviewer: null
last_reviewed: null
---

# Modules and External Modules

## What Problem Does This Solve?

Kernel modules let drivers and features be loaded separately from the main kernel image. External modules let you build module code outside the kernel source tree while still using the kernel build system.

For embedded Linux, module handling matters for driver development, product packaging, rootfs integration, and field debugging.

## Core Concepts

- built-in driver
- loadable module
- `*.ko`
- `obj-m`
- `M=`
- kernel build directory
- `Module.symvers`
- vermagic
- symbol versions
- `modules_install`
- `depmod`
- `/lib/modules/<kernel-version>/`

## Mental Model

A module must match the kernel it is loaded into:

```text
kernel source/config/build
-> module build
-> module install into matching rootfs
-> depmod metadata
-> modprobe/insmod on target
```

If any part is from a different kernel build, module loading can fail.

## Syntax / API / Mechanism

In-tree module selection:

```make
obj-$(CONFIG_MY_DRIVER) += my_driver.o
```

External module Makefile:

```make
obj-m += my_driver.o

KDIR ?= /lib/modules/$(shell uname -r)/build

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
```

Build:

```sh
make -C /path/to/kernel/build M=$PWD modules
```

Cross-build:

```sh
make -C /path/to/kernel/build \
  ARCH=arm \
  CROSS_COMPILE=arm-linux-gnueabihf- \
  M=$PWD modules
```

Install modules:

```sh
make -C /path/to/kernel/build \
  INSTALL_MOD_PATH=/tmp/rootfs \
  modules_install
```

## Minimal Example

External module source:

```c
#include <linux/module.h>
#include <linux/init.h>

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
```

Makefile:

```make
obj-m += hello.o
```

Build:

```sh
make -C /path/to/kernel/build M=$PWD modules
```

Inspect:

```sh
modinfo hello.ko
```

## In-Tree vs External Module

In-tree:

- source lives in kernel tree
- selected by kernel Kconfig/Kbuild
- built with kernel build
- easier for BSP-integrated drivers

External:

- source lives outside kernel tree
- built against a prepared kernel build directory
- useful for vendor drops or development
- packaging must keep module/kernel version aligned

For long-term product maintenance, consider whether external module code should be upstreamed, moved into the kernel tree, or packaged clearly.

## Module Install Layout

Runtime path:

```text
/lib/modules/<kernel-release>/
```

Check kernel release:

```sh
make -C /path/to/kernel/build kernelrelease
uname -r
```

These must match on target for normal module lookup.

Run `depmod` after installing modules into a rootfs:

```sh
depmod -b /tmp/rootfs <kernel-release>
```

Build systems often do this for you. Know where it happens.

## Common Scenarios

### `Invalid module format`

Likely causes:

- module built for different kernel release
- different config options
- different compiler/toolchain expectations
- symbol version mismatch

Check:

```sh
modinfo my_driver.ko
uname -r
dmesg | tail
```

Look at `vermagic`.

### Unknown Symbol

Likely causes:

- dependency module not loaded
- missing exported symbol
- `Module.symvers` missing or wrong during external build
- feature not enabled in kernel

Check:

```sh
dmesg | grep -i 'unknown symbol'
grep symbol Module.symvers
```

### Module Exists But `modprobe` Cannot Find It

Likely causes:

- module installed under wrong kernel version directory
- `depmod` not run
- rootfs did not include module
- package installed to wrong path

Check:

```sh
find /lib/modules -name 'my_driver.ko'
uname -r
modprobe -v my_driver
```

### Boot-Critical Driver Built As Module

If the rootfs storage driver is a module but no initramfs loads it, the kernel cannot mount rootfs. Build boot-critical drivers into the kernel.

## Expansion: Module Compatibility Diagnostics

A module is compatible with a kernel only when its build inputs match the running kernel closely enough. The most common embedded failure is mixing a new rootfs module package with an old boot partition kernel, or mixing an old rootfs with a new kernel image.

Check the running kernel:

```sh
uname -r
cat /proc/version
```

Check the module:

```sh
modinfo my_driver.ko
modinfo my_driver.ko | grep vermagic
```

Check the build tree:

```sh
make O=build ARCH=arm64 kernelrelease
grep '^CONFIG_MODVERSIONS' build/.config
test -f build/Module.symvers
```

Typical interpretations:

- `vermagic` differs from `uname -r`: the module was built against a different kernel release.
- `Unknown symbol`: the dependency module is missing, a symbol is not exported, or `Module.symvers` did not match.
- `Invalid module format`: kernel release, compiler metadata, module versioning, or architecture mismatch.
- `modprobe` cannot find the module: install path or `depmod` database is wrong.

In Yocto-style systems, also verify that the image includes the module package and that the kernel provider used by the module recipe is the same provider used by `virtual/kernel`.

For release discipline around matching modules with kernel images and DTBs, see [Kernel Release Artifacts](kernel-release-artifacts.md).

## Yocto / Buildroot / TI SDK Integration

Yocto:

- external modules are often recipes inheriting kernel module classes
- module packages must be included in the image
- kernel provider and module recipe must match

Buildroot:

- packages can build kernel modules using Buildroot kernel infrastructure
- install into target rootfs happens through package rules

TI Processor SDK:

- use SDK kernel provider and matching build output
- do not build external modules against a different kernel tree than the image uses

## Common Mistakes

- Building an external module against host `/lib/modules`.
- Building module against kernel headers only when a configured build tree is required.
- Installing modules into rootfs without matching kernel image.
- Forgetting `depmod`.
- Looking for `.ko` when driver is built in.
- Using `insmod` without loading dependency modules.
- Ignoring `dmesg` after module load failure.

## Debugging Checklist

- Check target `uname -r`.
- Check `make kernelrelease` from the kernel build tree.
- Check `modinfo module.ko`.
- Check `vermagic`.
- Check `Module.symvers`.
- Check module install path.
- Run or verify `depmod`.
- Check `dmesg`.
- Confirm module package is included in image.
- Confirm kernel image, rootfs modules, and `Module.symvers` came from the same build.

## Related Topics

- [Kbuild Objects and Directories](kbuild-objects-and-directories.md)
- [Cross-Building and Installing](cross-building-and-installing.md)
- [Debugging Kernel Builds](debugging-kernel-builds.md)
- [Kernel Release Artifacts](kernel-release-artifacts.md)
- [BSP Artifact Flow and Provenance](../bsp-integration/artifact-flow-and-provenance.md)

## References

- Linux kernel external module documentation
- Linux kernel module documentation
- `modinfo(8)`
- `depmod(8)`

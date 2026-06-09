---
status: draft
reviewed: false
domain: build-systems
difficulty: intermediate
reviewer: null
last_reviewed: null
---

# Linux Kernel Build System Roadmap

## What Problem Does This Solve?

The Linux kernel build system is not just a large Makefile. It combines Make, Kbuild files, Kconfig, generated headers, architecture-specific rules, device tree builds, module builds, and install targets.

This roadmap explains what to learn after basic Make and cross-compilation so kernel build commands become understandable instead of memorized.

## Core Concepts

- Kbuild
- Kconfig
- `.config`
- `defconfig`
- `menuconfig`
- `ARCH`
- `CROSS_COMPILE`
- `O=` out-of-tree builds
- `M=` external module builds
- `obj-y` and `obj-m`
- built-in objects and loadable modules
- generated headers
- device tree compilation
- kernel image outputs

## Mental Model

Think of the kernel build as three connected systems:

```text
Kconfig
-> selected configuration symbols
-> .config and generated headers

Kbuild files
-> selected source files and directories
-> built-in objects, modules, images, and device trees

architecture rules
-> target-specific image format and boot artifacts
```

The top-level `Makefile` coordinates the build. Kbuild files describe what should be built. Kconfig describes what can be configured. Architecture-specific directories define how the final bootable image is produced.

## Syntax / API / Mechanism

### Basic Kernel Build

Common native build:

```sh
make defconfig
make -j8
```

Common cross-build:

```sh
make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- defconfig
make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- -j8
```

Out-of-tree build:

```sh
make O=build ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- defconfig
make O=build ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- -j8
```

### Configuration

Common configuration targets:

```sh
make defconfig
make menuconfig
make oldconfig
make savedefconfig
```

Important files:

- `.config` stores the selected configuration.
- `arch/<arch>/configs/*_defconfig` stores minimal board or platform defaults.
- `include/generated/autoconf.h` exposes selected configuration to C code.

### Kbuild Object Selection

Kbuild files select objects based on configuration:

```make
obj-y += core.o
obj-$(CONFIG_MY_DRIVER) += my_driver.o
obj-$(CONFIG_MY_MODULE) += my_module.o
```

Mental model:

- `obj-y` builds code into the kernel image.
- `obj-m` builds loadable kernel modules.
- `obj-$(CONFIG_FOO)` depends on the value selected by Kconfig.

### External Modules

Out-of-tree modules use the kernel build directory:

```make
obj-m += my_driver.o

KDIR ?= /path/to/kernel/build

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
```

Cross-build:

```sh
make -C /path/to/kernel/build \
  ARCH=arm \
  CROSS_COMPILE=arm-linux-gnueabihf- \
  M=$PWD modules
```

### Device Tree Builds

Device trees are usually built through kernel targets:

```sh
make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- dtbs
```

Device tree source files live under architecture-specific directories such as:

```text
arch/arm/boot/dts/
arch/arm64/boot/dts/
```

### Kernel Outputs

Common outputs depend on architecture and configuration:

- `vmlinux`: uncompressed ELF kernel image used for debugging and symbol lookup.
- `Image`: raw kernel image on several architectures.
- `zImage`: compressed ARM kernel image on older ARM systems.
- `uImage`: U-Boot-wrapped image when the build supports it.
- `*.dtb`: compiled device tree blobs.
- `*.ko`: loadable kernel modules.

## Minimal Example

Build an ARM kernel in a separate output directory:

```sh
make O=build-arm ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- defconfig
make O=build-arm ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- -j8
make O=build-arm ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- dtbs
```

Build an external module against that output directory:

```sh
make -C /path/to/linux/build-arm \
  ARCH=arm \
  CROSS_COMPILE=arm-linux-gnueabihf- \
  M=$PWD modules
```

## Real-World Example

A practical learning sequence:

1. Build a kernel with the default configuration for the host.
2. Run `make menuconfig` and inspect how changes affect `.config`.
3. Build with `O=build` and keep source and output directories separate.
4. Cross-build a kernel for a target architecture.
5. Build only device trees with `dtbs`.
6. Find the generated `Image`, `zImage`, `vmlinux`, modules, and DTBs.
7. Build a minimal external module with `M=$PWD`.
8. Add a small Kconfig option and wire it into a Kbuild file.
9. Compare built-in code with module code.
10. Integrate the kernel build into Buildroot or Yocto.

## Common Mistakes

- Building with the wrong `ARCH` value.
- Forgetting the trailing hyphen in `CROSS_COMPILE`.
- Reusing an old `.config` after changing architecture or toolchain.
- Editing generated files instead of source Kconfig or Kbuild files.
- Confusing `obj-y` built-in code with `obj-m` modules.
- Building modules against headers or a build directory that does not match the running target kernel.
- Copying a kernel image without the matching device tree.
- Assuming every architecture produces the same image filenames.

## Debugging Checklist

- Check `ARCH` and `CROSS_COMPILE` first.
- Confirm the compiler shown in verbose build output is the target compiler.
- Use `make V=1` when command lines need inspection.
- Check `.config` for the expected `CONFIG_*` symbols.
- Check whether code is selected by `obj-y`, `obj-m`, or not selected.
- Confirm the output directory passed through `O=` is the one being inspected.
- For modules, compare the target kernel version with `modinfo my_driver.ko`.
- Use `file vmlinux Image zImage` where applicable to inspect outputs.
- Confirm the bootloader is loading the matching kernel image and DTB.

## Learning Path

### Beginner

1. Basic kernel source tree layout
2. Top-level `make` targets
3. `ARCH` and `CROSS_COMPILE`
4. `.config`, `defconfig`, and `menuconfig`
5. Output directories with `O=`

### Intermediate

1. Kbuild object selection
2. Kconfig symbols and dependencies
3. Built-in objects vs modules
4. External modules with `M=`
5. Device tree build targets
6. Kernel image outputs

### Advanced

1. Architecture-specific build rules
2. Generated headers and generated source
3. Module installation and depmod integration
4. Buildroot kernel integration
5. Yocto kernel recipes, fragments, and patches
6. Reproducible kernel builds

## Related Topics

- [Build Systems for Embedded Linux](embedded-linux-roadmap.md)
- [U-Boot Build System Roadmap](u-boot-build-roadmap.md)
- [Build Systems](index.md)
- [Linux Kernel Programming](../linux-kernel/index.md)
- [Embedded Linux](../embedded-linux/index.md)

## References

- Linux kernel documentation
- Linux kernel Kbuild documentation
- Linux kernel Kconfig documentation
- Linux kernel external module documentation
- Linux kernel device tree documentation

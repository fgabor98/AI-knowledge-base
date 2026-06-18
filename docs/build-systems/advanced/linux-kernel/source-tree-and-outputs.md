---
status: draft
reviewed: false
domain: build-systems
difficulty: advanced
reviewer: null
last_reviewed: null
---

# Kernel Source Tree and Outputs

## What Problem Does This Solve?

The Linux kernel source tree is large, and the build produces many different artifacts. Before changing configuration, device trees, modules, or Yocto recipes, you need to know where source inputs live and which outputs matter for an embedded board.

This page explains the kernel tree from a build-system point of view.

## Core Concepts

- top-level `Makefile`
- architecture directory
- Kbuild files
- Kconfig files
- generated headers
- output directory
- `vmlinux`
- `Image`, `zImage`, `bzImage`
- `System.map`
- `Module.symvers`
- `*.ko`
- `*.dtb`

## Mental Model

The kernel build tree has source inputs and generated outputs:

```text
source tree
  arch/
  drivers/
  include/
  scripts/
  tools/

output tree
  .config
  include/generated/
  arch/<arch>/boot/
  modules
  DTBs
```

With in-tree builds, source and output are mixed. With `O=build`, generated output goes into a separate directory.

For embedded work, prefer separate output directories because you often build multiple configurations or architectures from one source tree.

## Syntax / API / Mechanism

Native build:

```sh
make defconfig
make -j8
```

Out-of-tree output:

```sh
make O=build defconfig
make O=build -j8
```

Cross-build:

```sh
make O=build-arm ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- defconfig
make O=build-arm ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- -j8
```

List useful outputs:

```sh
find build-arm -name 'vmlinux' -o -name 'Image' -o -name 'zImage' -o -name '*.dtb' -o -name '*.ko'
```

## Minimal Example

```sh
make O=build ARCH=arm64 defconfig
make O=build ARCH=arm64 -j8
find build -maxdepth 4 -type f \( -name 'vmlinux' -o -name 'Image' -o -name '*.dtb' \)
```

The exact image names depend on architecture and selected targets.

## Source Tree Areas

### `arch/`

Architecture-specific source, configuration, boot image rules, and device trees.

Examples:

```text
arch/arm/
arch/arm64/
arch/riscv/
arch/x86/
```

Important subareas:

```text
arch/<arch>/configs/
arch/<arch>/boot/
arch/<arch>/boot/dts/
```

### `drivers/`

Most hardware drivers live under subsystem directories:

```text
drivers/gpio/
drivers/i2c/
drivers/net/
drivers/pinctrl/
drivers/spi/
drivers/tty/
```

A driver source file existing here does not mean it is built. It must be selected through Kconfig and Kbuild.

### `include/`

Public and internal kernel headers:

```text
include/linux/
include/uapi/
include/generated/
```

`include/generated/` is produced by the build and should not be edited as source.

### `scripts/`

Build helpers:

```text
scripts/kconfig/
scripts/dtc/
scripts/mod/
scripts/Makefile.*
```

These tools are part of why kernel builds have host-tool requirements.

### `tools/`

Host-side tools and utilities, such as `perf` and other buildable tools. These are not the same as target kernel artifacts.

## Important Outputs

### `vmlinux`

`vmlinux` is the linked kernel ELF image. It is not always the file booted by the board, but it is critical for debugging, symbol lookup, and crash analysis.

Archive it for releases.

### Architecture Boot Images

Common names:

```text
Image
zImage
bzImage
uImage
fitImage
```

Which one matters depends on architecture, bootloader, and BSP workflow.

### `System.map`

Maps symbols to addresses for the built kernel. Useful for debugging and matching logs to a kernel build.

### `Module.symvers`

Contains symbol version information used by modules. Important when building external modules or diagnosing module compatibility.

### `*.ko`

Loadable kernel modules. These must match the running kernel version and configuration.

### `*.dtb`

Compiled device tree blobs. The deployed DTB must match the board and kernel expectations.

## Common Scenarios

### Source Exists But Output Is Missing

Likely causes:

- Kconfig option not enabled
- wrong architecture
- driver selected as module but modules were not built/installed
- source is excluded by Makefile condition
- building a different output directory than the one inspected

### Board Boots But Uses Old Kernel

The build output may be correct but deployment may be wrong. Check:

```sh
uname -a
cat /proc/version
cat /proc/cmdline
```

Then compare against the built artifact and bootloader load path.

### Multiple Output Trees

If you have:

```text
build-arm/
build-arm64/
build-debug/
```

make sure all commands use the intended `O=` directory. Inspecting the wrong output tree is a common mistake.

## Common Mistakes

- Editing generated files under the output directory.
- Building in-tree and then confusing generated output with source.
- Assuming every architecture produces the same boot image names.
- Forgetting to archive `vmlinux` and `System.map`.
- Copying `Image` but not the matching DTB or modules.
- Inspecting one `O=` directory while deploying artifacts from another.

## Debugging Checklist

- Confirm source tree path.
- Confirm output tree path.
- Confirm `ARCH`.
- Confirm `.config` in the output tree.
- Run `find` for expected artifacts.
- Check timestamps and checksums of deployed artifacts.
- Confirm runtime kernel version on the board.
- Confirm DTB and module outputs belong to the same build.

## Related Topics

- [Linux Kernel Build System](index.md)
- [Kconfig and Defconfig](kconfig-and-defconfig.md)
- [Kbuild Objects and Directories](kbuild-objects-and-directories.md)
- [BSP Artifact Flow and Provenance](../bsp-integration/artifact-flow-and-provenance.md)

## References

- Linux kernel Kbuild documentation
- Linux kernel source tree documentation
- Linux kernel admin guide

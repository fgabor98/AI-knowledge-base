---
status: draft
reviewed: false
domain: build-systems
difficulty: advanced
reviewer: null
last_reviewed: null
---

# Kconfig and Defconfig

## What Problem Does This Solve?

Kconfig decides which kernel features, drivers, filesystems, buses, and debugging options are enabled. Defconfigs and fragments make those choices reproducible.

For embedded Linux, kernel configuration controls whether the board boots, whether rootfs storage is available, whether drivers are built in or modular, and whether BSP changes survive rebuilds.

## Core Concepts

- Kconfig
- `CONFIG_*` symbol
- tristate
- dependency
- `select`
- `imply`
- `.config`
- defconfig
- config fragment
- `menuconfig`
- `oldconfig`
- `savedefconfig`
- generated `autoconf.h`

## Mental Model

Kconfig describes what can be configured. `.config` records what is selected. Kbuild uses `.config` to decide what to build.

```text
Kconfig files
-> defconfig/fragments/menu choices
-> .config
-> include/generated/autoconf.h
-> Kbuild object selection
```

The final `.config` is generated state. The source of truth should be a defconfig, fragments, or build-framework metadata.

## Syntax / API / Mechanism

Start with a defconfig:

```sh
make O=build ARCH=arm64 defconfig
```

Use a board defconfig:

```sh
make O=build ARCH=arm64 vendor_board_defconfig
```

Interactive config:

```sh
make O=build ARCH=arm64 menuconfig
```

Update old config:

```sh
make O=build ARCH=arm64 oldconfig
```

Save minimal defconfig:

```sh
make O=build ARCH=arm64 savedefconfig
```

Inspect final config:

```sh
grep CONFIG_I2C build/.config
```

## Minimal Example

```sh
make O=build ARCH=arm64 defconfig
make O=build ARCH=arm64 menuconfig
grep CONFIG_GPIO build/.config
make O=build ARCH=arm64 -j8
```

## Kconfig Symbol Values

Boolean:

```text
CONFIG_FOO=y
# CONFIG_FOO is not set
```

Tristate:

```text
CONFIG_FOO=y  -> built in
CONFIG_FOO=m  -> built as module
not set       -> not built
```

For early boot dependencies, built-in is often required. A storage driver needed to mount the rootfs cannot be a module unless an initramfs loads it first.

## Kconfig File Example

```kconfig
config MY_DRIVER
    tristate "My example driver"
    depends on I2C
    help
      Build support for the example I2C device.
```

Kbuild might use:

```make
obj-$(CONFIG_MY_DRIVER) += my_driver.o
```

## Defconfig Strategy

Defconfigs should be small enough to maintain and specific enough to reproduce.

Common sources:

```text
arch/arm/configs/<board>_defconfig
arch/arm64/configs/<board>_defconfig
Yocto kernel config fragments
Buildroot kernel config
vendor SDK config fragments
```

In product builds, preserve the inputs that generated `.config`.

## Config Fragments

Fragments are partial config files:

```text
CONFIG_I2C=y
CONFIG_SPI=y
CONFIG_MY_DRIVER=m
```

They are common in Yocto and vendor BSP workflows. Fragments let a product layer add specific options without replacing the entire vendor defconfig.

Important: a requested value may not appear in final `.config` if dependencies are unmet.

## Common Scenarios

### Option Does Not Appear

Likely causes:

- dependency not enabled
- symbol name is wrong
- fragment not applied
- another fragment overrides it
- selected kernel provider is not the one you inspected

Debug:

```sh
grep CONFIG_MY_DRIVER build/.config
grep -R "config MY_DRIVER" drivers/ arch/
```

### Driver Built As Module But Needed For Boot

If the driver is needed before rootfs mount, use built-in:

```text
CONFIG_MMC=y
CONFIG_EXT4_FS=y
```

not:

```text
CONFIG_MMC=m
```

unless an initramfs loads the module before rootfs mount.

### `select` Surprises

`select` can enable symbols without their normal dependency prompts being visible. When a symbol appears unexpectedly, search Kconfig files:

```sh
grep -R "select FOO" .
```

### Vendor Defconfig Upgrade

When upgrading BSP releases:

- compare old and new vendor defconfigs
- reapply product fragments
- verify final `.config`
- avoid blindly copying old full `.config` to a new kernel

## Yocto / Buildroot / TI SDK Integration

Yocto:

- kernel recipes and `.bbappend` files apply fragments
- final config appears under kernel work/build directories
- `bitbake -e virtual/kernel` helps inspect metadata

Buildroot:

- kernel config is selected through Buildroot configuration
- custom kernel configs can be stored in board or external trees

TI Processor SDK:

- TI kernel providers and machine metadata influence config
- keep SDK release, machine, kernel provider, and fragments aligned

## Common Mistakes

- Editing `.config` and not saving the source config.
- Treating `m` and `y` as interchangeable.
- Enabling a driver without enabling its bus dependency.
- Copying full old `.config` across major kernel versions.
- Assuming a fragment was applied without checking final `.config`.
- Forgetting that Kconfig choices can be architecture-specific.
- Debugging a missing driver without checking Kbuild object selection.

## Debugging Checklist

- Find the Kconfig symbol.
- Check dependencies.
- Check final `.config`.
- Check whether value is `y`, `m`, or unset.
- Check config fragments and their order.
- Check selected kernel provider and output tree.
- Check whether the driver object is selected in Kbuild.
- For boot-critical drivers, check whether module vs built-in is correct.

## Related Topics

- [Kernel Source Tree and Outputs](source-tree-and-outputs.md)
- [Kbuild Objects and Directories](kbuild-objects-and-directories.md)
- [Cross-Building and Installing](cross-building-and-installing.md)
- [Configuration and Patch Ownership](../bsp-integration/configuration-and-patch-ownership.md)

## References

- Linux kernel Kconfig documentation
- Linux kernel Kbuild documentation
- Yocto Project kernel development documentation

---
status: draft
reviewed: false
domain: build-systems
difficulty: advanced
reviewer: null
last_reviewed: null
---

# Kconfig and Generated Config

## What Problem Does This Solve?

U-Boot uses Kconfig to select commands, drivers, boot flows, SPL features, image formats, environment storage, and board behavior. Generated configuration then feeds C headers, Makefiles, and conditional compilation.

For embedded work, this explains why a command, driver, or boot feature may be missing even though the source code exists.

## Core Concepts

- Kconfig
- `CONFIG_*`
- defconfig
- `.config`
- generated headers
- `autoconf.mk`
- command selection
- driver selection
- dependency
- `select`
- SPL/TPL symbols

## Mental Model

U-Boot configuration follows a familiar pattern:

```text
Kconfig files describe options
-> defconfig requests values
-> olddefconfig resolves dependencies/defaults
-> generated config files feed build
-> Makefiles and C code include/exclude functionality
```

The final `.config` and generated headers are outputs. The maintained input is defconfig plus any project policy around it.

## Generated Files

After configuring, U-Boot generates files under the output directory:

```text
.config
include/generated/autoconf.h
include/config/auto.conf
include/config/uboot.release
```

These should not be edited manually. If they contain the wrong value, change the defconfig or Kconfig source.

## Checking A Symbol

Check final config:

```sh
grep '^CONFIG_CMD_MMC' build/.config
grep '^CONFIG_DM_SERIAL' build/.config
```

Check generated header:

```sh
grep 'CONFIG_CMD_MMC' build/include/generated/autoconf.h
```

If a symbol is missing, search Kconfig:

```sh
grep -R "config CMD_MMC" .
```

Use menuconfig search:

```sh
make O=build CROSS_COMPILE=aarch64-linux-gnu- menuconfig
```

Then press `/` and search for the symbol.

## Commands

U-Boot shell commands are usually selected through config symbols.

Examples:

```text
CONFIG_CMD_MMC=y
CONFIG_CMD_USB=y
CONFIG_CMD_NET=y
CONFIG_CMD_DHCP=y
CONFIG_CMD_PING=y
```

If a command is missing at the U-Boot prompt:

- check final `.config`
- check whether command support is compiled into SPL or only U-Boot proper
- check whether command was disabled to save size
- check whether the boot stage you are in supports the command

## Drivers

Drivers are also Kconfig-selected.

Examples:

```text
CONFIG_DM=y
CONFIG_DM_SERIAL=y
CONFIG_MMC=y
CONFIG_DM_MMC=y
CONFIG_SPI=y
CONFIG_DM_SPI=y
CONFIG_PHY=y
```

Driver model may also require device tree nodes. A driver can be built but not bind to hardware if the U-Boot DTB lacks the node or compatible string.

## SPL-Specific Configuration

Many U-Boot options have SPL variants:

```text
CONFIG_SPL=y
CONFIG_SPL_DM=y
CONFIG_SPL_MMC=y
CONFIG_SPL_SERIAL=y
CONFIG_SPL_OF_CONTROL=y
```

Do not assume an option enabled for U-Boot proper is enabled in SPL.

Common problem:

```text
CONFIG_MMC=y
# CONFIG_SPL_MMC is not set
```

U-Boot proper supports MMC, but SPL cannot load the next stage from MMC.

## Legacy Config Headers

Older U-Boot code and some board ports may still involve board headers under:

```text
include/configs/
```

Modern U-Boot has moved much configuration into Kconfig, but legacy headers still appear in real vendor trees.

When changing vendor U-Boot:

- prefer Kconfig where available
- understand existing legacy header usage
- avoid duplicating the same policy in Kconfig and a header
- check migration notes for the U-Boot version

## Environment Configuration

U-Boot environment behavior is partly controlled by config:

```text
CONFIG_ENV_IS_IN_MMC=y
CONFIG_ENV_IS_IN_SPI_FLASH=y
CONFIG_ENV_OFFSET=...
CONFIG_ENV_SIZE=...
CONFIG_USE_DEFAULT_ENV_FILE=y
```

Build-time environment defaults can be overridden by persistent environment stored on the board.

This creates a debugging trap: you rebuild U-Boot with new defaults, but the board keeps using old saved environment.

## Saving Config Changes

Workflow:

```sh
make O=build CROSS_COMPILE=aarch64-linux-gnu- board_defconfig
make O=build CROSS_COMPILE=aarch64-linux-gnu- menuconfig
make O=build CROSS_COMPILE=aarch64-linux-gnu- savedefconfig
```

Then inspect:

```sh
diff -u configs/board_defconfig build/defconfig
```

Commit only intentional changes.

## Yocto And TI SDK Integration

In Yocto, U-Boot configuration is controlled by machine metadata, recipe variables, patches, and defconfigs.

Check:

```sh
bitbake -e virtual/bootloader | grep -E 'UBOOT|SPL|MACHINE|SRCREV'
```

For TI Processor SDK:

- identify the selected U-Boot provider
- identify the selected machine
- inspect deploy artifacts
- inspect the configured U-Boot work directory
- check whether TI packaging tools add extra stages

## Common Mistakes

- Editing generated headers.
- Checking source code but not final `.config`.
- Enabling U-Boot proper support but forgetting SPL support.
- Ignoring device tree requirements for driver model.
- Expecting rebuilt default environment to override saved persistent environment.
- Carrying old defconfig values across major U-Boot upgrades without `olddefconfig`.

## Debugging Checklist

- Check final `.config`.
- Check generated `autoconf.h`.
- Check whether the symbol is for SPL, TPL, or U-Boot proper.
- Search Kconfig dependencies.
- Check selected defconfig.
- Check persistent environment.
- Check U-Boot DTB for driver model devices.
- Use verbose build output if object selection is unclear.

## Related Topics

- [Board Defconfigs](board-defconfigs.md)
- [SPL, TPL, and U-Boot Proper](spl-tpl-and-u-boot-proper.md)
- [Device Tree in U-Boot](device-tree-in-u-boot.md)
- [Debugging U-Boot Builds](debugging-u-boot-builds.md)

## References

- U-Boot Kconfig documentation
- U-Boot driver model documentation
- U-Boot environment documentation

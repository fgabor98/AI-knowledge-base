---
status: draft
reviewed: false
domain: build-systems
difficulty: advanced
reviewer: null
last_reviewed: null
---

# Environment and Boot Flow

## What Problem Does This Solve?

U-Boot environment and boot flow decide what actually gets loaded after U-Boot starts. Many apparent build problems are really environment, boot script, FIT configuration, `extlinux.conf`, or persistent-variable problems.

For embedded Linux work, this topic is critical because rebuilding U-Boot does not guarantee the board uses the new boot command, boot arguments, kernel file, DTB, or FIT configuration.

## Core Concepts

- default environment
- saved environment
- persistent environment
- `bootcmd`
- `boot_targets`
- distro boot
- boot script
- `extlinux.conf`
- FIT configuration
- `bootargs`
- kernel command line
- environment storage backend

## Mental Model

U-Boot boot flow is layered:

```text
compiled default environment
-> optional saved persistent environment
-> boot command selection
-> script/extlinux/FIT logic
-> kernel, DTB, initramfs loading
-> Linux bootargs handoff
```

The saved environment usually wins over rebuilt defaults. That is the most important rule.

## Default Vs Saved Environment

The default environment is compiled into U-Boot. It may come from:

- board code
- text environment files
- Kconfig-selected defaults
- distro boot defaults
- vendor SDK patches

Saved environment is stored on the target, often in:

- MMC/eMMC
- SPI flash
- NAND
- FAT/ext partition
- redundant environment slots

If saved environment exists, U-Boot loads it and overrides compiled defaults.

## Basic Environment Commands

At U-Boot prompt:

```text
printenv
printenv bootcmd
printenv bootargs
version
bdinfo
```

Change a variable temporarily:

```text
setenv bootdelay 3
```

Persist it:

```text
saveenv
```

Reset to compiled defaults:

```text
env default -a
```

Persist defaults:

```text
saveenv
```

Use reset carefully. Environment variables may contain board identity, MAC addresses, manufacturing data, boot media policy, or recovery settings.

## `bootcmd`

`bootcmd` is the command U-Boot executes automatically after `bootdelay`.

Example:

```text
bootcmd=run distro_bootcmd
```

or:

```text
bootcmd=load mmc 0:1 ${kernel_addr_r} Image; load mmc 0:1 ${fdt_addr_r} board.dtb; booti ${kernel_addr_r} - ${fdt_addr_r}
```

To debug:

```text
printenv bootcmd
run bootcmd
```

Break complex boot flows into the variables they call:

```text
printenv distro_bootcmd
printenv boot_targets
printenv bootcmd_mmc0
```

## `boot_targets`

Many modern U-Boot configurations use distro boot logic:

```text
boot_targets=mmc0 mmc1 usb0 pxe dhcp
```

The order matters. If `mmc0` has an old boot partition, U-Boot may never reach the media you updated.

Debug:

```text
printenv boot_targets
setenv boot_targets mmc1
run distro_bootcmd
```

For permanent change:

```text
saveenv
```

Only save after confirming the new order is correct.

## Boot Scripts

Boot scripts are usually text commands converted with `mkimage`.

Source:

```text
boot.cmd
```

Compiled:

```text
boot.scr
```

Build:

```sh
mkimage -A arm64 -T script -C none -n "boot script" -d boot.cmd boot.scr
```

Inspect:

```sh
dumpimage -l boot.scr
```

Common failure: editing `boot.cmd` but deploying an old `boot.scr`.

## `extlinux.conf`

Some U-Boot flows read:

```text
/extlinux/extlinux.conf
```

or:

```text
/boot/extlinux/extlinux.conf
```

Example:

```text
label Linux
    kernel /Image
    fdt /dtbs/board.dtb
    append root=/dev/mmcblk0p2 rootwait console=ttyS2,115200
```

Debug by checking:

- which partition U-Boot scans
- which path it expects
- whether `kernel`, `fdt`, and `append` match deployed files
- whether saved environment changes the scan order

## FIT Boot Flow

When booting a FIT:

```text
load mmc 0:1 ${loadaddr} image.itb
bootm ${loadaddr}#conf-custom-board
```

If no config is specified, U-Boot may use the FIT default configuration.

Debug:

```text
iminfo ${loadaddr}
bootm ${loadaddr}#conf-name
```

On host:

```sh
dumpimage -l image.itb
```

## Bootargs And Kernel Command Line

U-Boot passes kernel command line through `bootargs` or through script/extlinux/FIT-generated arguments.

Check in U-Boot:

```text
printenv bootargs
```

Check in Linux:

```sh
cat /proc/cmdline
```

If they differ, the boot flow may generate or override bootargs after you inspected them.

## DTB Selection

U-Boot may pass a DTB from:

- standalone file
- FIT image
- U-Boot internal DTB
- overlay-modified DTB
- board-detected DTB name
- environment variable such as `fdtfile`

Debug:

```text
printenv fdtfile
printenv fdt_addr_r
fdt addr ${fdt_addr_r}
fdt print / model
```

Then verify in Linux:

```sh
cat /proc/device-tree/model
cat /proc/device-tree/compatible
```

## Environment Storage Configuration

Build-time configuration controls where saved environment lives:

```text
CONFIG_ENV_IS_IN_MMC
CONFIG_ENV_IS_IN_SPI_FLASH
CONFIG_ENV_IS_IN_NAND
CONFIG_ENV_IS_NOWHERE
CONFIG_ENV_OFFSET
CONFIG_ENV_SIZE
```

Wrong environment storage configuration can cause:

- environment not saving
- old environment loaded from unexpected media
- corruption of nearby boot data
- redundant environment mismatch

## Debugging Saved Environment Problems

Symptoms:

- rebuilt default bootcmd ignored
- board boots old kernel path
- old bootargs remain
- `saveenv` fails
- changing boot targets has no effect after reset

Checks:

```text
printenv
env info
version
```

Depending on U-Boot version, `env info` may show environment backend and validity.

## Production Policy

Decide:

- whether production devices use saved environment
- which variables are allowed to be changed in field
- how environment is initialized during manufacturing
- whether environment is redundant
- how MAC addresses and serial numbers are stored
- how to recover from invalid environment

Do not let ad hoc development environment become product boot policy.

## TI Sitara Considerations

For TI SDK-style systems, inspect:

- SDK default environment
- boot partition contents
- `uEnv.txt` or boot script usage if present
- FIT vs standalone kernel/DTB flow
- selected `fdtfile`
- eMMC/SD boot priority
- saved environment location

Keep U-Boot environment, kernel artifact names, DTB names, and rootfs layout aligned.

## Common Mistakes

- Rebuilding U-Boot defaults while saved environment overrides them.
- Editing `boot.cmd` but not regenerating `boot.scr`.
- Updating `extlinux.conf` on a partition U-Boot does not scan.
- Changing `bootargs` but boot flow later overwrites it.
- Booting default FIT config instead of board-specific config.
- Updating SD files while `boot_targets` tries eMMC first.
- Resetting environment and losing manufacturing variables.

## Debugging Checklist

- Capture full serial log from reset.
- Run `version`.
- Run `printenv`.
- Identify `bootcmd`.
- Identify `boot_targets`.
- Identify boot script or `extlinux.conf` path.
- Identify FIT config if used.
- Identify kernel, DTB, and initramfs filenames.
- Compare U-Boot `bootargs` with Linux `/proc/cmdline`.
- Compare U-Boot DTB choice with Linux `/proc/device-tree`.
- Check saved vs default environment behavior.

## Related Topics

- [FIT Images and Boot Artifacts](fit-images-and-boot-artifacts.md)
- [Cross-Building and Flashing](cross-building-and-flashing.md)
- [Debugging U-Boot Builds](debugging-u-boot-builds.md)
- [Image Layout and Deployment](../bsp-integration/image-layout-and-deployment.md)

## References

- U-Boot environment documentation
- U-Boot distro boot documentation
- U-Boot boot command documentation
